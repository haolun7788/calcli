#pragma once
#include "calcli/auth.hpp"
#include "calcli/http_client.hpp"
#include "calcli/event.hpp"
#include <vector>
#include <optional>
#include <chrono>

namespace calcli {

class CalendarApi {
    public:
        CalendarApi(AuthManager& auth, IHttpClient& http);

        std::optional<Event> create_event(const std::string& calendar_id, const Event& e);
        std::optional<std::vector<Event>> list_events(const std::string& calendar_id,
                                                    std::chrono::system_clock::time_point from,
                                                    std::chrono::system_clock::time_point to);
        bool delete_event(const std::string& calendar_id, const std::string& event_id);

    private:
        AuthManager& auth_;
        IHttpClient& http_;
        std::vector<std::string> auth_headers();
    };
} // namespace calcli