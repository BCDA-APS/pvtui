#include <pvtui/pvtui.hpp>

#include <ftxui-plot/plot.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/loop.hpp>
#include <ftxui/dom/elements.hpp>

#include <chrono>
#include <deque>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace ftxui;
using namespace pvtui;
using namespace std::chrono_literals;

static constexpr std::string_view CLI_HELP_MSG = R"(
pvtui_waveform - Terminal UI for plotting several waveform PVs.

Usage:
  pvtui_waveform [options] PV1 PV2 PV3 ...

Options:
  -h, --help          Show this help message.
  --prefix PREFIX     Prefix to prepend to PV names.

Examples:
  pvtui_waveform MyIOC:wf1 MyIOC:wf2
  pvtui_waveform --prefix MyIOC: wf1 wf2
)";

using PlotData = std::vector<ftxui::PlotSeries<std::deque<double>>>;
// We are abitrarily limiting it to 10 PVs on the plot at once
constexpr int MAX_CHANNELS = 10;

struct WaveformChannel {
    WaveformChannel(pvtui::App& app, const std::string& pv_name, Color color_)
        : monitor(app.pvgroup, pv_name), name(pv_name), color(color_) {}

    void update_plot_data() {
        const auto& waveform = monitor.value();

        x.clear();
        y.clear();

        for (size_t i = 0; i < waveform.size(); ++i) {
            x.push_back(i);
            y.push_back(waveform[i]);
        }
    }

    bool connected() const { return monitor.connected(); }

    std::string name;
    Monitor<std::vector<double>> monitor;
    std::deque<double> x;
    std::deque<double> y;
    Color color;
};

int run_waveform_app(pvtui::App& app, const std::vector<std::string>& pv_names) {

    std::vector<std::unique_ptr<WaveformChannel>> channels;

    std::vector<Color> colors = {
        Color::Red, Color::Blue, Color::Green, Color::Purple,
        Color::GrayLight, Color::LightCoral, Color::LightGreenBis,
        Color::LightSlateBlue, Color::DarkOrange, Color::Yellow
    };

    for (size_t i = 0; i < pv_names.size(); ++i) {
        channels.push_back(std::make_unique<WaveformChannel>(app, pv_names[i], colors[i % colors.size()]));
    }

    PlotData data;
    size_t max_size = 0;
    for (auto& chan : channels) {
        data.push_back({&chan->x, &chan->y, &chan->color});
        max_size = std::max(max_size, chan->y.size());
    }
    // Axis limits
    // +-5 seems like a reasonable default?
    std::string xmin = "0";
    std::string xmax = "100";
    std::string ymin = "-5";
    std::string ymax = "5";
    PlotOption<std::deque<double>> plot_options;
    plot_options.data = &data;
    plot_options.xmin = &xmin;
    plot_options.xmax = &xmax;
    plot_options.ymin = &ymin;
    plot_options.ymax = &ymax;

    auto plot = Plot(plot_options);

    auto make_input = [](std::string& str) {
        auto op = InputOption{};
        op.multiline = false;
        op.content = &str;
        return Input(op);
    };

    auto xmin_input = make_input(xmin);
    auto xmax_input = make_input(xmax);
    auto ymin_input = make_input(ymin);
    auto ymax_input = make_input(ymax);

    auto controls = Container::Vertical({
        xmin_input,
        xmax_input,
        ymin_input,
        ymax_input,
    }) | Renderer([&](Element) {
        return vbox({
            text("Axis limits") | bold | underlined,
            separatorEmpty(),
            hbox({
               text("X min: "),
               xmin_input->Render() | size(WIDTH, EQUAL, 8),
            }),
            hbox({
               text("X max: "),
               xmax_input->Render() | size(WIDTH, EQUAL, 8),
            }),
            hbox({
               text("Y min: "),
               ymin_input->Render() | size(WIDTH, EQUAL, 8),
            }),
            hbox({
               text("Y max: "),
               ymax_input->Render() | size(WIDTH, EQUAL, 8),
            }),
       }) | border | size(WIDTH, EQUAL, 24);
    });

    auto component = Container::Horizontal({
        controls,
    }) | Renderer([&](Element controls_rendered) {
        Elements legend;

        for (auto& chan : channels) {
            std::string status = chan->connected() ? "● " : "○ ";
            legend.push_back(text(status + chan->name) | color(chan->color));
            legend.push_back(text("  "));
        }

        return hbox({
            vbox({
                hbox(std::move(legend)),
                separator(),
                plot->Render() | flex,
            }) | border | flex,
            controls_rendered,
        });
     });

    app.main_loop = [&channels](pvtui::App& app, const Component& renderer) {
        Loop loop(&app.screen, renderer);

        while (!loop.HasQuitted()) {
            app.pvgroup.sync();

            for (auto& chan : channels) {
                chan->update_plot_data();
            }

            app.screen.PostEvent(Event::Custom);
            loop.RunOnce();
            std::this_thread::sleep_for(50ms);
        }
    };

    app.run(component);
    return EXIT_SUCCESS;
}

int main(int argc, char* argv[]) {
    pvtui::App app(argc, argv, {"--prefix"});

    if (app.args.help(CLI_HELP_MSG)) {
        return EXIT_SUCCESS;
    }

    auto pos_args = app.args.positional_args();

    if (pos_args.size() < 2 || pos_args.size() >= MAX_CHANNELS + 1) {
        std::cout << CLI_HELP_MSG;
        return EXIT_SUCCESS;
    }

    std::string prefix = app.args.param("--prefix");

    std::vector<std::string> pv_names;
    for (size_t i = 1; i < pos_args.size(); ++i) {
        pv_names.push_back(prefix + pos_args[i]);
    }

    return run_waveform_app(app, pv_names);
}
