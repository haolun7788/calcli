#pragma once
#include <string>
#include <map>
#include <optional>
#include <filesystem>

namespace calcli {

    struct Config {
        std::string default_calendar_id = "primary";
        std::map<std::string, std::string> calendar_aliases;
    };

    std::string config_path();
    Config load_config();
    void save_config(const Config& cfg);

    std::filesystem::path app_data_dir();

    // cli_arg may be empty (use default), an alias name, or a literal calendar ID
    std::string resolve_calendar(const Config& cfg, const std::optional<std::string>& cli_arg);

} // namespace calcli