#include "protocols/MatrixHandler.h"
#include "network/HttpClient.h"
#include "network/WebSocketClient.h"
#include "utils/Logger.h"
#include "utils/JsonParser.h"
#include "utils/StringUtils.h"
#include <curl/curl.h>
#include <thread>
#include <sstream>

MatrixHandler::MatrixHandler()
    : homeserver_("https://matrix.org")
    , state_(ProtocolState::DISCONNECTED)
    , curl_handle_(nullptr)
    , ws_handle_(nullptr) {

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl_handle_ = curl_easy_init();
}

MatrixHandler::MatrixHandler(const std::string& homeserver)
    : homeserver_(homeserver)
    , state_(ProtocolState::DISCONNECTED)
    , curl_handle_(nullptr)
    , ws_handle_(nullptr) {

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl_handle_ = curl_easy_init();
}

MatrixHandler::~MatrixHandler() {
    disconnect();

    if (curl_handle_) {
        curl_easy_cleanup(curl_handle_);
    }
    curl_global_cleanup();
}

bool MatrixHandler::connect() {
    if (state_ != ProtocolState::DISCONNECTED) {
        LOG_WARNING("MatrixHandler already connected or connecting");
        return false;
    }

    state_ = ProtocolState::CONNECTING;

    if (access_token_.empty()) {
        LOG_ERROR("No access token available for Matrix connection");
        state_ = ProtocolState::ERROR;
        return false;
    }

    if (!http_request("GET", "/_matrix/client/r0/sync", "", next_batch_token_)) {
        LOG_ERROR("Failed to perform initial sync");
        state_ = ProtocolState::ERROR;
        return false;
    }

    state_ = ProtocolState::CONNECTED;
    LOG_INFO("MatrixHandler connected successfully");

    start_sync_loop();

    return true;
}

void MatrixHandler::disconnect() {
    state_ = ProtocolState::DISCONNECTED;
    stop_sync_loop();
    LOG_INFO("MatrixHandler disconnected");
}

ProtocolState MatrixHandler::get_state() const {
    return state_;
}

bool MatrixHandler::is_connected() const {
    return state_ == ProtocolState::CONNECTED;
}

bool MatrixHandler::login_password(const std::string& username, const std::string& password) {
    JsonValue login_data = JsonValue::object();
    login_data["type"] = "m.login.password";
    login_data["identifier"] = JsonValue::object();
    login_data["identifier"]["type"] = "m.id.user";
    login_data["identifier"]["user"] = username;
    login_data["password"] = password;

    std::string response;
    if (!http_request("POST", "/_matrix/client/r0/login", login_data.to_string(), response)) {
        LOG_ERROR("Matrix login failed");
        return false;
    }

    try {
        JsonValue json = JsonValue::parse(response);
        access_token_ = json["access_token"].as_string();
        user_id_ = json["user_id"].as_string();
        device_id_ = json["device_id"].as_string();

        LOG_INFO("Matrix login successful for user: " + user_id_);
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to parse login response: " + std::string(e.what()));
        return false;
    }
}

bool MatrixHandler::login_token(const std::string& access_token) {
    access_token_ = access_token;

    std::string response;
    if (!http_request("GET", "/_matrix/client/r0/account/whoami", "", response)) {
        LOG_ERROR("Matrix token verification failed");
        access_token_.clear();
        return false;
    }

    try {
        JsonValue json = JsonValue::parse(response);
        user_id_ = json["user_id"].as_string();

        LOG_INFO("Matrix token login successful for user: " + user_id_);
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to parse whoami response: " + std::string(e.what()));
        access_token_.clear();
        return false;
    }
}

bool MatrixHandler::send_message(const std::string& room_id, const std::string& message) {
    if (!is_connected()) {
        LOG_ERROR("MatrixHandler not connected");
        return false;
    }

    JsonValue content = JsonValue::object();
    content["msgtype"] = "m.text";
    content["body"] = message;

    std::string txn_id = generateTransactionId();
    std::string endpoint = "/_matrix/client/r0/rooms/" + room_id + "/send/m.room.message/" + txn_id;

    std::string response;
    if (!http_request("PUT", endpoint, content.to_string(), response)) {
        LOG_ERROR("Failed to send Matrix message");
        return false;
    }

    LOG_DEBUG("Matrix message sent to room: " + room_id);
    return true;
}

bool MatrixHandler::send_file(const std::string& room_id, const std::string& file_path, const std::string& filename) {
    std::string mxc_uri;
    if (!upload_file(file_path, mxc_uri)) {
        return false;
    }

    JsonValue content = JsonValue::object();
    content["msgtype"] = "m.file";
    content["body"] = filename.empty() ? "file" : filename;
    content["url"] = mxc_uri;

    std::string txn_id = generateTransactionId();
    std::string endpoint = "/_matrix/client/r0/rooms/" + room_id + "/send/m.room.message/" + txn_id;

    std::string response;
    if (!http_request("PUT", endpoint, content.to_string(), response)) {
        LOG_ERROR("Failed to send Matrix file message");
        return false;
    }

    return true;
}

std::vector<ChatRoom> MatrixHandler::get_rooms() {
    std::vector<ChatRoom> rooms;

    if (!is_connected()) {
        return rooms;
    }

    std::string response;
    if (!http_request("GET", "/_matrix/client/r0/joined_rooms", "", response)) {
        return rooms;
    }

    try {
        JsonValue json = JsonValue::parse(response);
        auto joined_rooms = json["joined_rooms"];

        for (size_t i = 0; i < joined_rooms.size(); ++i) {
            std::string room_id = joined_rooms[i].as_string();

            std::string room_response;
            if (http_request("GET", "/_matrix/client/r0/rooms/" + room_id + "/state", "", room_response)) {
                JsonValue room_json = JsonValue::parse(room_response);

                ChatRoom room;
                room.id = room_id;
                room.protocol = "matrix";

                for (size_t j = 0; j < room_json.size(); ++j) {
                    auto event = room_json[j];
                    if (event["type"].as_string() == "m.room.name") {
                        room.name = event["content"]["name"].as_string();
                        break;
                    }
                }

                if (room.name.empty()) {
                    room.name = "Matrix Room";
                }

                rooms.push_back(room);
            }
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to parse rooms response: " + std::string(e.what()));
    }

    return rooms;
}

User MatrixHandler::get_current_user() {
    User user;
    user.id = user_id_;
    user.protocols = {"matrix"};

    std::string response;
    if (http_request("GET", "/_matrix/client/r0/profile/" + user_id_, "", response)) {
        try {
            JsonValue json = JsonValue::parse(response);
            user.display_name = json["displayname"].as_string();
            user.avatar_url = json["avatar_url"].as_string();
        } catch (...) {
        }
    }

    return user;
}

uint32_t MatrixHandler::get_capabilities() const {
    return static_cast<uint32_t>(ProtocolCapabilities::MESSAGES) |
           static_cast<uint32_t>(ProtocolCapabilities::FILES) |
           static_cast<uint32_t>(ProtocolCapabilities::ENCRYPTION) |
           static_cast<uint32_t>(ProtocolCapabilities::TYPING) |
           static_cast<uint32_t>(ProtocolCapabilities::READ_RECEIPTS);
}

bool MatrixHandler::http_request(const std::string& method,
                                const std::string& endpoint,
                                const std::string& data,
                                std::string& response) {
    if (!curl_handle_) {
        return false;
    }

    std::string url = homeserver_ + endpoint;

    curl_easy_setopt(curl_handle_, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_handle_, CURLOPT_CUSTOMREQUEST, method.c_str());

    if (!access_token_.empty()) {
        std::string auth_header = "Authorization: Bearer " + access_token_;
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, auth_header.c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl_handle_, CURLOPT_HTTPHEADER, headers);
    }

    if (!data.empty()) {
        curl_easy_setopt(curl_handle_, CURLOPT_POSTFIELDS, data.c_str());
    }

    curl_easy_setopt(curl_handle_, CURLOPT_WRITEFUNCTION,
        [](void* contents, size_t size, size_t nmemb, std::string* response) {
            size_t total_size = size * nmemb;
            response->append(static_cast<char*>(contents), total_size);
            return total_size;
        });
    curl_easy_setopt(curl_handle_, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl_handle_);

    if (res != CURLE_OK) {
        LOG_ERROR("HTTP request failed: " + std::string(curl_easy_strerror(res)));
        return false;
    }

    return true;
}

std::string MatrixHandler::generateTransactionId() {
    static int counter = 0;
    return "m" + std::to_string(std::time(nullptr)) + std::to_string(counter++);
}

void MatrixHandler::start_sync_loop() {
}

void MatrixHandler::stop_sync_loop() {
}