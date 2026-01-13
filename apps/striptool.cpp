#include <ftxui/component/component.hpp>
#include <ftxui-plot/plot.hpp>
#include <pvtui/pvtui.hpp>
#include <chrono>

static constexpr std::string_view CLI_HELP_MSG = R"(
pvtui_striptool - EPICS live strip chart plotting tool.

Usage:
    pvtui_striptool [options] <pv_names...>

Options:
    -h, --help     Show this help message and exit.
    -m, --macro    Macros for PV name substitution (e.g., "P=Prefix:")

Examples:
    # Plot motor readbacks with a given IOC prefix
    ./pvtui_striptool --macro "P=MyIOC:" m1.RBV m2.RBV m3.RBV

    # Plot several PVs, perhaps with different prefixes
    ./pvtui_striptool MyIOC:m1.RBV IOC2:Temp1.VAL IOC3:Temp2.VAL

For more details, visit: https://github.com/nmarks99/pvtui
)";

using namespace ftxui;
using namespace pvtui;
using namespace std::chrono_literals;

using PlotData = std::vector<PlotSeries<std::deque<double>>>;

// Main loop polling time
constexpr double SAMPLE_RATE_SEC = 0.1;
constexpr double TIME_SPAN_SEC = 5.0;

// We are abitrarily limiting it to 10 PVs on the plot at once
constexpr int MAX_CHANNELS = 10;
std::array<Color, MAX_CHANNELS> colors = {
    Color::Red, Color::Blue, Color::Green, Color::Purple,
    Color::GrayLight, Color::LightCoral, Color::LightGreenBis,
    Color::LightSlateBlue, Color::DarkOrange, Color::Yellow
};

// Waits for the PV to connect, or times out
constexpr std::chrono::seconds PV_CONNECT_TIMEOUT = 3s;
bool wait_connect(const VarWidget<double> &var) {
    auto start = std::chrono::steady_clock::now();
    while (true) {
	const auto now = std::chrono::steady_clock::now();
	const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now-start);
	if (elapsed >= PV_CONNECT_TIMEOUT) {
	    return false;
	}
	if (var.connected()) {
	    return true;
	}
	std::this_thread::sleep_for(50ms);
    }
    return true;
}

// Manages the data for a single PV channel
struct Channel {
    Channel(VarWidget<double> var, Color color, double y0)
	: x(arange<std::deque<double>>(0, TIME_SPAN_SEC, SAMPLE_RATE_SEC)),
	y(std::deque<double>(x.size(), y0)), color(color), var(var)
    {}

    void resize(double new_span, double sample_rate) {
	constexpr double NaN = std::numeric_limits<double>::quiet_NaN();

	// constexpr double X_SPAN_MAX = 1000;
	// new_span = new_span > X_SPAN_MAX ? X_SPAN_MAX : new_span;
        size_t new_size = static_cast<size_t>(new_span / sample_rate);

        // Update X data
        x = arange<std::deque<double>>(0, new_span, sample_rate);

        // Update Y data
        if (y.size() > new_size) {
            // If shrinking, keep the most recent data (the back of the deque)
            while (y.size() > new_size) y.pop_front();
        } else if (y.size() < new_size) {
            while (y.size() < new_size) y.push_front(NaN);
        }
    }

    std::deque<double> x;
    std::deque<double> y;
    Color color;
    VarWidget<double> var;
};


int main(int argc, char *argv[]) {

    App app(argc, argv);
    if (app.args.help(CLI_HELP_MSG)) return EXIT_SUCCESS;

    // PV names to monitor are pass as positional arguments
    auto all_pos_args = app.args.positional_args();
    assert(all_pos_args.size() <= MAX_CHANNELS+1);
    std::vector<std::string> pv_names(all_pos_args.begin()+1, all_pos_args.end());

    // add prefix to PV names if P macro given
    if (app.args.macros_present({"P"})) {
	const std::string prefix = app.args.macros.at("P");
	std::transform(pv_names.begin(), pv_names.end(), pv_names.begin(), [&](auto& s){
	    return prefix + s;
	});
    }

    // Create VarWidget for each requested PV
    // For now, just throw if any fail to connect
    std::vector<Channel> channels;
    auto color_it = colors.begin();
    for (const auto& pv_name : pv_names) {
	std::cout << "Connecting to " << pv_name << "..." << std::flush;

	VarWidget<double> var(app.pvgroup, pv_name);
	if (!wait_connect(var)) {
	    throw std::runtime_error("Timed out trying to connect to " + pv_name);
	}

	app.pvgroup.sync();
	Channel chan(var, *color_it, var.value());
	channels.push_back(std::move(chan));
	color_it = std::next(color_it);
	std::cout << "Connected!" << std::endl;
    }

    // Add the data for plotting
    PlotData data;
    for (auto& chan : channels) {
	data.push_back({&chan.x, &chan.y, &chan.color});
    }

    // Axis limits
    // +-5 seems like a reasonable default?
    std::string ymin = "-5.0";
    std::string ymax = "5.0";
    std::string xmin = "0.0";
    std::string xmax = "5.0";
    auto make_input = [&](std::string &str){
	auto op = InputOption{};
	op.multiline = false;
	op.content = &str;
	return Input(op);
    };
    auto ymin_inp = make_input(ymin);
    auto ymax_inp = make_input(ymax);
    auto xmin_inp = make_input(xmin);

    // xmax is also used as the time span
    auto xmax_op = InputOption{};
    xmax_op.multiline = false;
    xmax_op.content = &xmax;
    xmax_op.on_enter = [&]{
	try {
	    double new_span = std::stod(xmax);
	    for (auto& chan : channels) {
		chan.resize(new_span, SAMPLE_RATE_SEC);
	    }
	} catch (...) {};
    };
    auto xmax_inp = Input(xmax_op);

    // Create the plot component
    PlotOption<std::deque<double>> op;
    op.data = &data;
    op.xmin = &xmin;
    op.xmax = &xmax;
    op.ymin = &ymin;
    op.ymax = &ymax;
    auto plot = Plot(op);

    // Main container to define interactivity of components
    auto main_container = Container::Vertical({
	plot,
	ymin_inp,
	ymax_inp,
	xmin_inp,
	xmax_inp,
    });

    // Main renderer to define visual layout of components and elements
    auto main_renderer = Renderer(main_container, [&] {
	return hbox({
	    plot->Render() | (border | (plot->Active() ? color(Color::LightSkyBlue1) : color(Color::White))),

	    // sidebar
	    vbox({
		text("Axis limits") | underlined | bold,
		hbox({
		    text("X Range: "),
		    xmin_inp->Render() | size(WIDTH, EQUAL, 6) | bgcolor(Color::RGB(50,50,50)),
		    separatorEmpty(),
		    xmax_inp->Render() | size(WIDTH, EQUAL, 6) | bgcolor(Color::RGB(50,50,50)),
		}),
		hbox({
		    text("Y Range: "),
		    ymin_inp->Render() | size(WIDTH, EQUAL, 6) | bgcolor(Color::RGB(50,50,50)),
		    separatorEmpty(),
		    ymax_inp->Render() | size(WIDTH, EQUAL, 6) | bgcolor(Color::RGB(50,50,50)),
		}),

		separatorEmpty(),

		// plot legend showing connected PVs and their values
		text("Channels") | underlined | bold,
		[&]{
		    Elements legend_elems;
		    for (const auto& chan : channels) {
			legend_elems.push_back(hbox({
			    text(unicode::rectangle(1)) | color(chan.color),
			    text(chan.var.pv_name() + " = " + std::to_string(chan.var.value()))
			}));
			legend_elems.push_back(separatorEmpty());
		    }
		    return vbox(legend_elems);
		}()

	    }) | border | flex | size(WIDTH, GREATER_THAN, 30)
	});
    });


    app.main_loop = [&channels](App& app, const Component& renderer, int poll_ms) {
	Loop loop(&app.screen, renderer);
	while (!loop.HasQuitted()) {
	    app.pvgroup.sync();

	    // deques are always full. push_back(latest value) and pop_front(oldest value)
	    for (auto& chan : channels) {
		chan.y.push_back(chan.var.value());
		chan.y.pop_front();
	    }

	    app.screen.PostEvent(Event::Custom);

	    loop.RunOnce();
	    std::this_thread::sleep_for(std::chrono::milliseconds(poll_ms));
	}
    };

    app.run(main_renderer, SAMPLE_RATE_SEC*1000);

    return 0;
}
