#include <pvtui/pvtui.hpp>
#include <ftxui/component/component.hpp>

const std::string PVTUI_TEXT = R"(
██████╗ ██╗   ██╗████████╗██╗   ██╗██╗
██╔══██╗██║   ██║╚══██╔══╝██║   ██║██║
██████╔╝██║   ██║   ██║   ██║   ██║██║
██╔═══╝ ╚██╗ ██╔╝   ██║   ██║   ██║██║
██║      ╚████╔╝    ██║   ╚██████╔╝██║
╚═╝       ╚═══╝     ╚═╝    ╚═════╝ ╚═╝
)";

using namespace ftxui;
using namespace pvtui;

int main(int argc, char *argv[]) {

    App app(argc, argv);

    if (not app.args.macros_present({"P"})) {
	printf("Missing required macros\nRequired macros: P\n");
	return EXIT_FAILURE;
    }
    std::string P = app.args.macros.at("P");

    InputWidget inp1(app, P+"string.VAL", PVPutType::String);
    ButtonWidget plus_button(app, P+"add1.PROC", " + ");
    ButtonWidget minus_button(app, P+"subtract1.PROC", " - ");
    VarWidget<int> int_val(app, P+"long.VAL");
    ChoiceWidget enum_h(app, P+"enum.VAL", ChoiceStyle::Horizontal);
    ChoiceWidget enum_v(app, P+"enum.VAL", ChoiceStyle::Vertical);
    ChoiceWidget enum_d(app, P+"enum.VAL", ChoiceStyle::Dropdown);

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
	inp1.component(),
	plus_button.component(),
	minus_button.component(),
	enum_h.component(),
	enum_v.component(),
	enum_d.component(),
	bits.component(),
    });

    // ftxui renderer defines the visual layout
    auto main_renderer = Renderer(main_container, [&] {

	auto row1 = hbox({
	    vbox({
		text("Input      ") | color(Color::White),
	    }) | center,
	    separator(),
	    vbox({
		inp1.component()->Render() | color(Color::Black) | bgcolor(Color::White),
	    }) | xflex
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
		text(std::to_string(int_val.value())) | color(Color::LightSlateBlue)
	    })
	});

	auto row4 = hbox({
	    vbox({
		text("Menu       ") | color(Color::White)
	    }) | center,
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
	    vbox({
		text("Bit monitor") | color(Color::White),
	    }) | center,
	    separator(),
	    vbox({
		hbox({
		    separatorEmpty(),
		    bit_labels, bits.component()->Render()
		}),
	    }) | center
	});

	auto row6 = hbox({
	    vbox({
		text("Gauge:     ") | color(Color::White)
	    }) | center,
	    separator(),
	    vbox({
		gaugeRight(bits.value()/255.0) | color(Color::Purple)
	    }) | xflex,
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
