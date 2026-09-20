#include "calcli/config.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <cstdlib>
#include <filesystem>

namespace calcli {

    std::filesystem::path app_data_dir() {
    #if defined(_WIN32)
        const char* appdata = std::getenv("APPDATA");
        std::filesystem::path base = appdata ? appdata : ".";
    #else
        const char* xdg = std::getenv("XDG_CONFIG_HOME");
        std::filesystem::path base = (xdg && *xdg)
            ? std::filesystem::path(xdg)
            : std::filesystem::path(std::getenv("HOME") ? std::getenv("HOME") : ".") / ".config";
    #endif
        auto dir = base / "calcli";
        std::filesystem::create_directories(dir);
        return dir;
    }

    std::string config_path() {
        return (app_data_dir() / "config.json").string();
    }

    Config load_config() {
        Config cfg;  // defaults if nothing else works out
        std::ifstream in(config_path());
        if (!in) return cfg;  // no file on first run, defaults are fine

        try {
            nlohmann::json j;
            in >> j;
            if (j.contains("default_calendar_id")) {
                cfg.default_calendar_id = j["default_calendar_id"].get<std::string>();
            }
            if (j.contains("calendar_aliases")) {
                for (auto& [key, value] : j["calendar_aliases"].items()) {
                    cfg.calendar_aliases[key] = value.get<std::string>();
                }
            }
        } catch (const nlohmann::json::exception&) {
            return Config{};  // malformed config shouldn't crash the whole CLI
        }
        return cfg;
    }

    void save_config(const Config& cfg) {
        std::filesystem::path path = config_path();
        std::filesystem::create_directories(path.parent_path());

        nlohmann::json j;
        j["default_calendar_id"] = cfg.default_calendar_id;
        j["calendar_aliases"] = cfg.calendar_aliases;

        std::ofstream out(path);
        out << j.dump(2);  // pretty-printed: hand-edit this at first
    }

    std::string resolve_calendar(const Config& cfg, const std::optional<std::string>& cli_arg) {
        std::string target = cli_arg.value_or(cfg.default_calendar_id);
        auto it = cfg.calendar_aliases.find(target);
        return it != cfg.calendar_aliases.end() ? it->second : target;
    }

} // namespace calcli