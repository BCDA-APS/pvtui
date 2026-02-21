#include <string>

#include <ftxui/dom/elements.hpp>
#include <pv/caProvider.h>
#include <pva/client.h>

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/loop.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/node.hpp>

#include <pvtui/pvtui.hpp>

using namespace ftxui;
using namespace pvtui;

int main(int argc, char *argv[]) {

    App app(argc, argv);

    if (not app.args.macros_present({"P"})) {
	printf("Missing required macros\nRequired macros: P\n");
	return EXIT_FAILURE;
    }
    std::string P = app.args.macros.at("P");

    // Create all the widgets
    InputWidget string1(app, P+"string", PVPutType::String);
    InputWidget float1(app, P+"float", PVPutType::Double);
    InputWidget long1(app, P+"long", PVPutType::Integer);

    VarWidget<std::string> string1_rbv(app, P+"string");
    VarWidget<std::string> float1_rbv(app, P+"float");
    VarWidget<std::string> long1_rbv(app, P+"long");

    ChoiceWidget enum1_h(app, P+"enum", ChoiceStyle::Horizontal);
    ChoiceWidget enum1_v(app, P+"enum", ChoiceStyle::Vertical);
    ChoiceWidget enum1_d(app, P+"enum", ChoiceStyle::Dropdown);

    ButtonWidget plus(app, P+"add1.PROC", " + ");
    ButtonWidget minus(app, P+"subtract1.PROC", " - ");

    BitsWidget bits(app, P+"int8.VAL", 8);

    // ftxui container to define interactivity of components
    auto main_container = Container::Vertical({
        ftxui::Container::Vertical({
            string1.component(),
            float1.component(),
            Container::Horizontal({
                long1.component(),
                minus.component(),
                plus.component(),
            }),
            enum1_h.component(),
            enum1_v.component(),
            enum1_d.component(),
	    bits.component(),
        })
    });

    // ftxui renderer defines the visual layout
    auto main_renderer = Renderer(main_container, [&] {
        return vbox({
            separator() | color(Color::Black),
            hbox({
                vbox({
                    text("PV") | color(Color::Black) | size(WIDTH, EQUAL, 30),
                    separator() | color(Color::Black),
                    text(P+"string") | color(Color::Black),
                    separatorEmpty(),
                    text(P+"float") | color(Color::Black),
                    separatorEmpty(),
                    text(P+"long") | color(Color::Black),
                    separatorEmpty(),
                    text(P+"enum") | color(Color::Black),
                    separatorEmpty(),
                    separatorEmpty(),
                    separatorEmpty(),
                    text(P+"int8") | color(Color::Black),
                }) | size(WIDTH, EQUAL, 30),

                separator() | color(Color::Black),

                vbox({
                    text("Input") | color(Color::Black) | size(WIDTH, EQUAL, 30),
                    separator() | color(Color::Black),
                    string1.component()->Render() | EPICSColor::edit(string1),
                    separatorEmpty(),
                    float1.component()->Render() | EPICSColor::edit(float1),
                    separatorEmpty(),
                    hbox({
                        long1.component()->Render() | EPICSColor::edit(long1),
                        minus.component()->Render() | color(Color::Black),
                        plus.component()->Render() | color(Color::Black),
                    }),
                    separatorEmpty(),
                    hbox({
                        vbox({ enum1_h.component()->Render() | EPICSColor::edit(enum1_h) | xflex}),
                        separatorEmpty(),
                        vbox({ enum1_v.component()->Render() | EPICSColor::edit(enum1_v) | xflex}),
                        separatorEmpty(),
                        vbox({ enum1_d.component()->Render() | EPICSColor::edit(enum1_d) | xflex}),
                    }),
		    separatorEmpty(),
		    hbox({
			vbox({
			    text("0:"),
			    text("1:"),
			    text("2:"),
			    text("3:"),
			    text("4:"),
			    text("5:"),
			    text("6:"),
			    text("7:")}
			) | color(Color::Black),
			bits.component()->Render() | center
		    }) | center
                }) | size(WIDTH, EQUAL, 30),

                separator() | color(Color::Black),

                vbox({
                    text("Readback") | color(Color::Black) | size(WIDTH, EQUAL, 30),
                    separator() | color(Color::Black),
                    text(string1_rbv.value()) | EPICSColor::readback(string1_rbv),
                    separatorEmpty(),
                    text(float1_rbv.value()) | EPICSColor::readback(float1_rbv),
                    separatorEmpty(),
                    text(long1_rbv.value()) | EPICSColor::readback(long1_rbv),
                    separatorEmpty(),
                    text("Index = " + std::to_string(enum1_h.value().index)) | EPICSColor::readback(enum1_h),
                    text("Choice = " + enum1_h.value().choice) | EPICSColor::readback(enum1_h),
                    separatorEmpty(),
                    separatorEmpty(),
		    text(std::to_string(bits.value())) | EPICSColor::readback(bits)
                }) | size(WIDTH, EQUAL, 30),
            }),

        }) | center | EPICSColor::background();
    });

    // Main loop
    app.run(main_renderer);
}
