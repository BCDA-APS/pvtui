#include <pvtui/pvtui.hpp>
#include <ftxui/component/component.hpp>

using namespace ftxui;
using namespace pvtui;

int main(int argc, char *argv[]) {

    // Instantiate an App to manage everything
    pvtui::App app(argc, argv);

    // Check for required macros if desired
    if (not app.args.macros_present({"P"})) {
	printf("Missing required macro P\n");
	return EXIT_FAILURE;
    }

    // Create all the widgets
    InputWidget desc(app, "$(P)Value.DESC", PVPutType::String);
    InputWidget val(app, "$(P)Value.VAL", PVPutType::Double);
    InputWidget twv(app, "$(P)TweakVal.VAL", PVPutType::Double);
    ButtonWidget twf(app, "$(P)TweakFwd.PROC", " + ");
    ButtonWidget twr(app, "$(P)TweakRev.PROC", " - ");

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

