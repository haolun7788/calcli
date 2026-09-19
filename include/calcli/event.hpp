#pragma once
#include <string>
#include <optional>
#include <chrono>

namespace calcli {

struct Event {
    std::optional<std::string> id;   // empty until Google assigns one
    std::string summary;
    std::optional<std::string> location;
    std::chrono::system_clock::time_point start;
    std::chrono::system_clock::time_point end;
};

std::string to_json(const Event& e);
std::optional<Event> from_json(const std::string& json_body);

} // namespace calcli