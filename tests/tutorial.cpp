#include <pvtui/pvtui.hpp>
#include <ftxui/component/component.hpp>

using namespace ftxui;
using namespace pvtui;

int main(int argc, char *argv[]) {

    pvtui::App app(argc, argv);

    auto pos_args = app.args.positional_args();
    if (pos_args.size() < 2) {
        printf("Usage: tutorial <prefix>\n");
        return EXIT_FAILURE;
    }
    const std::string prefix = pos_args[1];

    InputWidget desc(app, prefix + "Value.DESC");
    InputWidget val(app, prefix + "Value.VAL");
    InputWidget twv(app, prefix + "TweakVal.VAL");
    ButtonWidget twf(app, prefix + "TweakFwd.PROC", " + ");
    ButtonWidget twr(app, prefix + "TweakRev.PROC", " - ");

    auto top_container = Container::Vertical({
        desc.component(),
        val.component(),
    }) | Renderer([&](Element) {
        return vbox({
            hbox({
                text("DESC: "),
                desc.component()->Render()
                    | size(WIDTH, EQUAL, 15)
                    | EPICSColor::edit(desc),
            }),
            separator(),
            hbox({
                text("VAL: "),
                val.component()->Render()
                    | size(WIDTH, EQUAL, 11)
                    | EPICSColor::edit(val),
            }),
        });
    });

    auto tweak_container = Container::Horizontal({
        twr.component(),
        twv.component(),
        twf.component(),
    }) | Renderer([&](Element) {
        return hbox({
            twr.component()->Render(),
            separatorEmpty(),
            twv.component()->Render()
                | size(WIDTH, EQUAL, 5)
                | EPICSColor::edit(twv),
            separatorEmpty(),
            twf.component()->Render(),
        });
    });

    auto container = Container::Vertical({
        top_container,
        tweak_container,
    }) | Renderer([&](Element) {
        return vbox({
            top_container->Render(),
            separatorEmpty(),
            tweak_container->Render(),
        }) | size(WIDTH, EQUAL, 20);
    });

    app.run(container);

    return EXIT_SUCCESS;
}
