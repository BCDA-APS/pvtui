#include <pvtui/app.hpp>

namespace pvtui {

ArgParser::ArgParser(int argc, char* argv[], std::initializer_list<char const* const> extra_params) {
    cmdl_.add_params({"--provider"});
    if (extra_params.size() > 0)
        cmdl_.add_params(extra_params);
    cmdl_.parse(argc, argv);
    this->provider = cmdl_("--provider").str().empty() ? "ca" : cmdl_("--provider").str();
}

std::vector<std::string> ArgParser::positional_args() const { return cmdl_.pos_args(); }

bool ArgParser::flag(const std::string& f) const { return cmdl_[f]; }

std::string ArgParser::param(const std::string& name) const { return cmdl_(name).str(); }

App::App(int argc, char* argv[], std::initializer_list<char const* const> extra_params)
    : args(argc, argv, extra_params), context(args.provider),
      screen(ftxui::ScreenInteractive::Fullscreen()) {

    main_loop = [](App& app, const ftxui::Component& renderer) {
        ftxui::Loop loop(&app.screen, renderer);
        while (!loop.HasQuitted()) {
            if (app.context.sync()) {
                app.screen.PostEvent(ftxui::Event::Custom);
            }
            loop.RunOnce();
            std::this_thread::sleep_for(std::chrono::milliseconds(app.poll_period_ms));
        }
    };
}

void App::run(const ftxui::Component& renderer, int poll_period_ms) {
    this->poll_period_ms = poll_period_ms;
    main_loop(*this, renderer);
}

} // namespace pvtui
