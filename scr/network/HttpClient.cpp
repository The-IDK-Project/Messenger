#include "network/HttpClient.h"
#include "utils/Logger.h"
#include "utils/StringUtils.h"
#include <curl/curl.h>
#include <thread>
#include <sstream>

class HttpClient::Impl {
public:
    Impl() {
        curl_global_init(CURL_GLOBAL_DEFAULT);
        curl_ = curl_easy_init();
        set_default_options();
    }

    ~Impl() {
        if (curl_) {
            curl_easy_cleanup(curl_);
        }
        curl_global_cleanup();
    }

    Response request(const Request& req) {
        std::lock_guard<std::mutex> lock(mutex_);

        Response response;
        if (!curl_) {
            response.error = "CURL not initialized";
            return response;
        }
        std::string url = req.url;
        if (!base_url_.empty() && url.find("://") == std::string::npos) {
            url = base_url_ + url;
        }
        curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl_, CURLOPT_CUSTOMREQUEST, req.method.c_str());
        struct curl_slist* headers = nullptr;
        for (const auto& [key, value] : default_headers_) {
            std::string header = key + ": " + value;
            headers = curl_slist_append(headers, header.c_str());
        }
        for (const auto& [key, value] : req.headers) {
            std::string header = key + ": " + value;
            headers = curl_slist_append(headers, header.c_str());
        }
        curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers);
        if (!req.body.empty() && (req.method == "POST" || req.method == "PUT")) {
            curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, req.body.c_str());
        }
        curl_easy_setopt(curl_, CURLOPT_TIMEOUT_MS, req.timeout_ms);
        curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, req.follow_redirects ? 1L : 0L);
        curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, req.verify_ssl ? 1L : 0L);
        curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYHOST, req.verify_ssl ? 2L : 0L);
        if (!proxy_.empty()) {
            curl_easy_setopt(curl_, CURLOPT_PROXY, proxy_.c_str());
        }

        std::string response_body;
        curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response_body);
        std::string header_data;
        curl_easy_setopt(curl_, CURLOPT_HEADERFUNCTION, header_callback);
        curl_easy_setopt(curl_, CURLOPT_HEADERDATA, &header_data);
        CURLcode res = curl_easy_perform(curl_);
        if (headers) {
            curl_slist_free_all(headers);
        }

        if (res != CURLE_OK) {
            response.error = curl_easy_strerror(res);
            return response;
        }
        long status_code;
        curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &status_code);
        response.status_code = static_cast<int>(status_code);
        response.body = response_body;
        response.headers = parse_headers(header_data);
        store_cookies();

        return response;
    }

    void set_base_url(const std::string& base_url) {
        std::lock_guard<std::mutex> lock(mutex_);
        base_url_ = base_url;
    }

    void set_default_headers(const Headers& headers) {
        std::lock_guard<std::mutex> lock(mutex_);
        default_headers_ = headers;
    }

    void set_timeout(int timeout_ms) {
        std::lock_guard<std::mutex> lock(mutex_);
        timeout_ms_ = timeout_ms;
    }

    void set_proxy(const std::string& proxy) {
        std::lock_guard<std::mutex> lock(mutex_);
        proxy_ = proxy;
    }

    void set_cookie(const std::string& name, const std::string& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        cookies_[name] = value;
        update_cookie_header();
    }

    std::string get_cookie(const std::string& name) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = cookies_.find(name);
        return it != cookies_.end() ? it->second : "";
    }

    void clear_cookies() {
        std::lock_guard<std::mutex> lock(mutex_);
        cookies_.clear();
        update_cookie_header();
    }

    void clear_session() {
        std::lock_guard<std::mutex> lock(mutex_);
        cookies_.clear();
        default_headers_.clear();
        curl_easy_setopt(curl_, CURLOPT_COOKIELIST, "ALL");
    }

private:
    CURL* curl_ = nullptr;
    std::string base_url_;
    Headers default_headers_;
    std::string proxy_;
    int timeout_ms_ = 30000;
    std::map<std::string, std::string> cookies_;
    mutable std::mutex mutex_;

    void set_default_options() {
        if (!curl_) return;

        curl_easy_setopt(curl_, CURLOPT_USERAGENT, "UnifiedMessenger/1.0");
        curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl_, CURLOPT_MAXREDIRS, 10L);
        curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYHOST, 2L);
        curl_easy_setopt(curl_, CURLOPT_TIMEOUT_MS, timeout_ms_);
        curl_easy_setopt(curl_, CURLOPT_COOKIEFILE, ""); // Enable cookie engine
    }

    void update_cookie_header() {
        std::string cookie_header;
        for (const auto& [name, value] : cookies_) {
            if (!cookie_header.empty()) cookie_header += "; ";
            cookie_header += name + "=" + value;
        }

        if (!cookie_header.empty()) {
            default_headers_["Cookie"] = cookie_header;
        } else {
            default_headers_.erase("Cookie");
        }
    }

    void store_cookies() {
        if (!curl_) return;

        struct curl_slist* cookies = nullptr;
        curl_easy_getinfo(curl_, CURLINFO_COOKIELIST, &cookies);

        if (cookies) {
            cookies_.clear();
            struct curl_slist* nc = cookies;
            while (nc) {
                std::string cookie_line = nc->data;
                size_t eq_pos = cookie_line.find('=');
                if (eq_pos != std::string::npos) {
                    size_t tab_pos = cookie_line.find('\t');
                    if (tab_pos != std::string::npos) {
                        std::string name = cookie_line.substr(tab_pos + 1, eq_pos - tab_pos - 1);
                        std::string value = cookie_line.substr(eq_pos + 1);
                        cookies_[name] = value;
                    }
                }
                nc = nc->next;
            }
            curl_slist_free_all(cookies);
        }
    }

    static size_t write_callback(void* contents, size_t size, size_t nmemb, std::string* response) {
        size_t total_size = size * nmemb;
        response->append(static_cast<char*>(contents), total_size);
        return total_size;
    }

    static size_t header_callback(void* contents, size_t size, size_t nmemb, std::string* headers) {
        size_t total_size = size * nmemb;
        headers->append(static_cast<char*>(contents), total_size);
        return total_size;
    }

    static Headers parse_headers(const std::string& header_data) {
        Headers headers;
        std::istringstream stream(header_data);
        std::string line;

        while (std::getline(stream, line)) {
            if (line.empty() || line == "\r") continue;

            size_t colon_pos = line.find(':');
            if (colon_pos != std::string::npos) {
                std::string key = line.substr(0, colon_pos);
                std::string value = line.substr(colon_pos + 1);

                key = StringUtils::trim(key);
                value = StringUtils::trim(value);

                headers[key] = value;
            }
        }

        return headers;
    }
};

HttpClient::HttpClient() : impl_(std::make_unique<Impl>()) {}
HttpClient::~HttpClient() = default;

HttpClient::Response HttpClient::request(const Request& req) {
    return impl_->request(req);
}

HttpClient::Response HttpClient::get(const std::string& url, const Headers& headers) {
    Request req;
    req.url = url;
    req.method = "GET";
    req.headers = headers;
    return request(req);
}

HttpClient::Response HttpClient::post(const std::string& url, const std::string& body, const Headers& headers) {
    Request req;
    req.url = url;
    req.method = "POST";
    req.body = body;
    req.headers = headers;
    return request(req);
}

HttpClient::Response HttpClient::put(const std::string& url, const std::string& body, const Headers& headers) {
    Request req;
    req.url = url;
    req.method = "PUT";
    req.body = body;
    req.headers = headers;
    return request(req);
}

HttpClient::Response HttpClient::delete_(const std::string& url, const Headers& headers) {
    Request req;
    req.url = url;
    req.method = "DELETE";
    req.headers = headers;
    return request(req);
}

void HttpClient::set_base_url(const std::string& base_url) {
    impl_->set_base_url(base_url);
}

void HttpClient::set_default_headers(const Headers& headers) {
    impl_->set_default_headers(headers);
}

void HttpClient::set_timeout(int timeout_ms) {
    impl_->set_timeout(timeout_ms);
}

void HttpClient::set_proxy(const std::string& proxy) {
    impl_->set_proxy(proxy);
}

void HttpClient::set_cookie(const std::string& name, const std::string& value) {
    impl_->set_cookie(name, value);
}

std::string HttpClient::get_cookie(const std::string& name) const {
    return impl_->get_cookie(name);
}

void HttpClient::clear_cookies() {
    impl_->clear_cookies();
}

void HttpClient::clear_session() {
    impl_->clear_session();
}

std::string HttpClient::url_encode(const std::string& str) {
    CURL* curl = curl_easy_init();
    if (!curl) return str;

    char* encoded = curl_easy_escape(curl, str.c_str(), static_cast<int>(str.length()));
    std::string result = encoded ? encoded : str;

    curl_free(encoded);
    curl_easy_cleanup(curl);

    return result;
}

std::string HttpClient::url_decode(const std::string& str) {
    CURL* curl = curl_easy_init();
    if (!curl) return str;

    int out_length;
    char* decoded = curl_easy_unescape(curl, str.c_str(), static_cast<int>(str.length()), &out_length);
    std::string result = decoded ? std::string(decoded, out_length) : str;

    curl_free(decoded);
    curl_easy_cleanup(curl);

    return result;
}

std::string HttpClient::build_query_string(const std::map<std::string, std::string>& params) {
    std::string query;
    for (const auto& [key, value] : params) {
        if (!query.empty()) query += "&";
        query += url_encode(key) + "=" + url_encode(value);
    }
    return query;
}

HttpClient::Headers HttpClient::parse_headers(const std::string& header_string) {
    return Impl::parse_headers(header_string);
}