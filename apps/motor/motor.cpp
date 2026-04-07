#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/loop.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>

#include "motor_display.hpp"
#include <pvtui/pvtui.hpp>

using namespace ftxui;
using namespace pvtui;

static constexpr std::string_view CLI_HELP_MSG = R"(
pvtui_motor - Terminal UI for EPICS motor record

Usage:
    pvtui_motor <motor> [<motor>...]
    pvtui_motor [options] <motor> [<motor>...]

Options:
    -h, --help        Show this help message and exit.
    --prefix          Prefix to prepend to each motor name.

Examples:
    pvtui_motor xxx:m1
    pvtui_motor xxx:m1 xxx:m2 xxx:m3
    pvtui_motor --prefix "xxx:" m1 m2 m3

For more details, visit: https://github.com/BCDA-APS/pvtui
)";

struct MotorUIStack {
    Component small;
    Component large;
};

int main(int argc, char* argv[]) {

    pvtui::App app(argc, argv, {"--prefix"});

    if (app.args.help(CLI_HELP_MSG)) {
        return EXIT_SUCCESS;
    }

    auto pos_args = app.args.positional_args();
    if (pos_args.size() < 2) {
        std::cout << CLI_HELP_MSG << std::endl;
        return EXIT_FAILURE;
    }

    // Get all the motor record names (prepending prefix if given)
    std::string prefix = app.args.param("--prefix");
    std::vector<std::string> motor_names;
    for (size_t i = 1; i < pos_args.size(); i++) {
        motor_names.push_back(prefix + pos_args[i]);
    }

    // Create the displays for each motor record
    std::vector<MotorUIStack> displays;
    for (auto& name : motor_names) {
        displays.push_back(MotorUIStack{
            ftxui::Make<SmallMotorDisplay>(app.pvgroup, name),
            ftxui::Make<LargeMotorDisplay>(app.pvgroup, name),
        });
    }

    bool all = true; // Draw SmallMotorDisplay for each motor
    int selected_motor = 0; // Draw this motor's LargeMotorDisplay

    // Container of SmallMotorDisplay's and "more" buttons
    auto all_small_container = Container::Horizontal({});
    for (size_t i = 0; i < displays.size(); i++) {
        auto op = ButtonOption::Ascii();
        op.label = " MORE ";
        op.on_click = [&selected_motor, &all, i]{
            selected_motor = i;
            all = false;
        };
        auto button = Button(op);

        auto panel = Container::Vertical({displays[i].small, button});
        panel |= Renderer([&, i, button](Element){
            return vbox({
                displays[i].small->Render(),
                button->Render() | bgcolor(Color::DarkGreen) | color(Color::White) | center,
            });
        });

        all_small_container->Add(panel);
    }

    // Container of LargeMotorDisplay's and "back" button. Only one selected at a time.
    auto large_container = Container::Tab({}, &selected_motor);
    for (size_t i = 0; i < displays.size(); i++) {
        auto op = ButtonOption::Ascii();
        op.label = " BACK ";
        op.on_click = [&, i]{
            selected_motor = 0;
            all = true;
        };
        auto button = Button(op);

        auto panel = Container::Vertical({displays[i].large, button});
        panel |= Renderer([&, i, button](Element){
            return vbox({
                displays[i].large->Render(),
                button->Render() | bgcolor(Color::DarkGreen) | color(Color::White) | center,
            });
        });
        large_container->Add(panel);
    }

    // Both containers gated by ftxui::Maybe
    auto main_container = Container::Vertical({
        all_small_container | Maybe([&]{return all;}),
        large_container | Maybe([&]{return !all;})
    }) | center | EPICSColor::background();

    app.run(main_container);
}
