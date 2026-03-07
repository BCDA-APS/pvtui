#include <algorithm>
#include <chrono>
#include <sstream>
#include <thread>

#include <ftxui/component/event.hpp>
#include <ftxui/component/loop.hpp>

#include <pvtui/app.hpp>

namespace pvtui {

ArgParser::ArgParser(int argc, char* argv[]) {
    cmdl_.add_params({"-m", "--macro", "--macros"});
    cmdl_.add_params({"--provider"});
    cmdl_.parse(argc, argv);
    this->macros = get_macro_dict(cmdl_({"-m", "--macro", "--macros"}).str());
    this->provider = cmdl_("--provider").str().empty() ? "ca" : cmdl_("--provider").str();
};

bool ArgParser::macros_present(const std::vector<std::string>& macro_list) const {
    for (const auto& m : macro_list) {
        if (!this->macros.count(m)) {
            return false;
        }
    }
    return true;
};

std::string ArgParser::replace(const std::string& str) const {
    std::string out = str;
    size_t ind = 0;
    for (auto& [k, v] : this->macros) {
        std::string pholder = "$(" + k + ")";
        while ((ind = out.find(pholder)) != std::string::npos) {
            out.replace(ind, k.size() + 3, v);
        }
    }
    return out;
}

bool ArgParser::flag(const std::string& f) const { return cmdl_[f]; }

std::vector<std::string> ArgParser::split_string(const std::string& input, char delimiter) {
    std::vector<std::string> result;
    std::stringstream ss(input);
    std::string item;
    while (std::getline(ss, item, delimiter)) {
        result.push_back(std::move(item));
    }
    return result;
}

std::unordered_map<std::string, std::string> ArgParser::get_macro_dict(std::string all_macros) {
    all_macros.erase(
        std::remove_if(all_macros.begin(), all_macros.end(), [](unsigned char s) { return std::isspace(s); }),
        all_macros.end());

    std::unordered_map<std::string, std::string> map_out;
    for (const auto& m : split_string(all_macros, ',')) {
        auto pair = split_string(m, '=');
        if (pair.size() != 2) {
            return std::unordered_map<std::string, std::string>{};
        }
        map_out.emplace(std::move(pair.at(0)), std::move(pair.at(1)));
    }
    return map_out;
}

static pvac::ClientProvider init_epics_provider(const std::string& p) {
    epics::pvAccess::ca::CAClientFactory::start();
    pvac::ClientProvider provider(p);
    return provider;
}

App::App(int argc, char* argv[])
    : args(argc, argv), provider(init_epics_provider(args.provider)), pvgroup(provider),
      screen(ftxui::ScreenInteractive::Fullscreen()) {

    main_loop = [](App& app, const ftxui::Component& renderer, int ms) {
        ftxui::Loop loop(&app.screen, renderer);
        while (!loop.HasQuitted()) {
            if (app.pvgroup.sync()) {
                app.screen.PostEvent(ftxui::Event::Custom);
            }
            loop.RunOnce();
            std::this_thread::sleep_for(std::chrono::milliseconds(ms));
        }
    };
}

void App::run(const ftxui::Component& renderer, int poll_period_ms) {
    main_loop(*this, renderer, poll_period_ms);
}

} // namespace pvtui
