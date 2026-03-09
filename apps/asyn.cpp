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
  pvtui_asyn [options]

Options:
  -h, --help        Show this help message and exit.
  -m, --macro       Macros to pass to the UI (required: P, R)

Examples:
    pvtui_asyn --macro "P=xxx:,R=asyn1"

For more details, visit: https://github.com/nmarks99/pvtui
)";

int main(int argc, char *argv[]) {

    App app(argc, argv);

    if (app.args.help(CLI_HELP_MSG)) return EXIT_SUCCESS;

    if (not app.args.macros_present({"P", "R"})) {
        printf("Missing required macro P, R\n");
        return EXIT_FAILURE;
    }

    // Create all the widgets
    InputWidget aout(app, "$(P)$(R).AOUT", PVPutType::String);
    InputWidget oeos(app, "$(P)$(R).OEOS", PVPutType::String);
    InputWidget ieos(app, "$(P)$(R).IEOS", PVPutType::String);
    InputWidget tmot(app, "$(P)$(R).TMOT", PVPutType::Double);
    InputWidget tfil(app, "$(P)$(R).TFIL", PVPutType::String);
    InputWidget nowt(app, "$(P)$(R).NOWT", PVPutType::Integer);
    Monitor<PVEnum> stat(app, "$(P)$(R).STAT");
    Monitor<PVEnum> sevr(app, "$(P)$(R).SEVR");
    Monitor<std::string> tinp(app, "$(P)$(R).TINP");
    Monitor<std::string> nawt(app, "$(P)$(R).NAWT");
    Monitor<std::string> nord(app, "$(P)$(R).NORD");
    Monitor<std::string> errs(app, "$(P)$(R).ERRS");
    ChoiceWidget tmod(app, "$(P)$(R).TMOD", ChoiceStyle::Dropdown);
    ChoiceWidget tb0(app, "$(P)$(R).TB0", ChoiceStyle::Horizontal);
    ChoiceWidget tb1(app, "$(P)$(R).TB1", ChoiceStyle::Horizontal);
    ChoiceWidget tb2(app, "$(P)$(R).TB2", ChoiceStyle::Horizontal);
    ChoiceWidget tb3(app, "$(P)$(R).TB3", ChoiceStyle::Horizontal);
    ChoiceWidget tb4(app, "$(P)$(R).TB4", ChoiceStyle::Horizontal);
    ChoiceWidget tb5(app, "$(P)$(R).TB5", ChoiceStyle::Horizontal);
    ChoiceWidget tib0(app, "$(P)$(R).TIB0", ChoiceStyle::Horizontal);
    ChoiceWidget tib1(app, "$(P)$(R).TIB1", ChoiceStyle::Horizontal);
    ChoiceWidget tib2(app, "$(P)$(R).TIB2", ChoiceStyle::Horizontal);
    ChoiceWidget tinb0(app, "$(P)$(R).TINB0", ChoiceStyle::Horizontal);
    ChoiceWidget tinb1(app, "$(P)$(R).TINB1", ChoiceStyle::Horizontal);
    ChoiceWidget tinb2(app, "$(P)$(R).TINB2", ChoiceStyle::Horizontal);
    ChoiceWidget tinb3(app, "$(P)$(R).TINB3", ChoiceStyle::Horizontal);
    ChoiceWidget cnct(app, "$(P)$(R).CNCT", ChoiceStyle::Dropdown);
    ChoiceWidget enbl(app, "$(P)$(R).ENBL", ChoiceStyle::Dropdown);
    ChoiceWidget auct(app, "$(P)$(R).AUCT", ChoiceStyle::Dropdown);

    // ftxui container to define interactivity of components
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
    });

    auto sevr_color = [&]() -> Decorator{
        if (sevr.value().choice.find("MAJOR") != std::string::npos) {
            return EPICSColor::custom(sevr, color(Color::Red));
        } else if (sevr.value().choice.find("WARN") != std::string::npos) {
            return EPICSColor::custom(sevr, color(Color::Orange1));
        } else {
            return EPICSColor::readback(sevr);
        }
    };

    // ftxui renderer defines the visual layout
    auto main_renderer = Renderer(main_container, [&] {
        return vbox({
            text(app.args.macros.at("P") + app.args.macros.at("R"))
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

            // trace mask toggles
            hbox({
                vbox({
                    text("traceMask")
                        | bold
                        | underlined
                        | size(WIDTH, EQUAL, 7)
                        | color(Color::Black),
                    hbox({
                        tb0.component()->Render()
                            | EPICSColor::edit(tb0)
                            | size(WIDTH, EQUAL, 7),
                        separatorEmpty(),
                        text("traceError") | color(Color::Black)
                    }),
                    hbox({
                        tb1.component()->Render()
                            | EPICSColor::edit(tb1)
                            | size(WIDTH, EQUAL, 7),
                        separatorEmpty(),
                        text("traceIODevice") | color(Color::Black)
                    }),
                    hbox({
                        tb2.component()->Render()
                            | EPICSColor::edit(tb2)
                            | size(WIDTH, EQUAL, 7),
                        separatorEmpty(),
                        text("traceIOFilter") | color(Color::Black)
                    }),
                    hbox({
                        tb3.component()->Render()
                            | EPICSColor::edit(tb3)
                            | size(WIDTH, EQUAL, 7),
                        separatorEmpty(),
                        text("traceIODriver") | color(Color::Black)
                    }),
                    hbox({
                        tb4.component()->Render()
                            | EPICSColor::edit(tb4)
                            | size(WIDTH, EQUAL, 7),
                        separatorEmpty(),
                        text("traceFlow") | color(Color::Black)
                    }),
                    hbox({
                        tb5.component()->Render()
                            | EPICSColor::edit(tb5)
                            | size(WIDTH, EQUAL, 7),
                        separatorEmpty(),
                        text("traceWarning") | color(Color::Black)
                    }),
                }),
                filler(),
                vbox({
                    text("traceIOMask")
                        | bold
                        | underlined
                        | size(WIDTH, EQUAL, 9)
                        | color(Color::Black),
                    hbox({
                        tib0.component()->Render()
                            | EPICSColor::edit(tib0)
                            | size(WIDTH, EQUAL, 7),
                        separatorEmpty(),
                        text("traceIOASCII") | color(Color::Black)
                    }),
                    hbox({
                        tib1.component()->Render()
                            | EPICSColor::edit(tib1)
                            | size(WIDTH, EQUAL, 7),
                        separatorEmpty(),
                        text("traceIOEscape") | color(Color::Black)
                    }),
                    hbox({
                        tib2.component()->Render()
                            | EPICSColor::edit(tib2)
                            | size(WIDTH, EQUAL, 7),
                        separatorEmpty(),
                        text("traceIOHex") | color(Color::Black)
                    }),

                    separatorEmpty(),

                    text("traceInfoMask")
                        | bold
                        | underlined
                        | size(WIDTH, EQUAL, 11)
                        | color(Color::Black),
                    hbox({
                        tinb0.component()->Render()
                            | EPICSColor::edit(tinb0)
                            | size(WIDTH, EQUAL, 7),
                        separatorEmpty(),
                        text("traceInfoTime") | color(Color::Black)
                    }),
                    hbox({
                        tinb1.component()->Render()
                            | EPICSColor::edit(tinb1)
                            | size(WIDTH, EQUAL, 7),
                        separatorEmpty(),
                        text("traceInfoPort") | color(Color::Black)
                    }),
                    hbox({
                        tinb2.component()->Render()
                            | EPICSColor::edit(tinb2)
                            | size(WIDTH, EQUAL, 7),
                        separatorEmpty(),
                        text("traceInfoSource") | color(Color::Black)
                    }),
                    hbox({
                        tinb3.component()->Render()
                            | EPICSColor::edit(tinb3)
                            | size(WIDTH, EQUAL, 7),
                        separatorEmpty(),
                        text("traceInfoThread") | color(Color::Black)
                    }),
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

    app.run(main_renderer);
}
