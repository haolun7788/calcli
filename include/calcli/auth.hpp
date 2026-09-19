#pragma once
#include <string>
#include <optional>
#include <chrono>
#include "calcli/http_client.hpp"

namespace calcli {
    struct ClientCredentials {
        std::string client_id;
        std::string client_secret;
    };

    struct TokenSet {
        std::string access_token;
        std::string refresh_token;
        std::chrono::system_clock::time_point expires_at;
    };

    class AuthManager {
        public:
            AuthManager(ClientCredentials creds, std::string token_path, IHttpClient& http);

            // Returns a valid bearer token, running the browser consent flow
            // (first run) or a silent refresh (expired token) as needed.
            std::string bearer_token();

        private:
            ClientCredentials creds_;
            std::string token_path_;
            IHttpClient& http_;
            std::optional<TokenSet> tokens_;

            std::optional<TokenSet> load_tokens();
            void save_tokens(const TokenSet& tokens);
            TokenSet run_consent_flow();
            TokenSet refresh_tokens(const std::string& refresh_token);
            TokenSet exchange_code_for_tokens(const std::string& code, const std::string& redirect_uri);
            TokenSet parse_token_response(const HttpResponse& resp, std::optional<std::string> existing_refresh_token);
    };

} // namespace calcli