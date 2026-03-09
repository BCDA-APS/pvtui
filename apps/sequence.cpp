#include <pva/client.h>

#include <ftxui/component/component.hpp>
#include <ftxui/component/loop.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>

#include <memory>
#include <pvtui/pvtui.hpp>
#include <pvtui/display_base.hpp>

using namespace ftxui;
using namespace pvtui;

static constexpr std::string_view CLI_HELP_MSG = R"(
pvtui_seq - Terminal UI for EPICS sequence record
Inspired by MEDM sequence record screens.

Usage:
  pvtui_seq [options]

Options:
  -h, --help        Show this help message and exit.
  -m, --macro       Macros to pass to the UI (required: P, S)

Examples:
    pvtui_seq --macro "P=xxx:,S=userSeq1"

For more details, visit: https://github.com/BCDA-APS/pvtui
)";


// DisplayBase subclass to simplify adding rows which are the same
// except for the PV names
class SequenceRow : public DisplayBase {
  public:
    SequenceRow(App &app, const std::string &row_name) : DisplayBase(app),
	dolx(app, std::string("$(P)$(S).DOL")+row_name, pvtui::PVPutType::String),
	dlyx(app, std::string("$(P)$(S).DLY")+row_name, pvtui::PVPutType::Double),
	dox(app, std::string("$(P)$(S).DO")+row_name, pvtui::PVPutType::Double),
	lnkx(app, std::string("$(P)$(S).LNK")+row_name, pvtui::PVPutType::String),
	row_name_(row_name) {}

    ~SequenceRow() override = default;

    ftxui::Component get_container() override {
        return Container::Vertical({
            dolx.component(),
            dlyx.component(),
            dox.component(),
            lnkx.component(),
        });
    }

    ftxui::Element get_renderer() override {
        return hbox({
            text(" " + row_name_ + " ") | color(Color::Black),
            separator() | color(Color::Black),
            dolx.component()->Render()
                | size(WIDTH, EQUAL, 20)
                | EPICSColor::link(dolx),
            separator() | color(Color::Black),
            dlyx.component()->Render()
                | size(WIDTH, EQUAL, 10)
                | EPICSColor::edit(dlyx),
            separator() | color(Color::Black),
            dox.component()->Render()
                | size(WIDTH, EQUAL, 10)
                | EPICSColor::edit(dox),
            separator() | color(Color::Black),
            lnkx.component()->Render()
                | size(WIDTH, EQUAL, 20)
                | EPICSColor::link(lnkx),
        });
    }

  private:
    InputWidget dolx;
    InputWidget dlyx;
    InputWidget dox;
    InputWidget lnkx;
    std::string row_name_;
};


int main(int argc, char *argv[]) {

    App app(argc, argv);
    if (app.args.help(CLI_HELP_MSG)) return EXIT_SUCCESS;
    if (not app.args.macros_present({"P", "S"})) {
        printf("Missing required macros\nRequired macros: P, S\n");
        return EXIT_FAILURE;
    }

    ChoiceWidget scan(app, "$(P)$(S).SCAN", ChoiceStyle::Dropdown);
    ButtonWidget proc(app, "$(P)$(S).PROC", " PROC ");
    InputWidget desc(app, "$(P)$(S).DESC", PVPutType::String);
    InputWidget prec(app, "$(P)$(S).PREC", PVPutType::Integer);
    InputWidget flnk(app, "$(P)$(S).FLNK", PVPutType::String);

    // add a row for sequence record fields 0 through 9 (e.g. DOL0, DLY0, DO0, LNK0)
    std::vector<std::unique_ptr<DisplayBase>> rows;
    for (int i = 0; i < 10; i++) {
        rows.emplace_back(std::make_unique<SequenceRow>(app, std::to_string(i)));
    }

    // Main container to define interactivity of components
    auto main_container = Container::Vertical({
        desc.component(),
        scan.component(),
        proc.component(),
        prec.component(),
        flnk.component()
    });
    for (auto &row : rows) {
	    main_container->Add(row->get_container());
    }

    // Main renderer to define visual layout of components and elements
    auto main_renderer = Renderer(main_container, [&] {
        Elements elements {
            desc.component()->Render()
            | EPICSColor::edit(desc)
            | size(WIDTH, EQUAL, 25),
            separatorEmpty(),
            hbox({
            scan.component()->Render()
                | EPICSColor::edit(scan)
                | size(WIDTH, EQUAL, 12),
            filler() | size(WIDTH, EQUAL, 20),
            proc.component()->Render()
                | EPICSColor::edit(proc)
                | size(WIDTH, EQUAL, 8),
            filler() | size(WIDTH, EQUAL, 5),
            text("PREC: ") | color(Color::Black),
            prec.component()->Render()
                | EPICSColor::edit(prec)
                | size(WIDTH, EQUAL, 3),
            filler() | xflex,
            }),
            separatorEmpty(),
            hbox({
            filler()
                | size(WIDTH, EQUAL, 3)
                | color(Color::Black),
            separator() | color(Color::Black),
            text("DOLx")
                | color(Color::Black)
                | size(WIDTH, EQUAL, 20),
            separator() | color(Color::Black),
            text("DLYx")
                | color(Color::Black)
                | size(WIDTH, EQUAL, 10),
            separator() | color(Color::Black),
            text("DOx")
                | color(Color::Black)
                | size(WIDTH, EQUAL, 10),
            separator() | color(Color::Black),
            text("LNKx")
                | color(Color::Black)
                | size(WIDTH, EQUAL, 20),
            }),
            separator() | color(Color::Black)
        };
        for (auto &row : rows) {
            elements.push_back(row->get_renderer());
            elements.push_back(separator() | color(Color::Black));
        }

        elements.push_back(
            hbox({
            filler() | xflex,
            text("FLNK: ") | color(Color::Black),
            flnk.component()->Render()
                | EPICSColor::link(flnk)
                | size(WIDTH, EQUAL, 25),
            separatorEmpty()
            })
        );

        return vbox({
            elements,
        }) | center | bgcolor(Color::RGB(196,196,196));
    });


    app.run(main_renderer);

    return EXIT_SUCCESS;
}
