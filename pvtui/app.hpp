#pragma once

#include <functional>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include <ftxui/component/component_options.hpp>
#include <ftxui/component/screen_interactive.hpp>

#include <pvtui/detail/argh.h>
#include <pvtui/pvgroup.hpp>

namespace pvtui {

/**
 * @brief Parses command-line arguments for PVTUI applications.
 *
 * Handles macro definitions similar to MEDM, and any additional flags
 */
class ArgParser {
  public:
    /**
     * @brief Constructs an ArgParser from command-line arguments.
     * @param argc Argument count.
     * @param argv Argument values.
     */
    ArgParser(int argc, char* argv[]);

    /**
     * @brief Checks if all specified macros are present in the parsed arguments.
     * @param macro_list A list of macro names to check.
     * @return True if all macros are present, false otherwise.
     */
    bool macros_present(const std::vector<std::string>& macro_list) const;

    /**
     * @brief Prints a help message and returns true if help flags given
     * @param msg The help message, as a type streamable to std::cout
     * @return True if help flag present, false otherwise
     */
    template <typename T>
    bool help(const T& msg) {
        if (flag("help") or flag("h")) {
            std::cout << msg << std::endl;
            return true;
        } else {
            return false;
        }
    }

    /**
     * @brief Checks if a specific command-line flag is set.
     * @param f The flag name (e.g., "-h", "--version").
     * @return True if the flag is present, false otherwise.
     */
    bool flag(const std::string& f) const;

    /**
     * @brief Replaces macros in a string with their corresponding values.
     * @param str A string with macros like $(P), $(R), etc.
     * @return A new string with all macros replaced by their values.
     */
    std::string replace(const std::string& str) const;

    /**
     * @brief Get all positional arguments passed to the program
     * @return A vector of strings of all the positional arguments
     */
    std::vector<std::string> positional_args() const;

    std::unordered_map<std::string, std::string> macros; ///< Parsed macros (e.g., "P=VAL").
    std::string provider = "ca";                         ///< The EPICS provider type (e.g., "ca", "pva").

  private:
    argh::parser cmdl_; ///< Internal argh parser instance.

    /**
     * @brief Splits a string by a given delimiter.
     * @param input The string to split.
     * @param delimiter The character to split by.
     * @return A vector of substrings.
     */
    std::vector<std::string> split_string(const std::string& input, char delimiter);

    /**
     * @brief Creates a map of macro names to values from a string similar to MEDM or caQtDM.
     * @param all_macros A string like "P=xxx:,M=m1".
     * @return An unordered map of macro names to values. Returns an empty map on parse error.
     */
    std::unordered_map<std::string, std::string> get_macro_dict(std::string all_macros);
};

/**
 * @brief Convenience struct for managing a TUI application
 *
 * This class holds the pvtui::ArgParser, pvtui::PVGroup, pvac::ClientProvider,
 * and ftxui::ScreenInteractive to reduce boilerplate in PVTUI applications.
 */
struct App {

    /**
     * @brief Constructs an App which internally manages a pvac::ClientProvider
     * pvtui::PVGroup, pvtui::ArgParser, and ftxui::ScreenInteractive
     * @param argc Command line argument count
     * @param argv Command line arguments
     */
    App(int argc, char* argv[]);

    /**
     * @brief Runs the main FTXUI loop
     * @param renderer The ftxui::Component which defines the application layout
     * @param poll_period_ms Render loop polling period in milliseconds
     */
    void run(const ftxui::Component& renderer, int poll_period_ms = 100);

    /// @brief The main loop function to run with App::run. Can be redefined by the user
    std::function<void(App&, const ftxui::Component&, int)> main_loop;

    pvtui::ArgParser args;           ///< pvtui::ArgParser to store the cmd line arguments
    pvac::ClientProvider provider;   ///< EPICS client provider
    PVGroup pvgroup;                 ///< pvtui::PVGroup to manage PVs used in the application
    ftxui::ScreenInteractive screen; ///< screen instance for FTXUI rendering
};

} // namespace pvtui
