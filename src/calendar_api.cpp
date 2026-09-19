#include "calcli/calendar_api.hpp"
#include "calcli/util.hpp"
#include <nlohmann/json.hpp>

namespace calcli {

    CalendarApi::CalendarApi(AuthManager& auth, IHttpClient& http) : auth_(auth), http_(http) {}

    std::vector<std::string> CalendarApi::auth_headers() {
        return {
            "Authorization: Bearer " + auth_.bearer_token(),
            "Content-Type: application/json"
        };
    }

    std::optional<Event> CalendarApi::create_event(const std::string& calendar_id, const Event& e) {
        std::string url = "https://www.googleapis.com/calendar/v3/calendars/" + calendar_id + "/events";
        auto resp = http_.post(url, auth_headers(), to_json(e));
        if (resp.status_code != 200 && resp.status_code != 201) {
            return std::nullopt;
        }
        return from_json(resp.body);
    }

    std::optional<std::vector<Event>> CalendarApi::list_events(
            const std::string& calendar_id,
            std::chrono::system_clock::time_point from,
            std::chrono::system_clock::time_point to) {
        std::string url = "https://www.googleapis.com/calendar/v3/calendars/" + calendar_id + "/events"
            "?timeMin=" + url_encode(format_rfc3339(from)) +
            "&timeMax=" + url_encode(format_rfc3339(to)) +
            "&singleEvents=true&orderBy=startTime";

        auto resp = http_.get(url, auth_headers());
        if (resp.status_code != 200) return std::nullopt;

        try {
            auto j = nlohmann::json::parse(resp.body);
            std::vector<Event> events;
            for (const auto& item : j.at("items")) {
                if (auto ev = from_json(item.dump())) events.push_back(*ev);
            }
            return events;
        } catch (const nlohmann::json::exception&) {
            return std::nullopt;
        }
    }

    bool CalendarApi::delete_event(const std::string& calendar_id, const std::string& event_id) {
        std::string url = "https://www.googleapis.com/calendar/v3/calendars/" + calendar_id +
                        "/events/" + event_id;
        auto resp = http_.del(url, auth_headers());
        return resp.status_code == 200 || resp.status_code == 204;
    }
}