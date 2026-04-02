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

    std::string prefix = app.args.param("--prefix");

    std::vector<std::string> motor_names;
    for (size_t i = 1; i < pos_args.size(); i++) {
        motor_names.push_back(prefix + pos_args[i]);
    }

    std::vector<std::unique_ptr<DisplayBase>> displays;
    for (auto& name : motor_names) {
        displays.emplace_back(std::make_unique<SmallMotorDisplay>(app.pvgroup, name));
        displays.emplace_back(std::make_unique<LargeMotorDisplay>(app.pvgroup, name));
    }

    int selected_view = 0;

    Components buttons;
    std::string button_label_more = " MORE ";
    std::string button_label_top  = " BACK  ";
    for (size_t i = 0; i < displays.size(); i++) {
        auto opt = ButtonOption::Ascii();
        opt.label = (i % 2) ? button_label_top : button_label_more;
        opt.on_click = [&selected_view, i]{
            if ((i % 2) == 0) {
                selected_view = i+1;
            } else {
                selected_view = 0;
            }
        };
        buttons.push_back(Button(opt));
    }

    Component main_container = Container::Horizontal({});
    for (size_t i = 0; i < displays.size(); i++) {
        main_container->Add(displays[i]->get_container() | Maybe([&selected_view, i] {
            if ((i%2) == 0) {
                return selected_view == 0;
            } else {
                return selected_view == i;
            }}));
        main_container->Add(buttons[i]);
    }

    Component main_renderer = Renderer(main_container, [&]{

        Elements elements;

        if (selected_view == 0) {
            // view 0 means show all SmallMotorDisplays's aranged horizontally
            for (size_t i = 0; i < displays.size(); i++) {
                if ((i % 2) == 0) {
                    elements.push_back(vbox({
                        displays[i]->get_renderer(),
                        hbox({
                            buttons[i]->Render()
                                | color(Color::White) | bgcolor(Color::DarkGreen)
                                | size(WIDTH, EQUAL, 8)
                        }) | center
                    }));
                }
            }
        } else {
            // Show just the seleted LargeMotorDisplay
            elements.push_back(vbox({
                displays[selected_view]->get_renderer(),
                hbox({
                    buttons[selected_view]->Render()
                        | color(Color::White) | bgcolor(Color::DarkGreen)
                        | size(WIDTH, EQUAL, 8)
                })
            }));
        }
        return hbox({elements}) | center | EPICSColor::background();
    });

    app.run(main_renderer);
}
