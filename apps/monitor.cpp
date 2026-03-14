#include <cstdlib>
#include <pv/caProvider.h>
#include <pva/client.h>

#include <ftxui/component/component.hpp>
#include <ftxui/component/loop.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>
#include <chrono>

#include <pvtui/pvtui.hpp>

using namespace ftxui;
using namespace pvtui;

static constexpr std::string_view CLI_HELP_MSG = R"(
pvtui_monitor - Terminal UI for monitoring several PVs.

Usage:
  pvtui_monitor [options]

Options:
  -h, --help        Show this help message and exit.
  -m, --macro       Macros to pass to the UI

Examples:
    #  Optional prefix followed by list of PVs to monitor
    pvtui_monitor --macro "P=xxx:" m1.DESC m1.RBV m2.DESC m2.RBV

For more details, visit: https://github.com/BCDA-APS/pvtui
)";

int main(int argc, char *argv[]) {
    App app(argc, argv);
    if (app.args.help(CLI_HELP_MSG)) return EXIT_SUCCESS;

    auto pos_args = app.args.positional_args();
    if (pos_args.size() < 2) {
        std::cout << "Please provide PVs to monitor\n";
        std::cout << CLI_HELP_MSG;
        return 0;
    }
    std::vector<std::string> pv_names(pos_args.begin()+1, pos_args.end());

    // add prefix to PV names if P macro given
    if (app.args.macros_present({"P"})) {
        const std::string prefix = app.args.macros.at("P");
        std::transform(pv_names.begin(), pv_names.end(), pv_names.begin(), [&](auto& s){
            return prefix + s;
        });
    }

    // std::vector<std::unique_ptr<WidgetBase>> widgets;
    std::vector<std::unique_ptr<WidgetBase>> widgets;
    if (app.args.flag("edit")) {
        std::cout << "edit mode not implemented\n";
        return 0;
    } else {
        for (auto& name : pv_names) {
            auto chan = app.provider.connect(name);
            if (auto type_str = chan.get()->getStructure()->getField("value")->getID(); type_str == "enum_t") {
                widgets.emplace_back(std::make_unique<Monitor<PVEnum>>(app, name));
            } else {
                widgets.emplace_back(std::make_unique<Monitor<std::string>>(app, name));
            }
            chan.reset();
            // // FIX: this should be doable with PVGroup!
            // // Doing a get() before constructing the Widgets makes the widget values
            // // empty until the monitors fire again for some reason?
            // app.pvgroup.add(name);
            // // Monitor everything as a string, unless its an enum then we use PVEnum.
            // if (auto pstruct = app.pvgroup[name].channel.get()->getStructure()) {
                // if (pstruct->getField("value")->getID() == "enum_t") {
                    // widgets.emplace_back(std::make_unique<Monitor<PVEnum>>(app, name));
                // } else {
                    // widgets.emplace_back(std::make_unique<Monitor<std::string>>(app, name));
                // }
            // }
        }
    }

    auto main_contianer = Container::Vertical({});
    for (auto& widget : widgets) {
        main_contianer->Add(widget->component());
    }

    const auto longest = std::max_element(pv_names.begin(),
            pv_names.end(), [](const std::string& a, const std::string& b){
        return a.length() < b.length();
    });

    auto main_renderer = Renderer(main_contianer, [&] {
        Elements rows {
            hbox({
                text("PV")  | size(WIDTH, EQUAL, longest->size()) | bold | italic,
                separator(),
                text("Value") | bold | italic,
            }),
            separator()
        };
        for (auto& widget : widgets) {
            rows.push_back(hbox({
                text(widget->pv_name()) | size(WIDTH, EQUAL, longest->size()),
                separator(),
                widget->component()->Render() | color(Color::RGB(125, 174, 227))
            }));
            rows.push_back(separator());
        }
        return vbox({
            separator(),
            vbox({
                rows
            }),
        }) | size(WIDTH, EQUAL, 50);
    });

    app.run(main_renderer);

}
