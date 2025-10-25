#pragma once

#include <string>
#include <map>
#include <vector>
#include <functional>
#include <memory>

class HttpClient {
public:
    using Headers = std::map<std::string, std::string>;
    using ResponseCallback = std::function<void(int status_code,
                                              const std::string& response,
                                              const Headers& headers)>;
    using ErrorCallback = std::function<void(const std::string& error)>;

    struct Request {
        std::string url;
        std::string method = "GET";
        std::string body;
        Headers headers;
        int timeout_ms = 30000;
        bool follow_redirects = true;
        bool verify_ssl = true;
    };

    struct Response {
        int status_code = 0;
        std::string body;
        Headers headers;
        std::string error;
    };

    HttpClient();
    ~HttpClient();

    Response request(const Request& req);
    Response get(const std::string& url, const Headers& headers = {});
    Response post(const std::string& url,
                 const std::string& body,
                 const Headers& headers = {});
    Response put(const std::string& url,
                const std::string& body,
                const Headers& headers = {});
    Response delete_(const std::string& url, const Headers& headers = {});

    void request_async(const Request& req,
                      ResponseCallback on_response,
                      ErrorCallback on_error = nullptr);
    void get_async(const std::string& url,
                  ResponseCallback on_response,
                  ErrorCallback on_error = nullptr,
                  const Headers& headers = {});
    void post_async(const std::string& url,
                   const std::string& body,
                   ResponseCallback on_response,
                   ErrorCallback on_error = nullptr,
                   const Headers& headers = {});

    void set_base_url(const std::string& base_url);
    void set_default_headers(const Headers& headers);
    void set_timeout(int timeout_ms);
    void set_proxy(const std::string& proxy);
    void set_ssl_verify(bool verify);
    void set_user_agent(const std::string& user_agent);

    static std::string url_encode(const std::string& str);
    static std::string url_decode(const std::string& str);
    static std::string build_query_string(const std::map<std::string, std::string>& params);
    static Headers parse_headers(const std::string& header_string);

    void set_cookie(const std::string& name, const std::string& value);
    std::string get_cookie(const std::string& name) const;
    void clear_cookies();

    void clear_session();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    HttpClient(const HttpClient&) = delete;
    HttpClient& operator=(const HttpClient&) = delete;
};