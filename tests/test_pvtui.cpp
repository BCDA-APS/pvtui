#include <pvtui/pvtui.hpp>
#include <ftxui/component/component.hpp>

using namespace ftxui;
using namespace pvtui;

int main(int argc, char *argv[]) {

    pvtui::App app(argc, argv);

    auto pos_args = app.args.positional_args();
    if (pos_args.size() < 2) {
	printf("Usage: test_pvtui <prefix>\n");
	return EXIT_FAILURE;
    }
    const std::string prefix = pos_args[1];

    InputWidget desc(app, prefix + "Value.DESC", PVPutType::String);
    InputWidget val(app, prefix + "Value.VAL", PVPutType::Double);
    InputWidget twv(app, prefix + "TweakVal.VAL", PVPutType::Double);
    ButtonWidget twf(app, prefix + "TweakFwd.PROC", " + ");
    ButtonWidget twr(app, prefix + "TweakRev.PROC", " - ");

    // ftxui container to define interactivity of components
    auto main_container = Container::Vertical({
	desc.component(),
	val.component(),
	twf.component(),
	twr.component(),
	twv.component(),
    });

    // ftxui renderer defines the visual layout
    auto main_renderer = ftxui::Renderer(main_container, [&] {
        return vbox({

	    hbox({
		text("DESC: "),
		desc.component()->Render()
		    | size(WIDTH, EQUAL, 15)
		    | EPICSColor::edit(desc),
	    }),

	    separator(),

	    hbox({
		text("VAL: "),
		val.component()->Render()
		    | size(WIDTH, EQUAL, 11)
		    | EPICSColor::edit(val),
	    }),

	    separatorEmpty(),

	    hbox({
		twr.component()->Render(),
		separatorEmpty(),
		twv.component()->Render()
		    | size(WIDTH, EQUAL, 5)
		    | EPICSColor::edit(twv),
		separatorEmpty(),
		twf.component()->Render(),
	    })

        }) | size(WIDTH, EQUAL, 20);
    });

    // Run the main loop
    app.run(main_renderer);

    return EXIT_SUCCESS;
}

