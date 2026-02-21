#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/loop.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>

#include <pvtui/argh.h>
#include <pvtui/pvgroup.hpp>

namespace pvtui {

/**
 * @brief Provides unicode characters for TUI elements.
 */
namespace unicode {
constexpr std::string_view full_block = "█";    ///< Unicode character for a full block.
constexpr std::string_view green_circle = "🟢"; ///< Unicode character for a green circle.
constexpr std::string_view red_circle = "🔴";   ///< Unicode character for a red circle.

/**
 * @brief Generates a vertical string of full block characters.
 * @param len The number of full blocks (lines) to generate.
 * @return A string containing `len` full block characters, each on a new line.
 */
std::string rectangle(int len);
} // namespace unicode

/**
 * @brief Defines the data types for PV put operations for InputWidget
 */
enum class PVPutType {
    Integer,
    Double,
    String,
};

/**
 * @brief Style options for ChoiceWidget
 */
enum class ChoiceStyle {
    Vertical,
    Horizontal,
    Dropdown,
};

///< Type alias for input transformation function.
using InputTransform = std::function<ftxui::Element(ftxui::InputState)>;

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
    ArgParser(int argc, char *argv[]);

    /**
     * @brief Checks if all specified macros are present in the parsed arguments.
     * @param macro_list A list of macro names to check.
     * @return True if all macros are present, false otherwise.
     */
    bool macros_present(const std::vector<std::string> &macro_list) const;

    /**
     * @brief Prints a help message and returns true if help flags given
     * @param msg The help message, as a type streamable to std::cout
     * @return True if help flag present, false otherwise
     */
    template <typename T> bool help(const T &msg) {
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
    bool flag(const std::string &f) const;

    /**
     * @brief Replaces macros in a string with their corresponding values.
     * @param str A string with macros like $(P), $(R), etc.
     * @return A new string with all macros replaced by their values.
     */
    std::string replace(const std::string &str) const;

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
    std::vector<std::string> split_string(const std::string &input, char delimiter);

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
    App(int argc, char *argv[]);

    /**
     * @brief Runs the main FTXUI loop
     * @param renderer The ftxui::Component which defines the application layout
     * @param poll_period_ms Render loop polling period in milliseconds
     */
    void run(const ftxui::Component &renderer, int poll_period_ms = 100);

    /// @brief The main loop function to run with App::run. Can be redefined by the user
    std::function<void(App &, const ftxui::Component &, int)> main_loop;

    pvtui::ArgParser args;           ///< pvtui::ArgParser to store the cmd line arguments
    pvac::ClientProvider provider;   ///< EPICS client provider
    PVGroup pvgroup;                 ///< pvtui::PVGroup to manage PVs used in the application
    ftxui::ScreenInteractive screen; ///< screen instance for FTXUI rendering
};

/**
 * @brief A base class for all TUI widgets that interact with EPICS PVs.
 *
 * This class provides a standard interface for managing PV connections, accessing
 * PV names, and retrieving the underlying FTXUI component.
 */
class WidgetBase {
  public:
    virtual ~WidgetBase() = default;

    /**
     * @brief Gets the PV name associated with the widget.
     * @return The fully expanded PV name.
     */
    std::string pv_name() const;

    /**
     * @brief Gets the underlying FTXUI component for rendering.
     * @return A valid FTXUI component.
     * @throws std::runtime_error if the component has not been set.
     */
    ftxui::Component component() const;

    /**
     * @brief Checks if the widget's PV is currently connected.
     * @return True if the PV is connected, false otherwise.
     */
    bool connected() const;

  protected:
    /**
     * @brief Constructs a WidgetBase and registers the PV with a PVGroup.
     *
     * This constructor uses an ArgParser to expand any macros in the PV name.
     * @param pvgroup The PVGroup used to manage PVs for this widget.
     * @param args The ArgParser for macro expansion.
     * @param pv_name The macro-style PV name (e.g., "$(P)$(R)VAL").
     */
    WidgetBase(PVGroup &pvgroup, const ArgParser &args, const std::string &pv_name);

    /**
     * @brief Constructs a WidgetBase with a fully-expanded PV name.
     * @param pvgroup The PVGroup used to manage PVs for this widget.
     * @param pv_name The fully-expanded PV name.
     */
    WidgetBase(PVGroup &pvgroup, const std::string &pv_name);

    PVGroup &pvgroup_;                                      ///< The PVGroup
    std::string pv_name_;                                   ///< The PV name.
    ftxui::Component component_;                            ///< Underlying FTXUI component.
    bool connected_;                                        ///< Boolean for PV connection status
    std::shared_ptr<ConnectionMonitor> connection_monitor_; ///< Monitors PV connection status.
};

/**
 * @brief An editable input field linked to a PV.
 *
 * Supports typed PV put operations (int, double, string).
 */
class InputWidget : public WidgetBase {
  public:
    /**
     * @brief Constructs an InputWidget with macro expansion.
     * @param pvgroup The PVGroup managing the PVs used in this widget.
     * @param args ArgParser for macro replacement.
     * @param pv_name The PV name with macros, e.g. "$(P)$(M).VAL".
     * @param put_type Specifies how the input value is written to the PV.
     * @param tf optional ftxui transformation function
     */
    InputWidget(PVGroup &pvgroup, const ArgParser &args, const std::string &pv_name, PVPutType put_type,
                InputTransform tf = nullptr);

    /**
     * @brief Constructs an InputWidget with an already expanded PV name.
     * @param pvgroup The PVGroup managing the PVs used in this widget.
     * @param pv_name The PV name.
     * @param put_type Specifies how the input value is written to the PV.
     */
    InputWidget(PVGroup &pvgroup, const std::string &pv_name, PVPutType put_type);

    /**
     * @brief Constructs an InputWidget from an App class
     * @param app A reference to the App.
     * @param pv_name The PV name.
     * @param put_type Specifies how the input value is written to the PV.
     */
    InputWidget(App &app, const std::string &pv_name, PVPutType put_type);

    /**
     * @brief Gets the current value of the string displayed in the UI.
     * @return The current string value from the UI.
     */
    const std::string &value() const;

  private:
    std::shared_ptr<std::string> value_ptr_; ///< Value displayed on the UI
};

/**
 * @brief A simple button widget that writes a fixed value when pressed.
 */
class ButtonWidget : public WidgetBase {
  public:
    /**
     * @brief Constructs a ButtonWidget with macro expansion.
     * @param pvgroup The PVGroup managing the PVs used in this widget.
     * @param args ArgParser for macro replacement.
     * @param pv_name The PV name with macros, e.g. "$(P)$(M).VAL".
     * @param label The text displayed on the button.
     * @param press_val The value written to the PV on press.
     */
    ButtonWidget(PVGroup &pvgroup, const ArgParser &args, const std::string &pv_name,
                 const std::string &label, int press_val = 1);

    /**
     * @brief Constructs a ButtonWidget with an expanded PV name.
     * @param pvgroup The PVGroup managing the PVs used in this widget.
     * @param pv_name The PV name.
     * @param label The text displayed on the button.
     * @param press_val The value written to the PV on press.
     */
    ButtonWidget(PVGroup &pvgroup, const std::string &pv_name, const std::string &label, int press_val = 1);

    /**
     * @brief Constructs a ButtonWidget from an App class
     * @param app A reference to the App.
     * @param pv_name The PV name.
     * @param label The text displayed on the button.
     * @param press_val The value written to the PV on press.
     */
    ButtonWidget(App &app, const std::string &pv_name, const std::string &label, int press_val = 1);
};

/**
 * @brief A read-only widget that monitors a PV but has no visible UI component.
 *
 * This is useful for tracking a PV's value (e.g., a readback) and making it
 * available to other parts of the application without rendering it directly.
 * @tparam T The C++ type used to store the PV value.
 */
template <typename T> class VarWidget : public WidgetBase {
  public:
    /**
     * @brief Constructs a VarWidget with macro expansion.
     * @param pvgroup The PVGroup managing the PVs used in this widget.
     * @param args ArgParser for macro replacement.
     * @param pv_name The PV name with macros, e.g. "$(P)$(M).VAL".
     */
    VarWidget(PVGroup &pvgroup, const ArgParser &args, const std::string &pv_name)
        : WidgetBase(pvgroup, args, pv_name), value_ptr_(std::make_shared<T>()) {
        pvgroup.set_monitor(pv_name_, *value_ptr_);
    }

    /**
     * @brief Constructs a VarWidget with a fully expanded PV name.
     * @param pvgroup The PVGroup managing the PVs used in this widget.
     * @param pv_name The PV name.
     */
    VarWidget(PVGroup &pvgroup, const std::string &pv_name)
        : WidgetBase(pvgroup, pv_name), value_ptr_(std::make_shared<T>()) {
        pvgroup.set_monitor(pv_name_, *value_ptr_);
    }

    /**
     * @brief Constructs a VarWidget from an App class
     * @param app A reference to the App.
     * @param pv_name The PV name.
     */
    VarWidget(App &app, const std::string &pv_name)
        : WidgetBase(app.pvgroup, app.args, pv_name), value_ptr_(std::make_shared<T>()) {
        app.pvgroup.set_monitor(pv_name_, *value_ptr_);
    }

    /**
     * @brief Gets the current value of the variable for use with the UI.
     * @return The current value stored in the widget.
     */
    const T &value() const { return *value_ptr_; };

    /**
     * @brief This widget does not have a UI element, so the component method is deleted.
     */
    ftxui::Component component() const = delete;

  private:
    std::shared_ptr<T> value_ptr_;
};

/**
 * @brief A widget to display an integer in binary
 */
class BitsWidget : public WidgetBase {
  public:
    /**
     * @brief Constructs a BitsWidget with macro expansion.
     * @param pvgroup The PVGroup managing the PVs used in this widget.
     * @param args ArgParser for macro replacement.
     * @param pv_name The PV name with macros, e.g. "$(P)$(M).VAL".
     * @param labels String labels to draw next to each bit.
     */
    BitsWidget(PVGroup &pvgroup, const ArgParser &args, const std::string &pv_name, size_t nbits);

    /**
     * @brief Constructs a BitsWidget without macros.
     * @param pvgroup The PVGroup managing the PVs used in this widget.
     * @param pv_name The PV name.
     * @param labels String labels to draw next to each bit.
     */
    BitsWidget(App &app, const std::string &pv_name, size_t nbits);

    /**
     * @brief Gets the current integer value displayed in the UI.
     * @return The current integer value from the UI.
     */
    const int &value() const;

  private:
    std::shared_ptr<int> value_ptr_;
};

/**
 * @brief A widget for selecting an enum-style PV value from a list.
 *
 * Supports vertical, horizontal, or dropdown layout styles.
 */
class ChoiceWidget : public WidgetBase {
  public:
    /**
     * @brief Constructs a ChoiceWidget with macro expansion.
     * @param pvgroup The PVGroup managing the PVs used in this widget.
     * @param args ArgParser for macro replacement.
     * @param pv_name The PV name with macros, e.g. "$(P)$(M).VAL".
     * @param style Layout style (vertical, horizontal, dropdown).
     */
    ChoiceWidget(PVGroup &pvgroup, const ArgParser &args, const std::string &pv_name, ChoiceStyle style);

    /**
     * @brief Constructs a ChoiceWidget with a PV name without macros.
     * @param pvgroup The PVGroup managing the PVs used in this widget.
     * @param pv_name The PV name.
     * @param style Layout style (vertical, horizontal, dropdown).
     */
    ChoiceWidget(PVGroup &pvgroup, const std::string &pv_name, ChoiceStyle style);

    /**
     * @brief Constructs a ChoiceWidget from an App class
     * @param app A reference to the App.
     * @param pv_name The PV name.
     * @param style Layout style (vertical, horizontal, dropdown).
     */
    ChoiceWidget(App &app, const std::string &pv_name, ChoiceStyle style);

    /**
     * @brief Gets the current enum value displayed in the UI.
     * @return The current PVEnum value from the UI.
     */
    const PVEnum &value() const;

  private:
    std::shared_ptr<PVEnum> value_ptr_;
};

/**
 * @brief Functions to generate FTXUI decorators for EPICS-style UI elements.
 * To align stylistically with MEDM, caQtDM etc, when PVs are disconnected, the widget
 * is drawn as a white rectangle
 */
namespace EPICSColor {

/// @brief White foreground and background for disconnected widgets
static const ftxui::Decorator WHITE_ON_WHITE = bgcolor(ftxui::Color::White) | color(ftxui::Color::White);

/// @brief Light blue with black text for editable controls
inline ftxui::Decorator edit(const WidgetBase &w) {
    return w.connected() ? ftxui::bgcolor(ftxui::Color::RGB(87, 202, 228)) | ftxui::color(ftxui::Color::Black)
                         : WHITE_ON_WHITE;
}

/// @brief Dark green with white text for "related display" menus
inline ftxui::Decorator menu(const WidgetBase &w) {
    return w.connected() ? ftxui::bgcolor(ftxui::Color::RGB(16, 105, 25)) | ftxui::color(ftxui::Color::White)
                         : WHITE_ON_WHITE;
}

/// @brief Dark blue text on gray background for readbacks
inline ftxui::Decorator readback(const WidgetBase &w) {
    return w.connected()
               ? ftxui::bgcolor(ftxui::Color::RGB(196, 196, 196)) | ftxui::color(ftxui::Color::DarkBlue)
               : WHITE_ON_WHITE;
}

/// @brief Pinkish/purple with black text for links
inline ftxui::Decorator link(const WidgetBase &w) {
    return w.connected()
               ? ftxui::bgcolor(ftxui::Color::RGB(148, 148, 228)) | ftxui::color(ftxui::Color::Black)
               : WHITE_ON_WHITE;
}

/// @brief A custom color
inline ftxui::Decorator custom(const WidgetBase &w, ftxui::Decorator style) {
    return w.connected() ? style : WHITE_ON_WHITE;
}

/// @ Default gray background color
inline ftxui::Decorator background() { return ftxui::bgcolor(ftxui::Color::RGB(196, 196, 196)); }
} // namespace EPICSColor

} // namespace pvtui
