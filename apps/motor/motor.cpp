#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/loop.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>

#include "motor_display.hpp"
#include <thread>
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

    bool display_multi = motor_names.size() > 1;

    std::vector<std::unique_ptr<DisplayBase>> displays;
    if (display_multi) {
        for (const auto& name : motor_names) {
            displays.emplace_back(std::make_unique<SmallMotorDisplay>(app.pvgroup, name));
        }
    } else {
        displays.emplace_back(std::make_unique<SmallMotorDisplay>(app.pvgroup, motor_names[0]));
        displays.emplace_back(std::make_unique<MediumMotorDisplay>(app.pvgroup, motor_names[0]));
        displays.emplace_back(std::make_unique<AllMotorDisplay>(app.pvgroup, motor_names[0]));
    }

    int selected_tab = 0;
    std::vector<std::string> labels = {"Small", "Medium", "All"};
    auto dropdown_op = ftxui::DropdownOption();
    dropdown_op.radiobox.entries = &labels;
    dropdown_op.radiobox.selected = &selected_tab;
    dropdown_op.transform = [](bool open, ftxui::Element checkbox, ftxui::Element radiobox) {
        if (open) {
            return ftxui::vbox({
                checkbox | inverted,
                radiobox | vscroll_indicator | frame | size(HEIGHT, LESS_THAN, 10),
                filler(),
            });
        }
        return vbox({
            checkbox,
            filler(),
        });
    };

    ftxui::Component main_container;
    ftxui::Component main_renderer;

    if (display_multi) {
        main_container = ftxui::Container::Horizontal({});
        for (auto& display : displays) {
            main_container->Add(display->get_container());
        }
        main_renderer = ftxui::Renderer(main_container, [&] {
            Elements elements;
            for (auto& display : displays) {
                elements.push_back(display->get_renderer());
            }
            return hbox({elements}) | center | EPICSColor::background();
        });

    } else {
        auto tab_selector = ftxui::Dropdown(dropdown_op);

        ftxui::Components tabs;
        for (auto& display : displays) {
            tabs.push_back(display->get_container());
        }

        main_container = ftxui::Container::Vertical({
            ftxui::Container::Tab({tabs}, &selected_tab),
            tab_selector
        });

        main_renderer = ftxui::Renderer(main_container, [&] {
            return vbox({
                displays.at(selected_tab)->get_renderer(),
                tab_selector->Render()
                    | color(Color::White)
                    | bgcolor(Color::DarkGreen)
                    | size(WIDTH, EQUAL, 7)
            }) | center | pvtui::EPICSColor::background();
        });
    }

    app.run(main_renderer);
}
