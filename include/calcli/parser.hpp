#pragma once
#include <chrono>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace calcli {

    struct ParsedWhen {
        std::chrono::system_clock::time_point start;
        std::chrono::system_clock::time_point end;
    };
    
    std::optional<std::chrono::weekday> parse_weekday(const std::string& token);
    std::optional<std::pair<int, int>> parse_clock_time(const std::string& token); // {hour, minute}
    std::optional<int> parse_number(const std::string& token);
    std::optional<std::chrono::minutes> parse_duration(int amount, const std::string& unit_token);
    std::optional<ParsedWhen> parse_when(const std::vector<std::string>& tokens, std::chrono::system_clock::time_point now);

}  // namespace calcli