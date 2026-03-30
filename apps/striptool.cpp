#include <ftxui/component/component.hpp>
#include <ftxui/component/loop.hpp>
#include <ftxui-plot/plot.hpp>
#include <pvtui/pvtui.hpp>
#include <thread>
#include <chrono>

static constexpr std::string_view CLI_HELP_MSG = R"(
pvtui_striptool - EPICS live strip chart plotting tool.
A maximum of 10 PVs are supported.

Usage:
    pvtui_striptool [options] <pv_names...>

Options:
    -h, --help     Show this help message and exit.
    --prefix       Prefix to prepend to each PV name.

Examples:
    pvtui_striptool --prefix "MyIOC:" m1.RBV m2.RBV m3.RBV
    pvtui_striptool MyIOC:m1.RBV IOC2:Temp1.VAL IOC3:Temp2.VAL

For more details, visit: https://github.com/BCDA-APS/pvtui
)";

using namespace ftxui;
using namespace pvtui;
using namespace std::chrono_literals;

using PlotData = std::vector<PlotSeries<std::deque<double>>>;

constexpr double NaN = std::numeric_limits<double>::quiet_NaN();
constexpr double DEFAULT_REFRESH_RATE_SEC = 0.1;
constexpr double DEFAULT_TIME_SPAN_SEC = 5.0;

// We are abitrarily limiting it to 10 PVs on the plot at once
constexpr int MAX_CHANNELS = 10;
std::array<Color, MAX_CHANNELS> colors = {
    Color::Red, Color::Blue, Color::Green, Color::Purple,
    Color::GrayLight, Color::LightCoral, Color::LightGreenBis,
    Color::LightSlateBlue, Color::DarkOrange, Color::Yellow
};

// Manages the data for a single PV channel
struct Channel {
    Channel(Monitor<double> var, Color color, double y0 = NaN) :
        x(arange<std::deque<double>>(0, DEFAULT_TIME_SPAN_SEC, DEFAULT_REFRESH_RATE_SEC)),
        y(std::deque<double>(x.size(), y0)), color(color), var(var) {}

    void resize(double new_span, double sample_rate) {
        // Update X data
        x = arange<std::deque<double>>(0, new_span, sample_rate);
        size_t new_size = x.size();

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
    Monitor<double> var;
};


int main(int argc, char *argv[]) {

    pvtui::App app(argc, argv, {"--prefix"});
    if (app.args.help(CLI_HELP_MSG)) return EXIT_SUCCESS;

    // PV names to monitor are passed as positional arguments
    auto pos_args = app.args.positional_args();
    if (pos_args.size() < 2 || pos_args.size() >= MAX_CHANNELS+1) {
        std::cout << CLI_HELP_MSG;
        return EXIT_SUCCESS;
    }
    std::vector<std::string> pv_names(pos_args.begin()+1, pos_args.end());

    std::string prefix = app.args.param("--prefix");
    std::transform(pv_names.begin(), pv_names.end(), pv_names.begin(), [&](auto& s){
        return prefix + s;
    });

    // Create Monitor for each requested PV
    std::vector<Channel> channels;
    auto color_it = colors.begin();
    for (const auto& pv_name : pv_names) {
        Monitor<double> var(app.pvgroup, pv_name);
        app.pvgroup.sync();
        Channel chan(var, *color_it);
        channels.push_back(std::move(chan));
        color_it = std::next(color_it);
    }

    // Add the data for plotting
    PlotData data;
    for (auto& chan : channels) {
        data.push_back({&chan.x, &chan.y, &chan.color});
    }

    // Sample rate
    double refresh_rate = DEFAULT_REFRESH_RATE_SEC;

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

    // xmax is special because it is also used as the time span
    auto xmax_op = InputOption{};
    xmax_op.multiline = false;
    xmax_op.content = &xmax;
    xmax_op.on_enter = [&]{
        try {
            double new_span = std::stod(xmax);
            for (auto& chan : channels) {
                chan.resize(new_span, refresh_rate);
            }
        } catch (...) {};
    };
    auto xmax_inp = Input(xmax_op);

    auto to_string_fmt2 = [](double v) {
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%.2f", v);
        return std::string(buf);
    };

    // Input for sample rate
    std::string refresh_rate_str = to_string_fmt2(refresh_rate);
    auto rate_inp_op = InputOption{};
    rate_inp_op.multiline = false;
    rate_inp_op.content = &refresh_rate_str;
    rate_inp_op.on_enter = [&]{
        try {
            refresh_rate = std::stod(refresh_rate_str);
            double new_span = std::stod(xmax);
            for (auto& chan : channels) {
                chan.resize(new_span, refresh_rate);
            }
        } catch (...) {
            refresh_rate = DEFAULT_REFRESH_RATE_SEC;
            refresh_rate_str = to_string_fmt2(refresh_rate);
        }
    };
    auto refresh_rate_inp = Input(rate_inp_op);

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
        refresh_rate_inp,
    });

    bool show_menu = true;

    // Menu/sidebar to set axes limits and show plot legend
    auto menu_renderer = Renderer([&] {
        return vbox({
            text("Axis Limits") | underlined | bold,
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

            text("Refresh Rate") | underlined | bold,
            hbox({
                refresh_rate_inp->Render() | size(WIDTH, EQUAL, 6) | bgcolor(Color::RGB(50,50,50)),
                text(" sec")
            }),

            separatorEmpty(),

            // plot legend showing connected PVs and their values
            text("Channels") | underlined | bold,
            [&]{
                Elements legend_elems;
                for (const auto& chan : channels) {
                    legend_elems.push_back(
                    hbox({
                        text(unicode::rectangle(1)) | color(chan.color),
                        text(chan.var.pv_name() + " = "),
                        chan.var.connected()
                            ? text(std::to_string(chan.var.value()))
                            : text("Disconnected") | color(Color::Yellow),
                    }));
                    legend_elems.push_back(separatorEmpty());
                }
                return vbox(legend_elems);
            }(),
            filler() | yflex,
            text("Press 'm' to show/hide") | italic | dim
        }) | border | flex | size(WIDTH, GREATER_THAN, 34);
    }) | Maybe(&show_menu);

    // show/hide menu side bar with 'm' key
    main_container |= CatchEvent([&](Event event){
        if (event == Event::Character('m')) {
            show_menu = show_menu ? false : true;
            return true;
        }
        return false;
    });


    // Main renderer to define visual layout of components and elements
    auto main_renderer = Renderer(main_container, [&] {
        return hbox({
            plot->Render() | (border | (plot->Active() ? color(Color::LightSkyBlue1) : color(Color::White))),
            menu_renderer->Render()
        });
    });

    // Custom main loop since we need to update Y data deque.
    app.main_loop = [&channels, &refresh_rate](pvtui::App& app, const Component& renderer) {
        Loop loop(&app.screen, renderer);

        auto last_sample = std::chrono::steady_clock::now();
        while (!loop.HasQuitted()) {
            app.pvgroup.sync();

            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration<double>(now - last_sample).count();
            if (elapsed >= refresh_rate) {
                for (auto& chan : channels) {
                    // deques are always full. push_back(latest value) and pop_front(oldest value)
                    chan.y.push_back(chan.var.connected() ? chan.var.value() : NaN);
                    chan.y.pop_front();
                }
                app.screen.PostEvent(Event::Custom);
                last_sample = now;
            }

            loop.RunOnce();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    };

    app.run(main_renderer);

    return 0;
}
