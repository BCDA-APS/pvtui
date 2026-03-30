#include <cstdlib>
#include <pv/caProvider.h>
#include <pva/client.h>

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/loop.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>

#include <pvtui/pvtui.hpp>

using namespace ftxui;
using namespace pvtui;

static constexpr std::string_view CLI_HELP_MSG = R"(
pvtui_monitor - Terminal UI for monitoring several PVs.

Usage:
    pvtui_monitor [options]

Options:
    -h, --help    Show this help message and exit.
    --prefix      Prefix to prepend to each PV name.

Examples:
    pvtui_monitor --prefix "xxx:" m1.DESC m1.RBV m2.DESC m2.RBV
    pvtui_monitor MyIOC:m1.RBV IOC2:m1.RBV IOC2:m2.RBV IOC3:m1.RBV

For more details, visit: https://github.com/BCDA-APS/pvtui
)";

int main(int argc, char* argv[]) {
    pvtui::App app(argc, argv, {"--prefix"});
    if (app.args.help(CLI_HELP_MSG))
        return EXIT_SUCCESS;

    auto pos_args = app.args.positional_args();
    if (pos_args.size() < 2) {
        std::cout << "Please provide PVs to monitor\n";
        std::cout << CLI_HELP_MSG;
        return 0;
    }
    std::vector<std::string> pv_names(pos_args.begin() + 1, pos_args.end());

    std::string prefix = app.args.param("--prefix");
    std::transform(pv_names.begin(), pv_names.end(), pv_names.begin(), [&](auto& s){
        return prefix + s;
    });

    std::vector<std::unique_ptr<WidgetBase>> widgets;
    if (app.args.flag("edit")) {
        std::cout << "edit mode not implemented\n";
        return 0;
    } else {
        for (auto& name : pv_names) {
            std::cout << name << std::endl;
            auto chan = app.pvgroup[name].channel.get();
            if (chan.get()->getStructure()->getField("value")->getID() == "enum_t") {
                widgets.emplace_back(std::make_unique<Monitor<PVEnum>>(app, name));
            } else {
                widgets.emplace_back(std::make_unique<Monitor<std::string>>(app, name));
            }
        }
    }

    // Force update since the above channel.get() calls prevent the monitors
    // from firing at construction of the widgets
    app.pvgroup.force_update();

    auto main_contianer = Container::Vertical({});
    for (auto& widget : widgets) {
        main_contianer->Add(widget->component());
    }

    auto main_renderer = Renderer(main_contianer, [&] {
        Elements name_col = {text("PV") | bold | italic};
        Elements val_col = {text("Value") | bold | italic};

        for (auto& widget : widgets) {
            name_col.push_back(separator());
            name_col.push_back(text(widget->pv_name()));
            val_col.push_back(separator());
            val_col.push_back(widget->component()->Render() | color(Color::RGB(125, 174, 227)));
        }

        return hbox({
            vbox({
                separator(),
                hbox({
                    vbox(name_col),
                    separator(),
                    vbox(val_col) | size(WIDTH, GREATER_THAN, 20),
                }),
                separator(),
            })
        });
    });

    app.run(main_renderer);
}
