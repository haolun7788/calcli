#include "calcli/http_client.hpp"
#include <map>


class MockHttpClient : public calcli::IHttpClient {
public:
    struct Call { std::string method, url, body; std::vector<std::string> headers; };
    std::vector<Call> calls;
    std::map<std::string, calcli::HttpResponse> canned_responses; // key: "METHOD url"

    calcli::HttpResponse get(const std::string& url, const std::vector<std::string>& headers) override {
        calls.push_back({"GET", url, "", headers});
        return canned_responses.at("GET " + url);
    }
    calcli::HttpResponse post(const std::string& url, const std::vector<std::string>& headers, const std::string& body) override {
        calls.push_back({"POST", url, body, headers});
        return canned_responses.at("POST " + url);
    }
    calcli::HttpResponse patch(const std::string& url, const std::vector<std::string>& headers, const std::string& body) override {
        calls.push_back({"PATCH", url, body, headers});
        return canned_responses.at("PATCH " + url);
    }
    calcli::HttpResponse del(const std::string& url, const std::vector<std::string>& headers) override {
        calls.push_back({"DELETE", url, "", headers});
        return canned_responses.at("DELETE " + url);
    }
};