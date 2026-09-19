#pragma once
#include <string>
#include <vector>

namespace calcli {

struct HttpResponse {
    int status_code;
    std::string body;
};

class IHttpClient {
    public:
        virtual ~IHttpClient() = default;
        virtual HttpResponse get(const std::string& url,
                                const std::vector<std::string>& headers) = 0;
        virtual HttpResponse post(const std::string& url,
                                const std::vector<std::string>& headers,
                                const std::string& body) = 0;
        virtual HttpResponse patch(const std::string& url,
                                    const std::vector<std::string>& headers,
                                    const std::string& body) = 0;
        virtual HttpResponse del(const std::string& url,
                                const std::vector<std::string>& headers) = 0;
    };
} // namespace calcli