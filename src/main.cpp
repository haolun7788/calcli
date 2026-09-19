#include <CLI/CLI.hpp>
#include <iostream>
#include <string>
#include <calcli/parser.hpp>
#include <calcli/event.hpp>

int main(int argc, char **argv) {
    CLI::App app{"calcli - fast Google Calender management"};
    app.require_subcommand(1);

    std::string title;
    std::vector<std::string> when_tokens;
    std::string location;
    
    auto* add_cmd = app.add_subcommand("add", "Add new event");
    add_cmd->add_option("title", title, "Event title")->required();
    add_cmd->add_option("when", when_tokens,
        "Natural-language day/time/duration, e.g. friday 2:30 1 hour")->required();
    add_cmd->add_option("--location,-l", location, "Event location");

    CLI11_PARSE(app, argc, argv);

    if (*add_cmd) {
        auto result = calcli::parse_when(when_tokens, std::chrono::system_clock::now());
        if (!result) {
            std::cerr << "Error: couldn't understand the date/time/duration. \n";
            return 1;
        }
        std::cout << "Would add event: \"" << title << "\"\n"
                  << " Start: " << std::chrono::floor<std::chrono::seconds>(result->start) << "\n"
                  << " End: " << std::chrono::floor<std::chrono::seconds>(result->end) << "\n";
        
        if  (!location.empty()) {
            std::cout << " Location: " << location << "\n";
        }
    }

    return 0;
}
