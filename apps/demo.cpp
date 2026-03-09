#include <ftxui/component/component.hpp>
#include <pvtui/pvtui.hpp>

const std::string PVTUI_TEXT = R"(
██████╗ ██╗   ██╗████████╗██╗   ██╗██╗
██╔══██╗██║   ██║╚══██╔══╝██║   ██║██║
██████╔╝██║   ██║   ██║   ██║   ██║██║
██╔═══╝ ╚██╗ ██╔╝   ██║   ██║   ██║██║
██║      ╚████╔╝    ██║   ╚██████╔╝██║
╚═╝       ╚═══╝     ╚═╝    ╚═════╝ ╚═╝
)";

static constexpr std::string_view CLI_HELP_MSG = R"(
pvtui_demo - Demonstrates some of the EPICS aware widgets in PVTUI.
Note: This demo application uses PVs loaded by the example IOC: tests/xxx
which uses ${USER} environment variable as IOC prefix.

Usage:
  pvtui_demo [options]

Options:
  -h, --help        Show this help message and exit.
  -m, --macro       Macros to pass to the UI (required: P)

Examples:
    pvtui_demo --macro "P=${USER}:"

For more details, visit https://github.com/BCDA-APS/pvtui
)";

using namespace ftxui;
using namespace pvtui;

int main(int argc, char* argv[]) {

    // Create an App which manages PVs and macros
    App app(argc, argv);

    // Show help message and return if requested
    if (app.args.help(CLI_HELP_MSG)) return EXIT_SUCCESS;

    // Return if not all required macros are given.
    if (not app.args.macros_present({"P"})) {
        printf("Missing required macros\nRequired macros: P\n");
        return EXIT_FAILURE;
    }

    // Create all the widgets we want for this display
    InputWidget inp1(app, "$(P)string.VAL", PVPutType::String, Color::Black, Color::White);
    ButtonWidget plus_button(app, "$(P)add1.PROC", " + ");
    ButtonWidget minus_button(app, "$(P)subtract1.PROC", " - ");
    Monitor<int> int_val(app, "$(P)long.VAL");
    ChoiceWidget enum_h(app, "$(P)enum.VAL", ChoiceStyle::Horizontal);
    ChoiceWidget enum_v(app, "$(P)enum.VAL", ChoiceStyle::Vertical);
    ChoiceWidget enum_d(app, "$(P)enum.VAL", ChoiceStyle::Dropdown);

    // BitsWidget displays an integer's individual bits
    size_t nbits = 8;
    BitsWidget bits(app, "$(P)int8.VAL", nbits);
    Elements labs;
    for (size_t i = 0; i < nbits; i++) {
        labs.push_back(text(std::to_string(i) + ":") | color(Color::White));
    }
    auto bit_labels = vbox(labs);

    // ftxui container to define interactivity of components
    // Only interactive components need to be included here.
    auto main_container = Container::Vertical({
        inp1.component(),
        plus_button.component(),
        minus_button.component(),
        enum_h.component(),
        enum_v.component(),
        enum_d.component(),
    });

    // ftxui renderer defines the visual layout of components
    auto main_renderer = Renderer(main_container, [&]{
        auto row1 = hbox({
            text("Input      ") | color(Color::White),
            separator(),
            inp1.component()->Render() | bgcolor(Color::GrayLight) | xflex
        });

        auto row2 = hbox({
            vbox({text("Buttons    ")}) | center,
            separator(),
            hbox({
                plus_button.component()->Render(),
                separatorEmpty(),
                minus_button.component()->Render(),
            })
        });

        auto row3 = hbox({
            vbox({text("Readback   ")}) | center, separator(),
            int_val.component()->Render() | color(Color::LightSlateBlue)
        });

        auto row4 = hbox({
            vbox({text("Menus      ") | color(Color::White)}) | center,
            separator(),
            hbox({
                vbox({
                    separatorEmpty(),
                    enum_h.component()->Render() | color(Color::White) | bgcolor(Color::DarkGreen),
                    separatorEmpty(),
                }),
                separatorEmpty(),
                separatorEmpty(),
                vbox({
                    separatorEmpty(),
                    enum_v.component()->Render() | color(Color::White) | bgcolor(Color::DarkGreen),
                    separatorEmpty(),
                }),
                separatorEmpty(),
                separatorEmpty(),
                vbox({
                    separatorEmpty(),
                    enum_d.component()->Render() | color(Color::White) | bgcolor(Color::DarkGreen),
                    separatorEmpty(),
                }),
            }) | center | size(HEIGHT, EQUAL, 5),
        });

        auto row5 = hbox({
            text("Bit monitor") | color(Color::White) | vcenter,
            separator(),
            hbox({
                separatorEmpty(),
                bit_labels,
                bits.component()->Render()
            })
        });

        auto row6 = hbox({
            text("Gauge:     ") | color(Color::White) | vcenter,
            separator(),
            gaugeRight(bits.value() / 255.0) | color(Color::Purple) | xflex,
        });

        return vbox({
            paragraph(PVTUI_TEXT) | color(Color::Blue),
            separator(),
            row1,
            separator(),
            row2,
            separator(),
            row3,
            separator(),
            row4,
            separator(),
            row5,
            separator(),
            row6,
            separator(),
        }) | size(WIDTH, EQUAL, 50);

    }) | bgcolor(Color::Black);

    // Main loop
    app.run(main_renderer);
}
