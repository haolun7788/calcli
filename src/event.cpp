#include <string>
#include <optional>
#include <chrono>
#include <cstdio>
#include <nlohmann/json.hpp>
#include <calcli/event.hpp>

namespace calcli {

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

    std::optional<std::chrono::system_clock::time_point> parse_rfc3339(const std::string &s) {
        int year, month, day, hour, minute, second;
        // Strict: expects exactly "YYYY-MM-DDTHH:MM:SSZ" — good enough since it's
        // parsing back what to_json produces; real API responses may vary
        // (offsets instead of Z, fractional seconds) — a problem for later.
        if (s.size() != 20 || s.back() != 'Z')
            return std::nullopt;
        if (std::sscanf(s.c_str(), "%d-%d-%dT%d:%d:%dZ",
                        &year, &month, &day, &hour, &minute, &second) != 6)
        {
            return std::nullopt;
        }

        std::chrono::year_month_day ymd{std::chrono::year{year},
                                        std::chrono::month{unsigned(month)},
                                        std::chrono::day{unsigned(day)}};
        if (!ymd.ok())
            return std::nullopt; // rejects e.g. month=13, day=32

        std::chrono::sys_days base{ymd};
        return base + std::chrono::hours{hour} + std::chrono::minutes{minute} + std::chrono::seconds{second};
    }

    std::string to_json(const Event& e) {
        nlohmann::json j;
        j["summary"] = e.summary;
        if (e.location) {
            j["location"] = *e.location;
        }
        j["start"]["dateTime"] = format_rfc3339(e.start);
        j["end"]["dateTime"] = format_rfc3339(e.end);
        return j.dump();
    }

    std::optional<Event> from_json(const std::string& json_body) {
        try {
            nlohmann::json j = nlohmann::json::parse(json_body);

            if (!j.contains("summary") || !j.contains("start") || !j.contains("end")) {
                return std::nullopt;
            }
            if (!j["start"].contains("dateTime") || !j["end"].contains("dateTime")) {
                return std::nullopt;
            }

            auto start = parse_rfc3339(j["start"]["dateTime"]);
            auto end = parse_rfc3339(j["end"]["dateTime"]);
            if (!start || !end) return std::nullopt;

            Event e;
            e.summary = j["summary"];
            if (j.contains("location")) e.location = j["location"];
            e.start = *start;
            e.end = *end;
            return e;
        } catch (const nlohmann::json::exception&) {
            return std::nullopt;
        }
    }

} // namespace calcli