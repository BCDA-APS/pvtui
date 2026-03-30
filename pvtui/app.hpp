#pragma once

#include <functional>
#include <iostream>
#include <string>
#include <vector>

#include <ftxui/component/component_options.hpp>
#include <ftxui/component/screen_interactive.hpp>

#include <pvtui/detail/argh.h>
#include <pvtui/pvgroup.hpp>

namespace pvtui {

/// \brief Parses command-line arguments for PVTUI applications.
class ArgParser {
  public:
    /// \brief Constructs an ArgParser from command-line arguments.
    /// \param argc Argument count.
    /// \param argv Argument values.
    /// \param extra_params Additional named parameters to register before parsing.
    ArgParser(int argc, char* argv[],
              std::initializer_list<char const* const> extra_params = {});

    /// \brief Prints a help message and returns true if help flags given
    /// \param msg The help message, as a type streamable to std::cout
    /// \return True if help flag present, false otherwise
    template <typename T>
    bool help(const T& msg) {
        if (flag("help") or flag("h")) {
            std::cout << msg << std::endl;
            return true;
        } else {
            return false;
        }
    }

    /// \brief Checks if a specific command-line flag is set.
    /// \param f The flag name (e.g., "-h", "--version").
    /// \return True if the flag is present, false otherwise.
    bool flag(const std::string& f) const;

    /// \brief Gets the value of a named command-line parameter.
    /// \param name The parameter name (e.g., "--prefix").
    /// \return The parameter value, or an empty string if not provided.
    std::string param(const std::string& name) const;

    /// \brief Get all positional arguments passed to the program
    /// \return A vector of strings of all the positional arguments
    std::vector<std::string> positional_args() const;

    std::string provider = "ca"; ///< The EPICS provider type (e.g., "ca", "pva").

  private:
    argh::parser cmdl_; ///< Internal argh parser instance.
};

/// \brief Convenience struct for managing a TUI application
///
/// This class holds the pvtui::ArgParser, pvtui::PVGroup, pvac::ClientProvider,
/// and ftxui::ScreenInteractive to reduce boilerplate in PVTUI applications.
struct App {

    /// \brief Constructs an App which internally manages a pvac::ClientProvider
    /// pvtui::PVGroup, pvtui::ArgParser, and ftxui::ScreenInteractive
    /// \param argc Command line argument count
    /// \param argv Command line arguments
    /// \param extra_params Additional named parameters to register with the ArgParser.
    App(int argc, char* argv[],
        std::initializer_list<char const* const> extra_params = {});

    /// \brief Runs the main FTXUI loop
    /// \param renderer The ftxui::Component which defines the application layout
    /// \param poll_period_ms Render loop polling period in milliseconds
    void run(const ftxui::Component& renderer, int poll_period_ms = 100);

    /// \brief The main loop function to run with App::run. Can be redefined by the user
    std::function<void(App&, const ftxui::Component&)> main_loop;

    int poll_period_ms = 100;        ///< Main loop poll period
    pvtui::ArgParser args;           ///< pvtui::ArgParser to store the cmd line arguments
    pvac::ClientProvider provider;   ///< EPICS client provider
    PVGroup pvgroup;                 ///< pvtui::PVGroup to manage PVs used in the application
    ftxui::ScreenInteractive screen; ///< screen instance for FTXUI rendering
};

} // namespace pvtui
