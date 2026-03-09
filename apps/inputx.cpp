#include <cstdlib>
#include <pv/caProvider.h>
#include <pva/client.h>

#include <ftxui/component/component.hpp>
#include <ftxui/component/loop.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>

#include <pvtui/pvtui.hpp>

using namespace ftxui;
using namespace pvtui;

static constexpr std::string_view CLI_HELP_MSG = R"(
pvtui_inputx - Terminal UI for x number of PV input/readbacks.

Usage:
  pvtui_inputx [options]

Options:
  -h, --help        Show this help message and exit.
  -m, --macro       Macros to pass to the UI

Examples:
    # Make a screen with a input and readack component for each given PV
    pvtui_inputx --macro "P=xxx:,R1=m1.DESC,R2=m2.DESC,R2=m3.DESC"

    # Pass desired input/readback PV pairs separated by "/"
    # The below will make a screen with an input field for xxx:m1.VAL
    # and readback widget for xxx:m1.RBV, and the same for m2.VAL and m2.RBV
    pvtui_inputx --macro "P=xxx:,R1=m1.VAL/m1.RBV,R2=m2.VAL/m2.RBV"

For more details, visit: https://github.com/nmarks99/pvtui
)";

int main(int argc, char *argv[]) {

    // Parse command line arguments and macros
    pvtui::ArgParser args(argc, argv);

    if (args.help(CLI_HELP_MSG)) return EXIT_SUCCESS;

    if (args.macros.empty()) {
        printf("Missing required macros\n");
        return EXIT_FAILURE;
    }

    // Prefix is optional
    std::string ioc_prefix = "";
    if (args.macros.count("P") > 0) {
        ioc_prefix = args.macros.at("P");
    }

    // Get PV names for set and readback PVs
    std::vector<std::string> val_pvs;
    std::vector<std::string> rbv_pvs;
    for (const auto &[k, v] : args.macros) {
        if (k == "P") {
            continue;
        }
        std::string val_pv_name = "";
        std::string rbv_pv_name = "";
        if (auto ind = v.find("/"); ind != std::string::npos) {
            val_pv_name = ioc_prefix + v.substr(0, ind);
            rbv_pv_name = ioc_prefix + v.substr(ind+1, v.size());
        } else {
            val_pv_name = ioc_prefix + v;
            rbv_pv_name = ioc_prefix + v;
        }
        val_pvs.push_back(val_pv_name);
        rbv_pvs.push_back(rbv_pv_name);
    }

    // Create the FTXUI screen. Interactive and uses the full terminal screen
    auto screen = ScreenInteractive::Fullscreen();

    // Instantiate EPICS PVA client
    // Start CAClientFactory so we can see CA only PVs
    epics::pvAccess::ca::CAClientFactory::start();
    pvac::ClientProvider provider(args.provider);

    // PVGroup to manage all PVs for displays
    PVGroup pvgroup(provider);

    // Create input widgets for the set PVs and Monitor<std::string> for the readback PVs
    // We use strings for everything here because it should work for most (all?) PV types
    std::vector<std::unique_ptr<InputWidget>> val_widgets;
    std::vector<std::unique_ptr<Monitor<std::string>>> rbv_widgets;
    for (size_t i = 0; i < val_pvs.size(); i++) {
        val_widgets.emplace_back(std::make_unique<InputWidget>(pvgroup, val_pvs.at(i), PVPutType::String));
        rbv_widgets.emplace_back(std::make_unique<Monitor<std::string>>(pvgroup, rbv_pvs.at(i)));
    }

    // Add components to a main container.
    // Remember Monitor doesn't define a component
    auto main_container = Container::Vertical({});
    for (size_t i = 0; i < val_pvs.size(); i++) {
        auto hcont = Container::Horizontal({
            val_widgets.at(i)->component(),
        });
        main_container->Add(hcont);
    }

    // Define the visual layout of components in the main renderer
    auto main_renderer = Renderer(main_container, [&] {
        Elements elements;
        for (size_t i = 0; i < val_pvs.size(); i++) {
            auto &val = val_widgets.at(i);
            auto &rbv = rbv_widgets.at(i);
            auto h = vbox({
            hbox({
                text(rbv->pv_name()) | color(Color::Black),
                separatorEmpty(),
                text(rbv->value())
                | EPICSColor::readback(*rbv)
                | size(WIDTH, EQUAL, 20),
            }),
            hbox({
                text(val->pv_name()) | color(Color::Black),
                separatorEmpty(),
                val->component()->Render()
                | EPICSColor::edit(*val)
                | size(WIDTH, EQUAL, 20),
            })
            });
            elements.push_back(h);
            elements.push_back(separatorEmpty());
        }

        return vbox({
            elements,
        }) | center | EPICSColor::background();
    });

    // main program loop
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
