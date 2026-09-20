#include <catch2/catch_test_macros.hpp>
#include "calcli/parser.hpp"
#include <chrono>

using namespace std::chrono;
using namespace calcli;

namespace {

    // Wednesday, Sept 16 2026, 10:00 AM — fixed reference point for all tests
    system_clock::time_point fixed_now() {
        return sys_days{2026y / September / 16} + hours{10};
    }
 
    // Force UTC timezone
    struct ForceUtcTimezone {
    ForceUtcTimezone() {
    #if defined(_WIN32)
            _putenv_s("TZ", "UTC");
            _tzset();
    #else
            setenv("TZ", "UTC", 1);
            tzset();
    #endif
        }
    };
    ForceUtcTimezone force_utc_once;
} // namespace

TEST_CASE("weekday -> parse_weekday", "[parser]") {
    auto result = parse_weekday("friday");
    REQUIRE(result.has_value());

    auto expected = std::chrono::Friday;
    REQUIRE(result.value() == expected);
}

TEST_CASE("case insensitive: weekday -> parse_weekday", "[parser]") {
    auto lower = parse_weekday("friday");
    auto rand = parse_weekday("fRiDaY");
    auto upper = parse_weekday("FRIDAY");
    REQUIRE(lower.has_value());
    REQUIRE(rand.has_value());
    REQUIRE(upper.has_value());

    auto expected = std::chrono::Friday;
    REQUIRE(lower.value() == expected);
    REQUIRE(rand.value() == expected);
    REQUIRE(upper.value() == expected);
}
    
TEST_CASE("weekday + time + hour duration, bare time defaults to PM", "[parser]") {
    auto result = parse_when({"friday", "2:30", "1", "hour"}, fixed_now());
    REQUIRE(result.has_value());

    auto expected_start = sys_days{2026y / September / 18} + hours{14} + minutes{30};
    auto expected_end   = expected_start + hours{1};

    REQUIRE(result->start == expected_start);
    REQUIRE(result->end   == expected_end);
}

TEST_CASE("plural duration unit parses like singular", "[parser]") {
    auto result = parse_when({"friday", "2:30", "2", "hours"}, fixed_now());
    REQUIRE(result.has_value());
    REQUIRE(result->end - result->start == hours{2});
}

TEST_CASE("weekday name is case-insensitive", "[parser]") {
    auto lower = parse_when({"friday", "2:30", "1", "hour"}, fixed_now());
    auto upper = parse_when({"Friday", "2:30", "1", "hour"}, fixed_now());
    REQUIRE(lower.has_value());
    REQUIRE(upper.has_value());
    REQUIRE(lower->start == upper->start);
}

TEST_CASE("day already passed this week rolls to next week", "[parser]") {
    // now is Saturday, Sept 19 2026 — "friday" should NOT mean 6 days ago
    auto saturday_now = sys_days{2026y / September / 19} + hours{10};
    auto result = parse_when({"friday", "2:30", "1", "hour"}, saturday_now);
    REQUIRE(result.has_value());

    auto expected_start = sys_days{2026y / September / 25} + hours{14} + minutes{30};
    REQUIRE(result->start == expected_start);
}

TEST_CASE("unrecognized token is rejected", "[parser]") {
    auto result = parse_when({"blorp", "2:30", "1", "hour"}, fixed_now());
    REQUIRE_FALSE(result.has_value());
}

TEST_CASE("missing duration is rejected", "[parser]") {
    auto result = parse_when({"friday", "2:30"}, fixed_now());
    REQUIRE_FALSE(result.has_value());
}