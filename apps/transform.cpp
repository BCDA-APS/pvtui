#include <pv/caProvider.h>
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
pvtui_transform - Terminal UI for EPICS transform record
Inspired by MEDM transform record screens.

Usage:
  pvtui_transform [options]

Options:
  -h, --help        Show this help message and exit.
  -m, --macro       Macros to pass to the UI (required: P, T)

Examples:
    pvtui_transform --macro "P=xxx:,C=userTran1"

For more details, visit: https://github.com/nmarks99/pvtui
)";


// DisplayBase subclass to simplify adding rows which are the same
// except for the PV names
class TransformRow : public DisplayBase {
  public:
    TransformRow(App &app, const std::string &row_name) : DisplayBase(app),
	cmtx(app, std::string("$(P)$(T).CMT")+row_name, pvtui::PVPutType::String),
	inpx(app, std::string("$(P)$(T).INP")+row_name, pvtui::PVPutType::String),
	clcx(app, std::string("$(P)$(T).CLC")+row_name, pvtui::PVPutType::String),
	valx(app, std::string("$(P)$(T).")+row_name, pvtui::PVPutType::Double),
	outx(app, std::string("$(P)$(T).OUT")+row_name, pvtui::PVPutType::String),
	row_name_(row_name) {}

    ~TransformRow() override = default;

    ftxui::Component get_container() override {
        return Container::Vertical({
            cmtx.component(),
            inpx.component(),
            clcx.component(),
            valx.component(),
            outx.component()
        });
    }

    ftxui::Element get_renderer() override {
        return hbox({
            text(" " + row_name_ + " ") | color(Color::Black),
            separator() | color(Color::Black),
            cmtx.component()->Render()
                | size(WIDTH, EQUAL, 15)
                | EPICSColor::edit(cmtx),
            separator() | color(Color::Black),
            inpx.component()->Render()
                | size(WIDTH, EQUAL, 20)
                | EPICSColor::link(cmtx),
            separator() | color(Color::Black),
            clcx.component()->Render()
                | size(WIDTH, EQUAL, 25)
                | EPICSColor::edit(cmtx),
            separator() | color(Color::Black),
            valx.component()->Render()
                | size(WIDTH, EQUAL, 15)
                | EPICSColor::edit(cmtx),
            separator() | color(Color::Black),
            outx.component()->Render()
                | size(WIDTH, EQUAL, 20)
                | EPICSColor::link(cmtx),
        });
    }

  private:
    InputWidget cmtx;
    InputWidget inpx;
    InputWidget clcx;
    InputWidget valx;
    InputWidget outx;
    std::string row_name_;
};


int main(int argc, char *argv[]) {

    App app(argc, argv);
    if (app.args.help(CLI_HELP_MSG)) return EXIT_SUCCESS;
    if (not app.args.macros_present({"P", "T"})) {
        printf("Missing required macros\nRequired macros: P, T\n");
        return EXIT_FAILURE;
    }

    ChoiceWidget scan(app, "$(P)$(T).SCAN", ChoiceStyle::Dropdown);
    ButtonWidget proc(app, "$(P)$(T).PROC", " PROC ");
    InputWidget desc(app, "$(P)$(T).DESC", PVPutType::String);
    InputWidget prec(app, "$(P)$(T).PREC", PVPutType::Integer);
    InputWidget flnk(app, "$(P)$(T).FLNK", PVPutType::String);
    ChoiceWidget copt(app, "$(P)$(T).COPT", ChoiceStyle::Dropdown);

    // add a row for transform record fields A through P (e.g. INPA, CLCA...INPA, CLCP)
    std::vector<std::unique_ptr<DisplayBase>> rows;
    for (char c = 'A'; c <= 'P'; c++) {
        std::string s {c};
        rows.emplace_back(std::make_unique<TransformRow>(app, s));
    }

    // Main container to define interactivity of components
    auto main_container = Container::Vertical({
        desc.component(),
        scan.component(),
        proc.component(),
        prec.component(),
        copt.component(),
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
                | size(WIDTH, EQUAL, 10),
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
            text("Comment")
                | color(Color::Black)
                | size(WIDTH, EQUAL, 15),
            separator() | color(Color::Black),
            text("In link")
                | color(Color::Black)
                | size(WIDTH, EQUAL, 20),
            separator() | color(Color::Black),
            text("Calc")
                | color(Color::Black)
                | size(WIDTH, EQUAL, 25),
            separator() | color(Color::Black),
            text("Value")
                | color(Color::Black)
                | size(WIDTH, EQUAL, 15),
            separator() | color(Color::Black),
            text("Out link")
                | color(Color::Black)
                | size(WIDTH, EQUAL, 20)
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
            text("Calc option: ") | color(Color::Black),
            copt.component()->Render()
                | EPICSColor::edit(copt)
                | size(WIDTH, EQUAL, 15),
            filler() | size(WIDTH, EQUAL, 5),
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
