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

using namespace ftxui;
using namespace pvtui;

static constexpr std::string_view CLI_HELP_MSG = R"(
pvtui_transform - Terminal UI for EPICS transform record
Inspired by MEDM transform record screens.

Usage:
    pvtui_transform <record>

Options:
    -h, --help        Show this help message and exit.

Examples:
    pvtui_transform xxx:userTran1

For more details, visit: https://github.com/BCDA-APS/pvtui
)";


// DisplayBase subclass to simplify adding rows which are the same
// except for the PV names
class TransformRow : public ComponentBase {
  public:
    TransformRow(pvtui::App &app, const std::string &record, const std::string &row_name) :
        cmtx(app, record + ".CMT" + row_name, pvtui::PVPutType::String),
        inpx(app, record + ".INP" + row_name, pvtui::PVPutType::String),
        clcx(app, record + ".CLC" + row_name, pvtui::PVPutType::String),
        valx(app, record + "." + row_name, pvtui::PVPutType::Double),
        outx(app, record + ".OUT" + row_name, pvtui::PVPutType::String),
        row_name_(row_name)
    {
        auto container = Container::Vertical({
            cmtx.component(),
            inpx.component(),
            clcx.component(),
            valx.component(),
            outx.component()
        });

        container |= Renderer([&](Element){
            return hbox({
                text(" " + row_name_ + " ") | color(Color::Black),
                separator() | color(Color::Black),
                cmtx.component()->Render()
                    | size(WIDTH, EQUAL, 15)
                    | EPICSColor::edit(cmtx),
                separator() | color(Color::Black),
                inpx.component()->Render()
                    | size(WIDTH, EQUAL, 20)
                    | EPICSColor::link(inpx),
                separator() | color(Color::Black),
                clcx.component()->Render()
                    | size(WIDTH, EQUAL, 25)
                    | EPICSColor::edit(clcx),
                separator() | color(Color::Black),
                valx.component()->Render()
                    | size(WIDTH, EQUAL, 15)
                    | EPICSColor::edit(valx),
                separator() | color(Color::Black),
                outx.component()->Render()
                    | size(WIDTH, EQUAL, 20)
                    | EPICSColor::link(outx),
            });
        });

        Add(container);

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

    pvtui::App app(argc, argv);
    if (app.args.help(CLI_HELP_MSG)) return EXIT_SUCCESS;

    auto pos_args = app.args.positional_args();
    if (pos_args.size() != 2) {
        std::cout << CLI_HELP_MSG << std::endl;
        return EXIT_FAILURE;
    }
    const std::string record_name = pos_args[1];

    ChoiceWidget scan(app, record_name + ".SCAN", ChoiceStyle::Dropdown);
    ButtonWidget proc(app, record_name + ".PROC", " PROC ");
    InputWidget desc(app, record_name + ".DESC", PVPutType::String);
    InputWidget prec(app, record_name + ".PREC", PVPutType::Integer);
    InputWidget flnk(app, record_name + ".FLNK", PVPutType::String);
    ChoiceWidget copt(app, record_name + ".COPT", ChoiceStyle::Dropdown);

    std::vector<Component> rows;
    for (char c = 'A'; c <= 'P'; c++) {
        std::string s {c};
        rows.push_back(Make<TransformRow>(app, record_name, s));
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
        main_container->Add(row);
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
            elements.push_back(row->Render());
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
        }) | center | EPICSColor::background();
    });

    app.run(main_renderer);

    return EXIT_SUCCESS;
}
