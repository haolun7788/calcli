#pragma once
#include "calcli/http_client.hpp"

namespace calcli {

class CurlHttpClient : public IHttpClient {
    public:
        CurlHttpClient();
        ~CurlHttpClient() override;

        HttpResponse get(const std::string& url, const std::vector<std::string>& headers) override;
        HttpResponse post(const std::string& url, const std::vector<std::string>& headers,
                        const std::string& body) override;
        HttpResponse patch(const std::string& url, const std::vector<std::string>& headers,
                            const std::string& body) override;
        HttpResponse del(const std::string& url, const std::vector<std::string>& headers) override;

    private:
        HttpResponse perform(const std::string& method, const std::string& url,
                            const std::vector<std::string>& headers,
                            const std::string* body);
    };
} // namespace calcli