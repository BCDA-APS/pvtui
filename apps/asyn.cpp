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
pvtui_asyn - Terminal UI for EPICS asyn record.
Inspired by MEDM asyn record screens.

Usage:
  pvtui_asyn <record>

Options:
  -h, --help        Show this help message and exit.

Examples:
    pvtui_asyn xxx:asyn1

For more details, visit: https://github.com/BCDA-APS/pvtui
)";


// Helper to make the Element for toggle widgets for the asyn masks
Element trace_toggle(ChoiceWidget& choice_widget, std::string label) {
    return hbox({
        choice_widget.component()->Render()
            | EPICSColor::edit(choice_widget)
            | size(WIDTH, EQUAL, 7),
        separatorEmpty(),
        text(label) | color(Color::Black)
    });
}


int main(int argc, char *argv[]) {

    pvtui::App app(argc, argv);

    if (app.args.help(CLI_HELP_MSG)) return EXIT_SUCCESS;

    auto pos_args = app.args.positional_args();
    if (pos_args.size() != 2) {
        std::cout << "Invalid arguments" << std::endl;
        std::cout << CLI_HELP_MSG << std::endl;
        return EXIT_FAILURE;
    }
    const std::string record_name = pos_args[1];

    InputWidget aout(app, record_name + ".AOUT");
    InputWidget oeos(app, record_name + ".OEOS");
    InputWidget ieos(app, record_name + ".IEOS");
    InputWidget tmot(app, record_name + ".TMOT");
    InputWidget tfil(app, record_name + ".TFIL");
    InputWidget nowt(app, record_name + ".NOWT");
    Monitor<PVEnum> stat(app, record_name + ".STAT");
    Monitor<PVEnum> sevr(app, record_name + ".SEVR");
    Monitor<std::string> tinp(app, record_name + ".TINP");
    Monitor<std::string> nawt(app, record_name + ".NAWT");
    Monitor<std::string> nord(app, record_name + ".NORD");
    Monitor<std::string> errs(app, record_name + ".ERRS");
    ChoiceWidget tmod(app, record_name + ".TMOD", ChoiceStyle::Dropdown);
    ChoiceWidget tb0(app, record_name + ".TB0", ChoiceStyle::Horizontal);
    ChoiceWidget tb1(app, record_name + ".TB1", ChoiceStyle::Horizontal);
    ChoiceWidget tb2(app, record_name + ".TB2", ChoiceStyle::Horizontal);
    ChoiceWidget tb3(app, record_name + ".TB3", ChoiceStyle::Horizontal);
    ChoiceWidget tb4(app, record_name + ".TB4", ChoiceStyle::Horizontal);
    ChoiceWidget tb5(app, record_name + ".TB5", ChoiceStyle::Horizontal);
    ChoiceWidget tib0(app, record_name + ".TIB0", ChoiceStyle::Horizontal);
    ChoiceWidget tib1(app, record_name + ".TIB1", ChoiceStyle::Horizontal);
    ChoiceWidget tib2(app, record_name + ".TIB2", ChoiceStyle::Horizontal);
    ChoiceWidget tinb0(app, record_name + ".TINB0", ChoiceStyle::Horizontal);
    ChoiceWidget tinb1(app, record_name + ".TINB1", ChoiceStyle::Horizontal);
    ChoiceWidget tinb2(app, record_name + ".TINB2", ChoiceStyle::Horizontal);
    ChoiceWidget tinb3(app, record_name + ".TINB3", ChoiceStyle::Horizontal);
    ChoiceWidget cnct(app, record_name + ".CNCT", ChoiceStyle::Dropdown);
    ChoiceWidget enbl(app, record_name + ".ENBL", ChoiceStyle::Dropdown);
    ChoiceWidget auct(app, record_name + ".AUCT", ChoiceStyle::Dropdown);

    auto sevr_color = [&]() -> Decorator {
        if (sevr.value().choice.find("MAJOR") != std::string::npos) {
            return EPICSColor::custom(sevr, color(Color::Red));
        } else if (sevr.value().choice.find("WARN") != std::string::npos) {
            return EPICSColor::custom(sevr, color(Color::Orange1));
        } else {
            return EPICSColor::readback(sevr);
        }
    };

    auto main_container = ftxui::Container::Vertical({
        tmot.component(),
        tmod.component(),
        aout.component(),
        oeos.component(),
        ieos.component(),
        cnct.component(),
        enbl.component(),
        auct.component(),
        tb0.component(),
        tb1.component(),
        tb2.component(),
        tb3.component(),
        tb4.component(),
        tb5.component(),
        tib0.component(),
        tib1.component(),
        tib2.component(),
        tinb0.component(),
        tinb1.component(),
        tinb2.component(),
        tinb3.component(),
        tfil.component(),
    }) | Renderer([&](Element){
        return vbox({
            text(record_name)
                | bold | italic
                | bgcolor(Color::NavyBlue) | color(Color::White),
            separatorEmpty(),

            hbox({
                text("Timeout(sec): ") | color(Color::Black),
                tmot.component()->Render() | EPICSColor::edit(tmot) | size(WIDTH, EQUAL, 6),
                filler(),
                text("Transfer: ") | color(Color::Black),
                tmod.component()->Render() | EPICSColor::edit(tmod) | size(WIDTH, LESS_THAN, 12),
            }),
            separator(),
            hbox({
                text("Out: ") | color(Color::Black),
                aout.component()->Render() | EPICSColor::edit(aout) | xflex,
                separatorEmpty(),
                oeos.component()->Render() | EPICSColor::edit(oeos) | size(WIDTH, EQUAL, 5),
                separatorEmpty(),
                text(nawt.value()) | color(Color::Black) | size(WIDTH, EQUAL, 3),
            }),
            separatorEmpty(),
            hbox({
                text(" In: ") | color(Color::Black),
                text(tinp.value()) | bgcolor(Color::RGB(220,220,220)) | EPICSColor::readback(tinp) | xflex,
                separatorEmpty(),
                ieos.component()->Render() | EPICSColor::edit(ieos) | size(WIDTH, EQUAL, 5),
                separatorEmpty(),
                text(nord.value()) | color(Color::Black) | size(WIDTH, EQUAL, 3),
            }),
            separator(),
            hbox({
                text("Err: ") | color(Color::Black),
                paragraph(errs.value()) | bgcolor(Color::RGB(220,220,220)) | EPICSColor::readback(errs) | xflex,
            }),
            separatorEmpty(),
            hbox({
                cnct.component()->Render() | EPICSColor::edit(cnct),
                filler(),
                enbl.component()->Render() | EPICSColor::edit(enbl),
                filler(),
                auct.component()->Render() | EPICSColor::edit(auct)
            }),
            separatorEmpty(),
            hbox({
                text("I/O Status: ") | color(Color::Black),
                text(stat.value().choice) | EPICSColor::readback(stat),
                filler(),
                text("I/O Severity: ") | color(Color::Black),
                text(sevr.value().choice) | sevr_color()
            }),

            separator(),

            hbox({
                vbox({
                    text("traceMask")
                        | bold | underlined
                        | size(WIDTH, EQUAL, 7)
                        | color(Color::Black),
                    trace_toggle(tb0, "traceError"),
                    trace_toggle(tb1, "traceIODevice"),
                    trace_toggle(tb2, "traceIOFilter"),
                    trace_toggle(tb3, "traceDriver"),
                    trace_toggle(tb4, "traceFlow"),
                    trace_toggle(tb5, "traceWarning"),
                }),
                filler(),
                vbox({
                    text("traceIOMask")
                        | bold | underlined
                        | size(WIDTH, EQUAL, 9)
                        | color(Color::Black),
                    trace_toggle(tib0, "traceIOASCII"),
                    trace_toggle(tib1, "traceIOEscape"),
                    trace_toggle(tib2, "traceIOHex"),

                    separatorEmpty(),

                    text("traceInfoMask")
                        | bold | underlined
                        | size(WIDTH, EQUAL, 11)
                        | color(Color::Black),
                    trace_toggle(tinb0, "traceInfoTime"),
                    trace_toggle(tinb1, "traceInfoPort"),
                    trace_toggle(tinb2, "traceInfoSource"),
                    trace_toggle(tinb3, "traceInfoThread"),
                }),
            }),
            separatorEmpty(),
            hbox({
                text("Trace file: ") | color(Color::Black),
                tfil.component()->Render() | EPICSColor::edit(tfil),
            }),
            separatorEmpty(),
        }) | border | color(Color::Black) | size(WIDTH, EQUAL, 52) | center | EPICSColor::background();
    });

    app.run(main_container);
}
