#include <curl/curl.h>
#include <stdexcept>
#include "calcli/curl_http_client.hpp"

namespace calcli {

    namespace {
        size_t write_callback(char* ptr, size_t size, size_t nmemb, void* userdata) {
            auto* out = static_cast<std::string*>(userdata);
            out->append(ptr, size * nmemb);
            return size * nmemb;
        }
    } //namespace

    CurlHttpClient::CurlHttpClient() {
        curl_global_init(CURL_GLOBAL_DEFAULT);
    }

    CurlHttpClient::~CurlHttpClient() {
        curl_global_cleanup();
    }

    HttpResponse CurlHttpClient::perform(const std::string& method, const std::string& url,
                                        const std::vector<std::string>& headers,
                                        const std::string* body) {
        CURL* curl = curl_easy_init();
        if (!curl) {
            throw std::runtime_error("Failed to initialize CURL");
        }

        std::string response_body;

        struct curl_slist* header_list = nullptr;
        for (const auto& header : headers) {
            header_list = curl_slist_append(header_list, header.c_str());
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header_list);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);

        if (method == "POST") {
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body->c_str());
        } else if (method == "PATCH" || method == "DELETE") {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method.c_str());
            if (body) curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body->c_str());
        }

        CURLcode res = curl_easy_perform(curl);
        long response_code = 0;
        
        if (res != CURLE_OK) {
            curl_slist_free_all(header_list);
            curl_easy_cleanup(curl);
            throw std::runtime_error("CURL request failed: " + std::string(curl_easy_strerror(res)));
        }

        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

        curl_slist_free_all(header_list);
        curl_easy_cleanup(curl);

        return {static_cast<int>(response_code), response_body};
    }

    HttpResponse CurlHttpClient::get(const std::string& url, const std::vector<std::string>& headers) {
        return perform("GET", url, headers, nullptr);
    }

    HttpResponse CurlHttpClient::post(const std::string& url, const std::vector<std::string>& headers, const std::string& body) {
        return perform("POST", url, headers, &body);
    }

    HttpResponse CurlHttpClient::patch(const std::string& url, const std::vector<std::string>& headers, const std::string& body) {
        return perform("PATCH", url, headers, &body);
    }

    HttpResponse CurlHttpClient::del(const std::string& url, const std::vector<std::string>& headers) {
        return perform("DELETE", url, headers, nullptr);
    }

} // namespace calcli