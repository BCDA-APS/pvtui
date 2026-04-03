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
    std::vector<std::shared_ptr<MotorDisplay>> displays;
    for (auto& name : motor_names) {
        displays.push_back(std::make_shared<MotorDisplay>(app.pvgroup, name));
    }


    int selected_motor = -1; // -1 = all

    auto main_container = Container::Horizontal({});

    std::string button_label = " MORE ";

    // Buttons to switch between small, large, and all views
    std::vector<Component> view_buttons;
    for (size_t i = 0; i < displays.size(); i++) {
        auto op = ButtonOption::Ascii();
        op.label = &button_label;
        op.on_click = [&, i] {
            if (selected_motor == i) {
                // this means we currently we are on the large view
                // for displays[i] and want to go back to all view.
                button_label = " MORE ";
                displays[i]->view = 0;
                selected_motor = -1;
            } else {
                // we must be in the all screen and want to go to a
                // specific large motor screen
                button_label = " BACK ";
                selected_motor = -1;
                displays[i]->view = 1;
                selected_motor = i;
            }
        };
        auto button = Button(op);
        view_buttons.push_back(button | bgcolor(Color::DarkGreen) | color(Color::White));

        main_container->Add(displays[i] | Maybe([&, i] {
            return selected_motor == -1 || selected_motor == (int)i;
        }));
        main_container->Add(button | Maybe([&, i] {
            return selected_motor == -1 || selected_motor == (int)i;
        }));

    }

    auto main_renderer = Renderer(main_container, [&] {
        if (selected_motor == -1) {
            Elements panels;
            for (size_t i = 0; i < displays.size(); i++) {
                panels.push_back(vbox({
                    displays[i]->Render(),
                    view_buttons[i]->Render() | center,
                }));
            }
            return hbox(panels) | center | EPICSColor::background();
        } else {
            return vbox({
                displays[selected_motor]->Render(),
                view_buttons[selected_motor]->Render() | center,
            }) | center | EPICSColor::background();
        }
    });

    app.run(main_renderer);
}
