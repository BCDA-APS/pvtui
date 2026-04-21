#include <ftxui/component/component.hpp>
#include <pvtui/pvtui.hpp>

using namespace ftxui;
using namespace pvtui;

static constexpr std::string_view CLI_HELP_MSG = R"(
pvtui_seq - Terminal UI for EPICS sequence record
Inspired by MEDM sequence record screens.

Usage:
    pvtui_seq <record>

Options:
    -h, --help        Show this help message and exit.

Examples:
    pvtui_seq xxx:userSeq1

For more details, visit: https://github.com/BCDA-APS/pvtui
)";

class SequenceRow : public ComponentBase {
  public:
    SequenceRow(pvtui::App &app, const std::string &record, const std::string &row_name) :
        dolx(app, record + ".DOL" + row_name),
        dlyx(app, record + ".DLY" + row_name),
        dox(app, record + ".DO" + row_name),
        lnkx(app, record + ".LNK" + row_name),
        row_name_(row_name)
    {
        auto container = Container::Vertical({
            dolx.component(),
            dlyx.component(),
            dox.component(),
            lnkx.component(),
        });

        container |= Renderer([&](Element){
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
        });

        Add(container);
    }

  private:
    InputWidget dolx;
    InputWidget dlyx;
    InputWidget dox;
    InputWidget lnkx;
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
    InputWidget desc(app, record_name + ".DESC");
    InputWidget prec(app, record_name + ".PREC");
    InputWidget flnk(app, record_name + ".FLNK");

    auto head_container = Container::Horizontal({
        desc.component(),
        scan.component(),
        proc.component(),
        prec.component(),
    }) | Renderer([&](Element){
        return vbox({
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
        });
    });

    bool show_more = false;

    auto button_op = ButtonOption::Ascii();
    std::string button_label = " MORE ";
    button_op.label = &button_label;
    button_op.on_click = [&]{
        show_more = !show_more;
        button_label = show_more ? " LESS " : " MORE ";
    };
    auto view_button = Button(button_op);

    auto foot_container = Container::Horizontal({
        view_button,
        flnk.component(),
    }) | Renderer([&](Element){
        return hbox({
            view_button->Render() | bgcolor(Color::DarkGreen) | color(Color::White),
            separatorEmpty(),
            filler() | xflex,
            text("FLNK: ") | color(Color::Black),
            flnk.component()->Render()
                | EPICSColor::link(flnk)
                | size(WIDTH, EQUAL, 25),
            separatorEmpty()
        });
    });

    std::vector<Component> rows;
    for (int i = 0; i < 10; i++) {
        rows.emplace_back(Make<SequenceRow>(app, record_name, std::to_string(i)));
    }

    auto some_rows_container = Container::Vertical({});
    auto more_rows_container = Container::Vertical({});
    for (size_t i = 0; i < rows.size(); i++) {
        auto renderer = Renderer([&](Element inner){
            return vbox({
                inner,
                separator() | color(Color::Black)
            });
        });
        auto row_comp = rows.at(i) |= renderer;
        if (i < 4) {
            some_rows_container->Add(row_comp);
        } else {
            more_rows_container->Add(row_comp);
        }
    }

    auto container = Container::Vertical({
        head_container,
        some_rows_container,
        more_rows_container | Maybe([&]{return show_more;}),
        foot_container
    });

    app.run(container | center | EPICSColor::background());
}
