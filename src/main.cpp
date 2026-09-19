#include <CLI/CLI.hpp>
#include <iostream>
#include <string>

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
        std::string when_str;
        for (size_t i = 0; i < when_tokens.size(); ++i) {
            if (i) when_str += " ";
            when_str += when_tokens[i];
        }
        std::cout << "Would add event: \"" << title << "\"\n"
                  << "  when: \"" << when_str << "\"\n";
        if (!location.empty())
            std::cout << "  location: \"" << location << "\"\n";
    }

    return 0;
}
