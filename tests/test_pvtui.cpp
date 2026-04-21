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

    InputWidget x(app, prefix + "Position", "X.value");
    InputWidget y(app, prefix + "Position", "Y.value");
    InputWidget z(app, prefix + "Position", "Z.value");
    Monitor<double> x_rbv(app, prefix + "Position", "X.value");
    Monitor<double> y_rbv(app, prefix + "Position", "Y.value");
    Monitor<double> z_rbv(app, prefix + "Position", "Z.value");

    auto container = Container::Vertical({
        x.component(),
        y.component(),
        z.component(),
    }) | Renderer([&](Element) {
        return vbox({
            separator() | size(WIDTH, EQUAL, 15),
            hbox({
                text("X: "),
                x.component()->Render()
                    | size(WIDTH, EQUAL, 15)
                    | EPICSColor::edit(x),
            }),
            hbox({
                text("Y: "),
                y.component()->Render()
                    | size(WIDTH, EQUAL, 15)
                    | EPICSColor::edit(y),
            }),
            hbox({
                text("Y: "),
                z.component()->Render()
                    | size(WIDTH, EQUAL, 15)
                    | EPICSColor::edit(z),
            }),
            separator() | size(WIDTH, EQUAL, 15),
            hbox({
                text("X RBV: "),
                x_rbv.component()->Render()
                    | size(WIDTH, EQUAL, 15)
                    | EPICSColor::custom(x_rbv, color(Color::SteelBlue)),
            }),
            hbox({
                text("Y RBV: "),
                y_rbv.component()->Render()
                    | size(WIDTH, EQUAL, 15)
                    | EPICSColor::custom(y_rbv, color(Color::SteelBlue)),
            }),
            hbox({
                text("Z RBV: "),
                z_rbv.component()->Render()
                    | size(WIDTH, EQUAL, 15)
                    | EPICSColor::custom(z_rbv, color(Color::SteelBlue)),
            }),
            separator() | size(WIDTH, EQUAL, 15),
        });
    });

    app.run(container);

    return EXIT_SUCCESS;
}
