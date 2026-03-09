#include <numeric>

#include <pv/caProvider.h>
#include <pva/client.h>

#include <ftxui/component/component.hpp>
#include <ftxui/component/loop.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>
#include <ftxui/component/event.hpp>

#include <pvtui/pvtui.hpp>

using namespace ftxui;
using namespace pvtui;

static constexpr std::string_view CLI_HELP_MSG = R"(
pvtui_sr - APS storage ring status display
Inspired by xfd-display.adl
Note: pvgateway must be set correctly. See /APSshare/bin/xfd-display

Usage:
  pvtui_sr [options]

Options:
  -h, --help        Show this help message and exit.

For more details, visit: https://github.com/BCDA-APS/pvtui
)";

std::vector<double> downsample_and_clip(const std::vector<double>& input, int target_size, double curr_min, double curr_max, double height) {
    std::vector<double> result;
    double chunk_size = static_cast<double>(input.size()) / target_size;

    for (int i = 0; i < target_size; ++i) {
        int start = static_cast<int>(i * chunk_size);
        int end = static_cast<int>((i + 1) * chunk_size);
        end = std::min(end, static_cast<int>(input.size()));
        if (start >= end) break;

        double sum = std::accumulate(input.begin() + start, input.begin() + end, 0.0);
        double avg = sum / (end - start);

        double scaled = ((avg - curr_min) / (curr_max - curr_min)) * height;
        scaled = std::clamp(scaled, 0.0, height);
        result.push_back(scaled);
    }

    return result;
}


static const std::unordered_map<int, Element> inj_status_text = {
    {0, text("Waiting for Injection") | color(Color::Red)},
    {1, text("")},
    {2, text("Swapout in Progress") | color(Color::Green)},
    {3, text("Waiting for Injection") | color(Color::Red)},
    {4, text("Stored Beam") | color(Color::Blue)},
    {5, text("")},
};

static const std::unordered_map<int, Element> shutter_status_text = {
    {0, text("Shutters Disabled") | bgcolor(Color::Red) | size(WIDTH, EQUAL, 17)},
    {1, text("Shutters Enabled") | bgcolor(Color::Green) | color(Color::Black) | size(WIDTH, EQUAL, 15)},
};


int main(int argc, char *argv[]) {

    App app(argc, argv);
    if (app.args.help(CLI_HELP_MSG)) return EXIT_SUCCESS;

    // Widgets for each PV we want on the display
    // This particular display has no interactive elements
    Monitor<std::string> time_and_date(app, "S:IOC:timeOfDayForm1SI");
    Monitor<std::string> current(app, "S-DCCT:CurrentM");
    Monitor<int> lifetime(app, "S-DCCT:LifetimeM");
    Monitor<PVEnum> injection_status(app, "S-INJ:InjectionOperationM");
    Monitor<std::string> injection_period(app, "S-INJ:InjectionPeriodCounterM");
    Monitor<PVEnum> desired_mode(app, "S:DesiredMode");
    Monitor<PVEnum> actual_mode(app, "S:ActualMode");
    Monitor<PVEnum> shutter_status(app, "RF-ACIS:FePermit:Sect1To35IdM");
    Monitor<int> num_shutters_open(app, "NoOfShuttersOpenA");
    Monitor<std::string> operators(app, "OPS:message1");
    Monitor<std::string> floor_coord(app, "OPS:message2");
    Monitor<std::string> fill_patt(app, "OPS:message3");
    Monitor<std::string> dump_reason(app, "OPS:message5");
    Monitor<std::string> dump_reason_cont(app, "OPS:message16");
    Monitor<std::string> prob_info(app, "OPS:message4");
    Monitor<std::string> prob_info_cont(app, "OPS:message14");
    Monitor<std::string> next_fill(app, "OPS:message6");
    Monitor<std::string> next_fill_cont(app, "OPS:message17");
    Monitor<std::string> next_update(app, "OPS:message18");

    // Monitor does not work right here since there isn't a way to preallocate 1440
    std::vector<double> user_ops_current(1440, 0.0);
    app.pvgroup.add("S:UserOpsCurrent");
    app.pvgroup.set_monitor("S:UserOpsCurrent", user_ops_current);

    std::vector<double> other_current(1440, 0.0);
    app.pvgroup.add("S:OtherCurrent");
    app.pvgroup.set_monitor("S:OtherCurrent", other_current);

    auto plot1_renderer = Renderer([&] {
        const double CURR_MAX = 200;
        const double CURR_MIN = 0;
        const double TARGET_HEIGHT = 50;
        const int TARGET_WIDTH = 100;
        auto c = Canvas(TARGET_WIDTH, TARGET_HEIGHT);
        std::vector<double> comp_user = downsample_and_clip(user_ops_current, TARGET_WIDTH, CURR_MIN, CURR_MAX, TARGET_HEIGHT);
        std::vector<double> comp_other = downsample_and_clip(other_current, TARGET_WIDTH, CURR_MIN, CURR_MAX, TARGET_HEIGHT);

        // "user" current
        std::vector<int> y1(comp_user.size());
        for (size_t x = 0; x < comp_user.size(); x++) {
            y1[x] = static_cast<int>(TARGET_HEIGHT-(comp_user.at(x)));
        }
        for (size_t x = 1; x < comp_user.size()-1; x++) {
            c.DrawPointLine(x, y1[x], x + 1, y1[x + 1], Color::Blue);
        }

        // "other" current
        std::vector<int> y2(comp_other.size());
        for (size_t x = 0; x < comp_other.size(); x++) {
            y2[x] = static_cast<int>(TARGET_HEIGHT-(comp_other.at(x)));
        }
        for (size_t x = 1; x < comp_other.size()-1; x++) {
            c.DrawPointLine(x, y2[x], x + 1, y2[x + 1], Color::Red);
        }

        return canvas(std::move(c));
    });

    // Container for "q" to quit
    auto main_container = Container::Vertical({});
    main_container |= CatchEvent([&](Event event) {
        if (event == Event::Character('q')) {
            app.screen.Exit();
            return true;
        }
        return false;
    });

    // Main renderer to define visual layout of components and elements
    auto main_renderer = Renderer(main_container, [&] {
        return vbox({
            text("Storage Ring Status") | borderLight | bold | size(WIDTH, EQUAL, 19),
            text(time_and_date.value()),
            separatorEmpty(),

            hbox({
                text("Current:  "),
                text(current.value()) | size(WIDTH, EQUAL, 7),
                text(" mA")
            }),
            hbox({
                text("Lifetime: "),
                text(std::to_string(lifetime.value())) | size(WIDTH, EQUAL, 7),
                text(" min")
            }),

            separatorEmpty(),
            inj_status_text.count(injection_status.value().index)
                ? inj_status_text.at(injection_status.value().index)
                : text(""),
            text("Swapout In: " + injection_period.value() + " sec."),
            separatorEmpty(),
            shutter_status_text.count(shutter_status.value().index)
                ? shutter_status_text.at(shutter_status.value().index)
                : text(""),
            text("Machine Status: " + desired_mode.value().choice),
            text("Operating Mode: " + actual_mode.value().choice),
            text("Shutters Open: " + std::to_string(num_shutters_open.value())),

            separatorEmpty(),
            text("Beam History: ") | bold | italic | underlined | size(WIDTH, EQUAL, 11),
            vbox({
                separatorEmpty(),
                hbox({
                    vbox({
                        text("200 -"),
                        filler(),
                        text("150 -"),
                        filler(),
                        text("100 -"),
                        filler(),
                        text("50  -"),
                        filler(),
                        text("0   -"),
                    }),
                    separator(),
                    separatorEmpty(),
                    plot1_renderer->Render(),
                    separatorEmpty(),
                }),
                separator(),
                hbox({
                    text("    24hr"), filler(), text("12hr"), filler(), text("0hr")
                }),
            }) | size(WIDTH, EQUAL, 57),
            separatorEmpty(),

            text("Messages from Operations: ") | bold | italic | underlined | size(WIDTH, EQUAL, 22),
            text("        Operators: " + operators.value()),
            text("Floor Coordinator: " + floor_coord.value()),
            text("     Fill Pattern: " + fill_patt.value()),
            text(" Dump/Trip Reason: " + dump_reason.value()),
            text("Trip Reason(cont): " + dump_reason_cont.value()),
            text("     Problem Info: " + prob_info.value()),
            text(" Prob Info (cont): " + prob_info_cont.value()),
            text("        Next Fill: " + next_fill.value()),
            text("  Next Fill(cont): " + next_fill_cont.value()),
            text("      Next Update: " + next_update.value()),
        });
    });

    app.run(main_renderer);

    return 0;
}
