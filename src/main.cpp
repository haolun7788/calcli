#include <CLI/CLI.hpp>
#include <iostream>
#include <string>

int main(int argc, char **argv) {
    CLI::App app{"calcli - fast Google Calender management"};
    app.require_subcommand(1);

    std::string title;
    std::string due;
    
    auto* add_cmd = app.add_subcommand("add", "Add new event");
    add_cmd->add_option("title", title, "Event title")->required();
    add_cmd->add_option("--due", due, "Due date/time (natural language)")->required();

    CLI11_PARSE(app, argc, argv);

    if (*add_cmd) {
        std::cout << "Would add event: \"" << title
                    << "\" due \"" << due << "\"\n";
    }

    return 0;
}
