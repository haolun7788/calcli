#include <CLI/CLI.hpp>
#include <iostream>
#include <string>
#include <filesystem>
#include <calcli/config.hpp>
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
        if (!in) throw std::runtime_error("no credentials file at " + path);

        nlohmann::json j;
        in >> j;

        if (j.contains("installed")) {  // Google's raw downloaded shape
            const auto& installed = j.at("installed");
            return { installed.at("client_id").get<std::string>(),
                    installed.at("client_secret").get<std::string>() };
        }
        // simplified shape: {"client_id": "...", "client_secret": "..."}
        return { j.at("client_id").get<std::string>(),
                j.at("client_secret").get<std::string>() };
    }

    std::pair<std::string, std::string> credential_paths() {
        auto dir = calcli::app_data_dir();
        return { (dir / "client_secret.json").string(), (dir / "token.json").string() };
    }

    calcli::ClientCredentials prompt_for_credentials(const std::string& path) {
        std::cout << "No Google OAuth credentials found.\n"
                    "Create a free OAuth client (Desktop app type) at:\n"
                    "  https://console.cloud.google.com/apis/credentials\n\n"
                    "Client ID: ";
        std::string id, secret;
        std::getline(std::cin, id);
        std::cout << "Client Secret: ";
        std::getline(std::cin, secret);

        nlohmann::json j;
        j["client_id"] = id;
        j["client_secret"] = secret;
        std::ofstream(path) << j.dump(2);

        return {id, secret};
    }

    calcli::AuthManager make_auth_manager(calcli::IHttpClient& http) {
        auto [creds_path, token_path] = credential_paths();
        calcli::ClientCredentials creds;
        try {
            creds = load_client_credentials(creds_path);
        } catch (const std::exception&) {
            creds = prompt_for_credentials(creds_path);
        }
        return calcli::AuthManager(creds, token_path, http);
    }
}

int main(int argc, char **argv) {
    CLI::App app{"calcli - fast Google Calender management"};
    app.set_version_flag("--version", "calcli 0.1.0");
    app.require_subcommand(1);

    std::string title;
    std::vector<std::string> when_tokens;
    std::string location;
    std::string calendar_arg;

    auto* auth_cmd = app.add_subcommand("auth", "Manage Google account authentication");
    auth_cmd->require_subcommand(1);
    auto* login_cmd = auth_cmd->add_subcommand("login", "Authenticate with your Google account");
    auto* status_cmd = auth_cmd->add_subcommand("status", "Show whether you're logged in");
    auto* logout_cmd = auth_cmd->add_subcommand("logout", "Remove cached credentials");

    auto* calendars_cmd = app.add_subcommand("calendars", "List available calendars");

    auto* add_cmd = app.add_subcommand("add", "Add new event");
    add_cmd->add_option("title", title, "Event title")->required();
    add_cmd->add_option("when", when_tokens,"Natural-language day/time/duration, e.g. friday 2:30 1 hour")->required();
    add_cmd->add_option("--location,-l", location, "Event location");
    add_cmd->add_option("--calendar,-c", calendar_arg, "Calendar name/alias or ID");

    auto* config_cmd = app.add_subcommand("config", "View or edit configuration");
    std::string set_default, alias_name, alias_id;
    config_cmd->add_option("--set-default", set_default, "Set the default calendar (alias or ID)");
    config_cmd->add_option("--add-alias", alias_name, "Alias name (pair with --id)");
    config_cmd->add_option("--id", alias_id, "Calendar ID for --add-alias");

    CLI11_PARSE(app, argc, argv);

    calcli::CurlHttpClient http;

    if (*add_cmd) {
        auto result = calcli::parse_when(when_tokens, std::chrono::system_clock::now());
        if (!result) {
            std::cerr << "Error: couldn't understand the date/time/duration.\n";
            return 1;
        }

        calcli::Event e;
        e.summary = title;
        if (!location.empty()) e.location = location;
        e.start = result->start;
        e.end = result->end;

        try {
            auto auth = make_auth_manager(http);
            calcli::CalendarApi api(auth, http);

            auto cfg = calcli::load_config();
            std::optional<std::string> cal_override;
            if (!calendar_arg.empty()) cal_override = calendar_arg;
            std::string calendar_id = calcli::resolve_calendar(cfg, cal_override);

            auto created = api.create_event(calendar_id, e);
            if (!created) {
                std::cerr << "Error: failed to create event\n";
                return 1;
            }
            std::cout << "Created event: " << created->id.value_or("(no id)") << "\n";
        } catch (const std::exception& ex) {
            std::cerr << "Error: " << ex.what() << "\n";
            return 1;
        }
    } else if (*login_cmd) {
        try {
            auto auth = make_auth_manager(http);
            auth.bearer_token();  // triggers the browser consent flow if not cached
            std::cout << "Logged in successfully.\n";
        } catch (const std::exception& ex) {
            std::cerr << "Login failed: " << ex.what() << "\n";
            return 1;
        }
    } else if (*status_cmd) {
        auto [creds_path, token_path] = credential_paths();
        if (std::filesystem::exists(token_path)) {
            std::cout << "Logged in (credentials cached).\n";
        } else {
            std::cout << "Not logged in. Run `calcli auth login`.\n";
        }
    } else if (*logout_cmd) {
        auto [creds_path, token_path] = credential_paths();
        if (std::filesystem::remove(token_path)) {
            std::cout << "Logged out.\n";
        } else {
            std::cout << "No cached credentials found.\n";
        }
    } else if (*calendars_cmd) {
        try {
            auto auth = make_auth_manager(http);
            calcli::CalendarApi api(auth, http);
            auto calendars = api.list_calendars();
            if (!calendars) {
                std::cerr << "Error: failed to fetch calendars\n";
                return 1;
            }
            for (auto& c : *calendars) {
                std::cout << c.summary << "  (" << c.id << ")\n";
            }
        } catch (const std::exception& ex) {
            std::cerr << "Error: " << ex.what() << "\n";
            return 1;
        }
    } else if (*config_cmd) {
        auto cfg = calcli::load_config();
        bool changed = false;
        if (!set_default.empty()) { cfg.default_calendar_id = set_default; changed = true; }
        if (!alias_name.empty() && !alias_id.empty()) {
            cfg.calendar_aliases[alias_name] = alias_id;
            changed = true;
        }
        if (changed) {
            calcli::save_config(cfg);
            std::cout << "Config updated.\n";
        } else {
            std::cout << "Default calendar: " << cfg.default_calendar_id << "\n";
            for (auto& [name, id] : cfg.calendar_aliases) {
                std::cout << "  alias " << name << " -> " << id << "\n";
            }
        }
    }
    return 0;
}
