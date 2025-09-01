#ifndef ANTHROPIC_HPP_
#define ANTHROPIC_HPP_


#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <optional>
#include <map>
#include <sstream>
#include <stdexcept>
#include <nlohmann/json.hpp>

#ifdef _WIN32
    #include <windows.h>
    #include <winhttp.h>
    #pragma comment(lib, "winhttp.lib")
#else
    #include <curl/curl.h>
#endif

using json = nlohmann::json;

namespace anthropic {

// Exception classes
class AnthropicException : public std::runtime_error {
public:
    explicit AnthropicException(const std::string& message) : std::runtime_error(message) {}
};

class APIException : public AnthropicException {
public:
    int status_code;
    APIException(int code, const std::string& message) 
        : AnthropicException("API Error " + std::to_string(code) + ": " + message), status_code(code) {}
};

// Message structure
struct Message {
    std::string role;  // "user" or "assistant"
    std::string content;
    
    Message(const std::string& r, const std::string& c) : role(r), content(c) {}
};

// Request parameters
struct ChatRequest {
    std::string model = "claude-3-sonnet-20240229";
    std::vector<Message> messages;
    int max_tokens = 1000;
    double temperature = 1.0;
    std::vector<std::string> stop_sequences;
    std::string system;
    bool stream = false;
    
    // Add a message to the conversation
    void add_message(const std::string& role, const std::string& content) {
        messages.emplace_back(role, content);
    }
    
    void add_user_message(const std::string& content) {
        add_message("user", content);
    }
    
    void add_assistant_message(const std::string& content) {
        add_message("assistant", content);
    }
};

// Response structure
struct ChatResponse {
    std::string id;
    std::string type;
    std::string role;
    std::string content;
    std::string model;
    std::string stop_reason;
    int input_tokens = 0;
    int output_tokens = 0;
    
    bool is_error = false;
    std::string error_message;
    int error_code = 0;
};

// HTTP Response structure
struct HttpResponse {
    int status_code;
    std::string body;
    std::map<std::string, std::string> headers;
};

// HTTP Client interface
class HttpClient {
public:
    virtual ~HttpClient() = default;
    virtual HttpResponse post(const std::string& url, 
                             const std::string& data, 
                             const std::map<std::string, std::string>& headers) = 0;
};

#ifdef _WIN32
// Windows WinHTTP implementation
class WinHttpClient : public HttpClient {
private:
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* response) {
        size_t total_size = size * nmemb;
        response->append((char*)contents, total_size);
        return total_size;
    }

public:
    HttpResponse post(const std::string& url, 
                     const std::string& data, 
                     const std::map<std::string, std::string>& headers) override {
        HttpResponse response;
        
        // Parse URL
        std::wstring wurl(url.begin(), url.end());
        URL_COMPONENTS urlComp = {};
        urlComp.dwStructSize = sizeof(urlComp);
        urlComp.dwSchemeLength = -1;
        urlComp.dwHostNameLength = -1;
        urlComp.dwUrlPathLength = -1;
        
        if (!WinHttpCrackUrl(wurl.c_str(), (DWORD)wurl.length(), 0, &urlComp)) {
            throw AnthropicException("Failed to parse URL");
        }
        
        std::wstring hostname(urlComp.lpszHostName, urlComp.dwHostNameLength);
        std::wstring path(urlComp.lpszUrlPath, urlComp.dwUrlPathLength);
        
        // Initialize WinHTTP
        HINTERNET hSession = WinHttpOpen(L"Anthropic-CPP-Client/1.0",
                                        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                        WINHTTP_NO_PROXY_NAME,
                                        WINHTTP_NO_PROXY_BYPASS, 0);
        if (!hSession) {
            throw AnthropicException("Failed to initialize WinHTTP session");
        }
        
        // Connect
        HINTERNET hConnect = WinHttpConnect(hSession, hostname.c_str(), 
                                           urlComp.nPort, 0);
        if (!hConnect) {
            WinHttpCloseHandle(hSession);
            throw AnthropicException("Failed to connect");
        }
        
        // Create request
        DWORD flags = (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
        HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", path.c_str(),
                                               NULL, WINHTTP_NO_REFERER,
                                               WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
        if (!hRequest) {
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            throw AnthropicException("Failed to create request");
        }
        
        // Set headers
        for (const auto& header : headers) {
            std::wstring wheader = std::wstring(header.first.begin(), header.first.end()) + 
                                  L": " + std::wstring(header.second.begin(), header.second.end());
            WinHttpAddRequestHeaders(hRequest, wheader.c_str(), -1, WINHTTP_ADDREQ_FLAG_ADD);
        }
        
        // Send request
        BOOL result = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                        (LPVOID)data.c_str(), (DWORD)data.length(),
                                        (DWORD)data.length(), 0);
        
        if (result) {
            result = WinHttpReceiveResponse(hRequest, NULL);
        }
        
        if (result) {
            // Get status code
            DWORD statusCode = 0;
            DWORD statusCodeSize = sizeof(statusCode);
            WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                               WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusCodeSize, WINHTTP_NO_HEADER_INDEX);
            response.status_code = statusCode;
            
            // Read response
            DWORD bytesAvailable = 0;
            while (WinHttpQueryDataAvailable(hRequest, &bytesAvailable) && bytesAvailable > 0) {
                std::vector<char> buffer(bytesAvailable + 1, 0);
                DWORD bytesRead = 0;
                if (WinHttpReadData(hRequest, buffer.data(), bytesAvailable, &bytesRead)) {
                    response.body.append(buffer.data(), bytesRead);
                }
            }
        }
        
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        
        if (!result) {
            throw ClaudeException("HTTP request failed");
        }
        
        return response;
    }
};

#else
// Linux/Mac cURL implementation
class CurlHttpClient : public HttpClient {
private:
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* response) {
        size_t total_size = size * nmemb;
        response->append((char*)contents, total_size);
        return total_size;
    }

public:
    CurlHttpClient() {
        curl_global_init(CURL_GLOBAL_DEFAULT);
    }
    
    ~CurlHttpClient() {
        curl_global_cleanup();
    }
    
    HttpResponse post(const std::string& url, 
                     const std::string& data, 
                     const std::map<std::string, std::string>& headers) override {
        HttpResponse response;
        
        CURL* curl = curl_easy_init();
        if (!curl) {
            throw ClaudeException("Failed to initialize cURL");
        }
        
        // Set URL
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        
        // Set POST data
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, data.length());
        
        // Set headers
        struct curl_slist* header_list = nullptr;
        for (const auto& header : headers) {
            std::string header_str = header.first + ": " + header.second;
            header_list = curl_slist_append(header_list, header_str.c_str());
        }
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header_list);
        
        // Set callback for response data
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response.body);
        
        // Perform the request
        CURLcode res = curl_easy_perform(curl);
        
        if (res != CURLE_OK) {
            curl_slist_free_all(header_list);
            curl_easy_cleanup(curl);
            throw ClaudeException("cURL error: " + std::string(curl_easy_strerror(res)));
        }
        
        // Get status code
        long status_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code);
        response.status_code = static_cast<int>(status_code);
        
        curl_slist_free_all(header_list);
        curl_easy_cleanup(curl);
        
        return response;
    }
};
#endif

// Main Claude API Client
class ClaudeClient {
private:
    std::string api_key_;
    std::string base_url_;
    std::unique_ptr<HttpClient> http_client_;
    
    // Build JSON request using nlohmann/json
    std::string build_request_json(const ChatRequest& request) {
        json j;
        j["model"] = request.model;
        j["max_tokens"] = request.max_tokens;
        j["temperature"] = request.temperature;
        
        if (!request.system.empty()) {
            j["system"] = request.system;
        }
        
        if (!request.stop_sequences.empty()) {
            j["stop_sequences"] = request.stop_sequences;
        }
        
        j["messages"] = json::array();
        for (const auto& msg : request.messages) {
            json msg_obj;
            msg_obj["role"] = msg.role;
            msg_obj["content"] = msg.content;
            j["messages"].push_back(msg_obj);
        }
        
        return j.dump();
    }
    
    // Parse JSON response using nlohmann/json
    ChatResponse parse_response_json(const std::string& json_str) {
        ChatResponse response;
        
        try {
            json j = json::parse(json_str);
            
            // Handle error responses
            if (j.contains("error")) {
                response.is_error = true;
                response.error_message = j["error"].contains("message") ? 
                    j["error"]["message"].get<std::string>() : "Unknown error";
                response.error_code = j["error"].contains("type") ? 
                    (j["error"]["type"] == "invalid_request_error" ? 400 : 500) : 500;
                return response;
            }
            
            // Parse successful response
            if (j.contains("id")) {
                response.id = j["id"].get<std::string>();
            }
            
            if (j.contains("type")) {
                response.type = j["type"].get<std::string>();
            }
            
            if (j.contains("role")) {
                response.role = j["role"].get<std::string>();
            }
            
            if (j.contains("model")) {
                response.model = j["model"].get<std::string>();
            }
            
            if (j.contains("stop_reason")) {
                response.stop_reason = j["stop_reason"].get<std::string>();
            }
            
            // Extract content from the response
            if (j.contains("content") && j["content"].is_array() && !j["content"].empty()) {
                // Claude API returns content as an array of content blocks
                const auto& content_block = j["content"][0];
                if (content_block.contains("text")) {
                    response.content = content_block["text"].get<std::string>();
                }
            }
            
            // Extract usage information
            if (j.contains("usage")) {
                const auto& usage = j["usage"];
                if (usage.contains("input_tokens")) {
                    response.input_tokens = usage["input_tokens"].get<int>();
                }
                if (usage.contains("output_tokens")) {
                    response.output_tokens = usage["output_tokens"].get<int>();
                }
            }
            
        } catch (const json::parse_error& e) {
            response.is_error = true;
            response.error_message = "JSON parse error: " + std::string(e.what());
            response.error_code = 500;
        } catch (const json::type_error& e) {
            response.is_error = true;
            response.error_message = "JSON type error: " + std::string(e.what());
            response.error_code = 500;
        }
        
        return response;
    }
    
public:
    explicit ClaudeClient(const std::string& api_key, 
                         const std::string& base_url = "https://api.anthropic.com") 
        : api_key_(api_key), base_url_(base_url) {
#ifdef _WIN32
        http_client_ = std::make_unique<WinHttpClient>();
#else
        http_client_ = std::make_unique<CurlHttpClient>();
#endif
    }
    
    // Set custom HTTP client
    void set_http_client(std::unique_ptr<HttpClient> client) {
        http_client_ = std::move(client);
    }
    
    // Main chat completion method
    ChatResponse chat(const ChatRequest& request) {
        std::string url = base_url_ + "/v1/messages";
        std::string json_data = build_request_json(request);
        
        std::map<std::string, std::string> headers = {
            {"Content-Type", "application/json"},
            {"x-api-key", api_key_},
            {"anthropic-version", "2023-06-01"}
        };
        
        try {
            HttpResponse http_response = http_client_->post(url, json_data, headers);
            
            if (http_response.status_code >= 400) {
                ChatResponse error_response;
                error_response.is_error = true;
                error_response.error_code = http_response.status_code;
                error_response.error_message = http_response.body;
                return error_response;
            }
            
            return parse_response_json(http_response.body);
            
        } catch (const std::exception& e) {
            ChatResponse error_response;
            error_response.is_error = true;
            error_response.error_message = e.what();
            return error_response;
        }
    }
    
    // Convenience method for single message
    ChatResponse ask(const std::string& message, const std::string& system = "") {
        ChatRequest request;
        request.system = system;
        request.add_user_message(message);
        return chat(request);
    }
    
    // List available models
    std::vector<std::string> list_models() {
        return {
            "claude-3-opus-20240229",
            "claude-3-sonnet-20240229", 
            "claude-3-haiku-20240307",
            "claude-3-5-sonnet-20241022"
        };
    }
    
    // Get model information
    json get_model_info(const std::string& model_name) {
        json model_info;
        model_info["name"] = model_name;
        
        // Add basic model information (you could extend this)
        if (model_name.find("opus") != std::string::npos) {
            model_info["tier"] = "premium";
            model_info["context_window"] = 200000;
            model_info["description"] = "Most capable model for complex tasks";
        } else if (model_name.find("sonnet") != std::string::npos) {
            model_info["tier"] = "balanced";
            model_info["context_window"] = 200000;
            model_info["description"] = "Balanced model for most use cases";
        } else if (model_name.find("haiku") != std::string::npos) {
            model_info["tier"] = "fast";
            model_info["context_window"] = 200000;
            model_info["description"] = "Fastest model for simple tasks";
        }
        
        return model_info;
    }
};

// Builder pattern for easier request construction
class ChatRequestBuilder {
private:
    ChatRequest request_;
    
public:
    ChatRequestBuilder& model(const std::string& model) {
        request_.model = model;
        return *this;
    }
    
    ChatRequestBuilder& max_tokens(int tokens) {
        request_.max_tokens = tokens;
        return *this;
    }
    
    ChatRequestBuilder& temperature(double temp) {
        request_.temperature = temp;
        return *this;
    }
    
    ChatRequestBuilder& system(const std::string& sys) {
        request_.system = sys;
        return *this;
    }
    
    ChatRequestBuilder& user_message(const std::string& content) {
        request_.add_user_message(content);
        return *this;
    }
    
    ChatRequestBuilder& assistant_message(const std::string& content) {
        request_.add_assistant_message(content);
        return *this;
    }
    
    ChatRequestBuilder& stop_sequences(const std::vector<std::string>& stops) {
        request_.stop_sequences = stops;
        return *this;
    }
    
    ChatRequestBuilder& add_stop_sequence(const std::string& stop) {
        request_.stop_sequences.push_back(stop);
        return *this;
    }
    
    ChatRequestBuilder& stream(bool enable_stream) {
        request_.stream = enable_stream;
        return *this;
    }
    
    ChatRequest build() {
        return request_;
    }
};

#endif
