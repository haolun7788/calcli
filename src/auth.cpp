#include "calcli/auth.hpp"
#include "calcli/util.hpp"
#include <nlohmann/json.hpp>
#include <random>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <stdexcept>
#include <httplib.h>


namespace calcli {

    namespace {
        std::string random_state() {
            static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dist(0, sizeof(charset) - 2);
            std::string s(24, '0');
            for (auto& c : s) c = charset[dist(gen)];
            return s;
        }

        void open_browser(const std::string& url) {
            #ifdef _WIN32
                std::string cmd = "start \"\" \"" + url + "\"";
            #elif __APPLE__
                std::string cmd = "open \"" + url + "\"";
            #else
                std::string cmd = "xdg-open \"" + url + "\"";
            #endif
                std::system(cmd.c_str());
        }
    } // namespace

    AuthManager::AuthManager(ClientCredentials creds, std::string token_path, IHttpClient& http)
        : creds_(std::move(creds)), token_path_(std::move(token_path)), http_(http) {
        tokens_ = load_tokens();
    }

    std::string AuthManager::bearer_token() {
        if (!tokens_) tokens_ = load_tokens();

        if (!tokens_) {
            tokens_ = run_consent_flow();
            save_tokens(*tokens_);
        } else if (std::chrono::system_clock::now() >= tokens_->expires_at) {
            tokens_ = refresh_tokens(tokens_->refresh_token);
            save_tokens(*tokens_);
        }
        return tokens_->access_token;
    }

    std::optional<TokenSet> AuthManager::load_tokens() {
        std::ifstream in(token_path_);
        if (!in) return std::nullopt;

        nlohmann::json j;
        in >> j;
        TokenSet tokens;
        tokens.access_token = j.at("access_token").get<std::string>();
        tokens.refresh_token = j.at("refresh_token").get<std::string>();
        tokens.expires_at = std::chrono::system_clock::time_point(std::chrono::seconds(j.at("expires_at").get<int64_t>()));
        return tokens;
    }

    void AuthManager::save_tokens(const TokenSet& tokens) {
        nlohmann::json j;
        j["access_token"] = tokens.access_token;
        j["refresh_token"] = tokens.refresh_token;
        j["expires_at"] = std::chrono::duration_cast<std::chrono::seconds>(tokens.expires_at.time_since_epoch()).count();

        std::ofstream out(token_path_);
        if (!out) {
            throw std::runtime_error("Could not open token file for writing: " + token_path_);
        }
        out << j.dump(4);
    }

    TokenSet AuthManager::run_consent_flow() {
        const std::string redirect_uri = "http://localhost:8080/callback";
        const std::string state = random_state();
        const std::string scope = "https://www.googleapis.com/auth/calendar.events";

        std::string auth_url =
            "https://accounts.google.com/o/oauth2/v2/auth"
            "?client_id=" + url_encode(creds_.client_id) +
            "&redirect_uri=" + url_encode(redirect_uri) +
            "&response_type=code"
            "&access_type=offline"
            "&prompt=consent"
            "&scope=" + url_encode(scope) +
            "&state=" + state;

        httplib::Server svr;
        std::string received_code, received_state;

        svr.Get("/callback", [&](const httplib::Request& req, httplib::Response& res) {
            received_code = req.get_param_value("code");
            received_state = req.get_param_value("state");
            res.set_content(
                "<html><body>Authorization complete — you can close this tab.</body></html>",
                "text/html");
            svr.stop();
        });

        open_browser(auth_url);
        svr.listen("localhost", 8080);  // blocks until the handler calls svr.stop()

        if (received_state != state) {
            throw std::runtime_error("OAuth state mismatch — possible CSRF, aborting");
        }

        return exchange_code_for_tokens(received_code, redirect_uri);
    }

    TokenSet AuthManager::exchange_code_for_tokens(const std::string& code,
                                                    const std::string& redirect_uri) {
        std::string body =
            "code=" + url_encode(code) +
            "&client_id=" + url_encode(creds_.client_id) +
            "&client_secret=" + url_encode(creds_.client_secret) +
            "&redirect_uri=" + url_encode(redirect_uri) +
            "&grant_type=authorization_code";

        auto resp = http_.post("https://oauth2.googleapis.com/token",
                                {"Content-Type: application/x-www-form-urlencoded"}, body);
        return parse_token_response(resp, std::nullopt);
    }

    TokenSet AuthManager::refresh_tokens(const std::string& refresh_token) {
        std::string body =
            "refresh_token=" + url_encode(refresh_token) +
            "&client_id=" + url_encode(creds_.client_id) +
            "&client_secret=" + url_encode(creds_.client_secret) +
            "&grant_type=refresh_token";

        auto resp = http_.post("https://oauth2.googleapis.com/token",
                                {"Content-Type: application/x-www-form-urlencoded"}, body);
        return parse_token_response(resp, refresh_token);
    }

    TokenSet AuthManager::parse_token_response(const HttpResponse& resp,
                                                std::optional<std::string> existing_refresh_token) {
        if (resp.status_code != 200) {
            throw std::runtime_error("OAuth token request failed: " + resp.body);
        }
        auto j = nlohmann::json::parse(resp.body);  // let it throw here — a malformed
                                                    // token response is a real failure,
                                                    // not something to silently swallow

        TokenSet tokens;
        tokens.access_token = j.at("access_token").get<std::string>();
        int expires_in = j.value("expires_in", 3600);
        tokens.expires_at = std::chrono::system_clock::now() + std::chrono::seconds(expires_in);

        if (j.contains("refresh_token")) {
            tokens.refresh_token = j["refresh_token"].get<std::string>();
        } else if (existing_refresh_token) {
            tokens.refresh_token = *existing_refresh_token;
        } else {
            throw std::runtime_error("No refresh_token in response and none on file");
        }
        return tokens;
    }

} // namespace calcli