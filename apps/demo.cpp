#include <string>
// #include <pv/caProvider.h>
// #include <pva/client.h>

#include <ftxui/component/component.hpp>
// #include <ftxui/dom/elements.hpp>
// #include <ftxui/component/component_base.hpp>
// #include <ftxui/component/loop.hpp>
// #include <ftxui/component/screen_interactive.hpp>
// #include <ftxui/component/event.hpp>
// #include <ftxui/dom/node.hpp>
const std::string PVTUI_TEXT = R"(
██████╗ ██╗   ██╗████████╗██╗   ██╗██╗
██╔══██╗██║   ██║╚══██╔══╝██║   ██║██║
██████╔╝██║   ██║   ██║   ██║   ██║██║
██╔═══╝ ╚██╗ ██╔╝   ██║   ██║   ██║██║
██║      ╚████╔╝    ██║   ╚██████╔╝██║
╚═╝       ╚═══╝     ╚═╝    ╚═════╝ ╚═╝
)";

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

    InputWidget str_inp(app, P+"string.VAL", PVPutType::String);
    ButtonWidget plus_button(app, P+"add1.PROC", " + ");
    ButtonWidget minus_button(app, P+"subtract1.PROC", " - ");
    VarWidget<std::string> int_val(app, P+"long.VAL");

    // BitsWidget displays an integer's individual bits
    size_t nbits = 8;
    BitsWidget bits(app, P+"int8.VAL", nbits);
    Elements labs;
    for (size_t i = 0; i < nbits; i++) {
	labs.push_back(text(std::to_string(i) + ":") | color(Color::White));
    }
    auto bit_labels = vbox(labs);

    // ftxui container to define interactivity of components
    auto main_container = Container::Vertical({
	str_inp.component(),
	bits.component(),
	plus_button.component(),
	minus_button.component(),
    });

    // ftxui renderer defines the visual layout
    auto main_renderer = Renderer(main_container, [&] {


	auto row1 = hbox({
	    vbox({
		text("Input      ") | color(Color::White),
	    }) | center,
	    separator(),
	    vbox({
		str_inp.component()->Render() | bgcolor(Color::White),
	    }) | center
	});

	auto row2 = hbox({
	    vbox({
		text("Buttons    ")
	    }) | center,
	    separator(),
	    vbox({
		hbox({
		    plus_button.component()->Render(),
		    separatorEmpty(),
		    minus_button.component()->Render(),
		}),
	    }) | center,
	});

	auto row3 = hbox({
	    vbox({
		text("Readback   ")
	    }) | center,
	    separator(),
	    vbox({
		text(int_val.value()) | color(Color::Blue)
	    })
	});

	auto row4 = hbox({
	    vbox({
		text("Bit monitor") | color(Color::White),
	    }) | center,
	    separator(),
	    vbox({
		hbox({
		    bit_labels, bits.component()->Render()
		}),
	    }) | center
	});

	return vbox({
	    paragraph(PVTUI_TEXT),
	    separator(),
	    row1,
	    separator(),
	    row2,
	    separator(),
	    row3,
	    separator(),
	    row4,
	    separator(),
	}) | size(WIDTH, EQUAL, 50);
    });

    // Main loop
    app.run(main_renderer);
}
