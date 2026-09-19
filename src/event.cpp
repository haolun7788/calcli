#include <string>
#include <optional>
#include <chrono>
#include <cstdio>
#include <nlohmann/json.hpp>
#include <calcli/event.hpp>
#include <calcli/util.hpp>

namespace calcli {

    std::optional<std::chrono::system_clock::time_point> parse_rfc3339(const std::string& s) {
        if (s.size() < 20) return std::nullopt;

        int year, month, day, hour, minute, second;
        if (std::sscanf(s.c_str(), "%d-%d-%dT%d:%d:%d",
                        &year, &month, &day, &hour, &minute, &second) != 6) {
            return std::nullopt;
        }

        std::string offset_part = s.substr(19);  // everything after "YYYY-MM-DDTHH:MM:SS"
        if (!offset_part.empty() && offset_part[0] == '.') {
            auto tz_start = offset_part.find_first_of("Z+-", 1);
            if (tz_start == std::string::npos) return std::nullopt;
            offset_part = offset_part.substr(tz_start);
        }

        int offset_minutes = 0;
        if (offset_part == "Z") {
            offset_minutes = 0;
        } else if (offset_part.size() == 6 && (offset_part[0] == '+' || offset_part[0] == '-')) {
            int oh, om;
            if (std::sscanf(offset_part.c_str() + 1, "%d:%d", &oh, &om) != 2) return std::nullopt;
            offset_minutes = (oh * 60 + om) * (offset_part[0] == '-' ? -1 : 1);
        } else {
            return std::nullopt;
        }

        std::chrono::year_month_day ymd{std::chrono::year{year},
                                        std::chrono::month{unsigned(month)},
                                        std::chrono::day{unsigned(day)}};
        if (!ymd.ok()) return std::nullopt;

        auto naive = std::chrono::sys_days{ymd} + std::chrono::hours{hour}
                + std::chrono::minutes{minute} + std::chrono::seconds{second};
        return naive - std::chrono::minutes{offset_minutes};  // shift to true UTC
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
            if (j.contains("id")) e.id = j["id"].get<std::string>();
            e.start = *start;
            e.end = *end;
            return e;
        } catch (const nlohmann::json::exception&) {
            return std::nullopt;
        }
    }

} // namespace calcli