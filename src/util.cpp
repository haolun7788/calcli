#include <string>
#include <chrono>

namespace calcli {
    std::string url_encode(const std::string& value) {
        std::ostringstream escaped;
        escaped.fill('0');
        escaped << std::hex;
        for (unsigned char c : value) {
            if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
                escaped << c;
            } else {
                escaped << '%' << std::setw(2) << std::uppercase << int(c);
            }
        }
        return escaped.str();
    }

    std::string format_rfc3339(std::chrono::system_clock::time_point tp) {
        auto secs = std::chrono::floor<std::chrono::seconds>(tp);
        auto days_part = std::chrono::floor<std::chrono::days>(secs);
        std::chrono::year_month_day ymd{days_part};
        std::chrono::hh_mm_ss hms{secs - days_part};

        char buf[32];
        std::snprintf(buf, sizeof(buf), "%04d-%02u-%02uT%02lld:%02lld:%02lldZ",
              int(ymd.year()), unsigned(ymd.month()), unsigned(ymd.day()),
              (long long)hms.hours().count(),
              (long long)hms.minutes().count(),
              (long long)hms.seconds().count());
        return buf;
    }
}