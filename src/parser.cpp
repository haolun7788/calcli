#include "calcli/parser.hpp"
#include <cctype>
#include <charconv>
#include <ctime>

namespace calcli {

    std::chrono::system_clock::time_point local_to_utc(int year, int month, int day,int hour, int minute, int second) {
        std::tm tm{};
        tm.tm_year = year - 1900;
        tm.tm_mon = month - 1;
        tm.tm_mday = day;
        tm.tm_hour = hour;
        tm.tm_min = minute;
        tm.tm_sec = second;
        tm.tm_isdst = -1;  
        std::time_t t = std::mktime(&tm);  // interprets tm as LOCAL time, returns UTC-based epoch
        return std::chrono::system_clock::from_time_t(t);
    }

    std::optional<std::chrono::weekday> parse_weekday(const std::string& token) {
        std::string lower_token = token;
        for (char& c : lower_token) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        if (lower_token == "monday") return std::chrono::Monday;
        if (lower_token == "tuesday") return std::chrono::Tuesday;
        if (lower_token == "wednesday") return std::chrono::Wednesday;
        if (lower_token == "thursday") return std::chrono::Thursday;
        if (lower_token == "friday") return std::chrono::Friday;
        if (lower_token == "saturday") return std::chrono::Saturday;
        if (lower_token == "sunday") return std::chrono::Sunday;
        return std::nullopt;
    }

    std::optional<std::pair<int, int>> parse_clock_time(const std::string& token) { // {hour, minute}
        auto colon_pos = token.find(':');
        // TODO
        // no bare-number time support for now
        if (colon_pos == std::string::npos) {
            return std::nullopt;  // no bare-number time support for now
        }
        // no bare-number time support for now

        std::pair<int, int> time_pair{0, 0};
        if (token.find(':') != std::string::npos) {
            size_t colon_pos = token.find(':');
            std::string hour_str = token.substr(0, colon_pos);
            std::string minute_str = token.substr(colon_pos + 1);
            try {
                time_pair.first = std::stoi(hour_str);
                time_pair.second = std::stoi(minute_str);
                if (time_pair.first < 0 || time_pair.first > 23 || time_pair.second < 0 || time_pair.second > 59) {
                    return std::nullopt;
                }
                return time_pair;
            } catch (...) {
                return std::nullopt;
            }
        } else {
            std::optional<int> num = parse_number(token);
            if (!num) {
                return std::nullopt;
            }
            int hour = *num / 100;
            int minute = *num % 100;
            if (hour < 0 || hour > 23 || minute < 0 || minute > 59) {
                return std::nullopt;
            }
            time_pair.first = hour;
            time_pair.second = minute;
            return time_pair;
        }
        return std::nullopt;
    }

    std::optional<int> parse_number(const std::string& token) {
        int out = 0;
        auto result = std::from_chars(token.data(), token.data() + token.size(), out);
        if (result.ec != std::errc() || result.ptr != token.data() + token.size()) {
            return std::nullopt;
        }
        return out;
    }

    std::optional<std::chrono::minutes> parse_duration(int amount, const std::string& unit_token) {
        if (unit_token == "minute" || unit_token == "minutes") {
            return std::chrono::minutes{amount};
        } else if (unit_token == "hour" || unit_token == "hours") {
            return std::chrono::minutes{amount * 60};
        } else if (unit_token == "day" || unit_token == "days") {
            return std::chrono::minutes{amount * 24 * 60};
        }
        return std::nullopt;
    }

    std::optional<ParsedWhen> parse_when(const std::vector<std::string>& tokens, std::chrono::system_clock::time_point now) {
        std::optional<std::chrono::sys_days> resolved_date;
        std::optional<std::pair<int, int>> resolved_time;
        std::optional<int> pending_amount;      // number seen, waiting for a unit token
        std::optional<std::chrono::minutes> resolved_duration;

        auto today = std::chrono::floor<std::chrono::days>(now);
        std::chrono::weekday today_weekday{today};

        for (const auto& token : tokens) {
            if (auto wd = parse_weekday(token)) {
                int diff = (wd->c_encoding() - today_weekday.c_encoding() + 7) % 7;
                resolved_date = today + std::chrono::days{diff};
            } else if (auto ct = parse_clock_time(token)) {
                resolved_time = ct;
            } else if (pending_amount) {
                resolved_duration = parse_duration(*pending_amount, token);
                if (!resolved_duration) return std::nullopt;
                pending_amount.reset();
            } else if (auto num = parse_number(token)) {
                pending_amount = num;
            } else {
                return std::nullopt;  // unrecognized token
            }
        }

        if (!resolved_date || !resolved_time || !resolved_duration) {
            return std::nullopt;
        }
        
        resolved_time->first = (resolved_time->first < 12) ? resolved_time->first + 12 : resolved_time->first; // PM default
        std::chrono::year_month_day ymd{*resolved_date};
        int hour = resolved_time->first;
        if (hour < 12) hour += 12;

        auto start = local_to_utc(int(ymd.year()), unsigned(ymd.month()), unsigned(ymd.day()),
                                hour, resolved_time->second, 0);
        auto end = start + *resolved_duration;


        return ParsedWhen{start, end};
    }

} // namespace calcli