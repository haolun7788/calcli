#include <CLI/CLI.hpp>
#include <iostream>
#include <string>
#include <calcli/parser.hpp>
#include <calcli/event.hpp>
#include <calcli/auth.hpp>
#include <calcli/calendar_api.hpp>
#include <calcli/curl_http_client.hpp>
#include <nlohmann/json.hpp>
#include <fstream>

namespace {
    calcli::ClientCredentials load_client_credentials(const std::string& path) {
        std::ifstream in(path);
        if (!in) {
            throw std::runtime_error("Could not open credentials file: " + path);
        }
        nlohmann::json j;
        in >> j;
        const auto& installed = j.at("installed");
        return calcli::ClientCredentials{
            installed.at("client_id").get<std::string>(),
            installed.at("client_secret").get<std::string>()
        };
    }
}

int main(int argc, char **argv) {
    CLI::App app{"calcli - fast Google Calender management"};
    app.require_subcommand(1);

    std::string title;
    std::vector<std::string> when_tokens;
    std::string location;
    
    auto* add_cmd = app.add_subcommand("add", "Add new event");
    add_cmd->add_option("title", title, "Event title")->required();
    add_cmd->add_option("when", when_tokens,
        "Natural-language day/time/duration, e.g. friday 2:30 1 hour")->required();
    add_cmd->add_option("--location,-l", location, "Event location");

    auto* auth_test_cmd = app.add_subcommand("auth-test", "Test OAuth authentication");

    CLI11_PARSE(app, argc, argv);

    if (*add_cmd) {
        auto result = calcli::parse_when(when_tokens, std::chrono::system_clock::now());
        if (!result) {
            std::cerr << "Error: couldn't understand the date/time/duration. \n";
            return 1;
        }

        calcli::Event e;
        e.summary = title;
        if (!location.empty()) {
            e.location = location;
        }
        e.start = result->start;
        e.end = result->end;

        auto creds = load_client_credentials("credentials/client_secret.json");
        calcli::CurlHttpClient http;
        calcli::AuthManager auth(creds, "credentials/token.json", http);
        calcli::CalendarApi api(auth, http);

        auto created = api.create_event("primary",e);
        if (!created) {
            std::cerr << "Error: failed to create event\n";
            return 1;
        }
        std::cout << "Created event: " << created->id.value_or("(no id)") << "\n";
    } 
    if (*auth_test_cmd) {
        try {
            auto creds = load_client_credentials("credentials/client_secret.json");
            calcli::CurlHttpClient http;
            calcli::AuthManager auth(creds, "credentials/token.json", http);

            std::string token = auth.bearer_token();
            std::cout << "Got bearer token (first 12 chars): "
                    << token.substr(0, 12) << "...\n";
        } catch (const std::exception& e) {
            std::cerr << "Auth test failed: " << e.what() << "\n";
            return 1;
        }
        return 0;
    }
    return 0;
}
