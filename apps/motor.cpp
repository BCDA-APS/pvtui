#include <charconv>

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
Inspired by MEDM motor record screens.

Usage:
  pvtui_motor [options]

Options:
  -h, --help        Show this help message and exit.
  -m, --macro       Macros to pass to the UI (required: P, M, or M1,M2,...)

Examples:
    # start screen for xxx:m1
    pvtui_motor --macro "P=xxx:,M=m1"

    # start motor3x.adl style screen for motors xxx:m1-m3
    pvtui_motor --macro "P=xxx:,M1=m1,M2=m2,M3=m3"

For more details, visit: https://github.com/nmarks99/pvtui
)";

enum class MotorDisplayType {
    Small,
    Medium,
    All,
    Multi,
};

int main(int argc, char* argv[]) {

    // Parse command line arguments and macros
    pvtui::ArgParser args(argc, argv);

    if (args.help(CLI_HELP_MSG))
        return EXIT_SUCCESS;

    if (not args.macros_present({"P"})) {
        printf("Missing required macro P\n");
        return EXIT_FAILURE;
    }

    bool display_multi = false;

    // If M1 macro present, assume Mutli display type
    // otherwise assume single and M macro must be present
    if (args.macros_present({"M1"})) {
        // display_type = MotorDisplayType::Multi;
        display_multi = true;
    } else if (not args.macros_present({"M"})) {
        printf("Missing required macro M or (M1,M2,...)\n");
        return EXIT_FAILURE;
    }

    // Create the FTXUI screen. Interactive and uses the full terminal screen
    auto screen = ScreenInteractive::Fullscreen();

    // Instantiate EPICS PVA client and CAClientFactory to see CA only PVs
    epics::pvAccess::ca::CAClientFactory::start();
    pvac::ClientProvider provider(args.provider);

    // unique_ptr's to DisplayBase for each screen
    std::vector<std::unique_ptr<DisplayBase>> displays;

    // PVGroup to manage all PVs for displays
    PVGroup pvgroup(provider);

    // multi display creates a SmallMotorDisplay for each Mn macro where n is an integer.
    // The resulting screen is similar to motorNx.adl
    std::vector<int> motor_num_vec;
    std::vector<ArgParser> args_vec;
    if (display_multi) {
        for (const auto& [k, v] : args.macros) {
            const size_t ind = k.find("M");
            if (ind != std::string::npos) {
                const std::string num_str = std::string(k.begin() + ind + 1, k.end());
                int num;
                if (std::from_chars(num_str.data(), num_str.data() + num_str.size(), num).ec != std::errc()) {
                    throw std::runtime_error("Invalid macro " + k);
                }
                motor_num_vec.push_back(num);
            }
        }
        std::sort(motor_num_vec.begin(), motor_num_vec.end());
        for (const auto& v : motor_num_vec) {
            auto args_n = args;
            args_n.macros["M"] = args_n.macros.at("M" + std::to_string(v));
            args_vec.push_back(args_n);
            displays.emplace_back(std::make_unique<SmallMotorDisplay>(pvgroup, args_n));
        }
    } else {
        displays.emplace_back(std::make_unique<SmallMotorDisplay>(pvgroup, args));
        displays.emplace_back(std::make_unique<MediumMotorDisplay>(pvgroup, args));
        displays.emplace_back(std::make_unique<AllMotorDisplay>(pvgroup, args));
    }

    int selected = 0;
    std::vector<std::string> labels = {"Small", "Medium", "All"};
    auto dropdown_op = ftxui::DropdownOption();
    dropdown_op.radiobox.entries = &labels;
    dropdown_op.radiobox.selected = &selected;
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
        auto view_select = ftxui::Dropdown(dropdown_op);
        ftxui::Components tabs;
        for (auto& display : displays) {
            tabs.push_back(display->get_container());
        }
        main_container = ftxui::Container::Vertical({ftxui::Container::Tab({tabs}, &selected), view_select});

        main_renderer = ftxui::Renderer(main_container, [&] {
            Elements elements;
            elements.push_back(displays.at(selected)->get_renderer());
            elements.push_back(view_select->Render() | color(Color::White) | bgcolor(Color::DarkGreen) |
                               size(WIDTH, EQUAL, 7));
            return vbox({
                elements,
            }) | center | pvtui::EPICSColor::background();
        });
    }

    constexpr int POLL_PERIOD_MS = 100;
    Loop loop(&screen, main_renderer);
    while (!loop.HasQuitted()) {
        if (pvgroup.sync()) {
            screen.PostEvent(Event::Custom);
        }
        loop.RunOnce();
        std::this_thread::sleep_for(std::chrono::milliseconds(POLL_PERIOD_MS));
    }
}
