#pragma once
#include <string>
#include <chrono>

namespace calcli {
    std::string url_encode(const std::string& value);
    std::string format_rfc3339(std::chrono::system_clock::time_point tp);
}