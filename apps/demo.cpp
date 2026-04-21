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

Examples:
    pvtui_demo

For more details, visit https://github.com/BCDA-APS/pvtui
)";

using namespace ftxui;
using namespace pvtui;

int main(int argc, char* argv[]) {

    // Create an App which manages PVs and macros
    pvtui::App app(argc, argv);

    // Show help message and return if requested
    if (app.args.help(CLI_HELP_MSG)) return EXIT_SUCCESS;

    std::string prefix;
    if (auto p = std::getenv("USER")) {
        prefix = std::string(p) + ":";
        if (prefix == ":") {
            std::cout << "$USER variable empty. Check your environment\n";
            return EXIT_FAILURE;
        }
    }

    InputWidget inp1(app, prefix + "string.VAL", Color::Black, Color::White);
    ButtonWidget plus_button(app, prefix + "add1.PROC", " + ", ButtonOption::Simple());
    ButtonWidget minus_button(app, prefix + "subtract1.PROC", " - ", ButtonOption::Simple());
    Monitor<int> int_val(app, prefix + "long.VAL");
    ChoiceWidget enum_h(app, prefix + "enum.VAL", ChoiceStyle::Horizontal);
    ChoiceWidget enum_v(app, prefix + "enum.VAL", ChoiceStyle::Vertical);
    ChoiceWidget enum_d(app, prefix + "enum.VAL", ChoiceStyle::Dropdown);
    SliderWidget int_val_slide(app, prefix + "long.VAL", {0, 255, 1}, Color::Cyan, Color::GrayDark);

    size_t nbits = 8;
    BitsWidget bits(app, prefix + "int8.VAL", {0, nbits});
    Elements labs;
    for (size_t i = 0; i < nbits; i++) {
        labs.push_back(text(std::to_string(i) + ":") | color(Color::White));
    }
    auto bit_labels = vbox(labs);

    // ftxui container to define interactivity of components
    // Only interactive components need to be included here.
    auto container = Container::Vertical({
        inp1.component(),
        plus_button.component(),
        minus_button.component(),
        enum_h.component(),
        enum_v.component(),
        enum_d.component(),
        int_val_slide.component()
    });

    // ftxui renderer defines the visual layout of components
    // We apply the renderer to the container with the "|=" operator.
    container |= Renderer([&](Element){
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
            text("Slider     ") | color(Color::White) | vcenter,
            separator(),
            int_val_slide.component()->Render()
        });

        auto row5 = hbox({
            vbox({text("Selectors  ") | color(Color::White)}) | center,
            separator(),
            hbox({
                vbox({
                    separatorEmpty(),
                    enum_h.component()->Render() | EPICSColor::menu(enum_h),
                    separatorEmpty(),
                }),
                separatorEmpty(),
                separatorEmpty(),
                vbox({
                    separatorEmpty(),
                    enum_v.component()->Render() | EPICSColor::menu(enum_v),
                    separatorEmpty(),
                }),
                separatorEmpty(),
                separatorEmpty(),
                vbox({
                    separatorEmpty(),
                    enum_d.component()->Render() | EPICSColor::menu(enum_d),
                    separatorEmpty(),
                }),
            }) | center | size(HEIGHT, EQUAL, 5),
        });

        auto row6 = hbox({
            text("Bit monitor") | color(Color::White) | vcenter,
            separator(),
            hbox({
                separatorEmpty(),
                bit_labels,
                bits.component()->Render()
            })
        });

        auto row7 = hbox({
            text("Gauge      ") | color(Color::White) | vcenter,
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
            row7,
            separator(),
        }) | size(WIDTH, EQUAL, 50);
    });

    // Main loop
    app.run(container);
}
