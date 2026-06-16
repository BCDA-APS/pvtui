#pragma once

#include <memory>
#include <string>
#include <vector>

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>

#include <pvtui/app.hpp>

namespace pvtui {

/// \brief Provides unicode characters for TUI elements.
namespace unicode {
constexpr std::string_view full_block = "█";    ///< Unicode character for a full block.
constexpr std::string_view green_circle = "🟢"; ///< Unicode character for a green circle.
constexpr std::string_view red_circle = "🔴";   ///< Unicode character for a red circle.

/// \brief Generates a vertical string of full block characters.
/// \param len The number of full blocks (lines) to generate.
/// \return A string containing `len` full block characters, each on a new line.
std::string rectangle(int len);
} // namespace unicode

/// \brief Defines the data types for PV put operations for InputWidget
enum class PVPutType {
    Integer,
    Double,
    String,
};

/// \brief Style options for ChoiceWidget
enum class ChoiceStyle {
    Vertical,
    Horizontal,
    Dropdown,
};

struct BitRange {
    size_t start;
    size_t end;
};

struct SliderRange {
    double min = 0.0;
    double max = 100.0;
    double increment = 5.0;
};

/// \brief A base class for all TUI widgets that interact with EPICS PVs.
///
/// This class provides a standard interface for managing PV connections, accessing
/// PV names, and retrieving the underlying FTXUI component.
class WidgetBase {
  public:
    virtual ~WidgetBase() = default;

    /// \brief Gets the PV name associated with the widget.
    /// \return The fully expanded PV name.
    std::string pv_name() const;

    /// \brief Gets the underlying FTXUI component for rendering.
    /// \return A valid FTXUI component.
    /// \throws std::runtime_error if the component has not been set.
    ftxui::Component component() const;

    /// \brief Checks if the widget's PV is currently connected.
    /// \return True if the PV is connected, false otherwise.
    bool connected() const;

    /// \brief Returns the widget's current PV value as a string.
    /// \return A string representation of the underlying PV value.
    virtual std::string value_as_string() const { return ""; }

  protected:
    /// \brief Constructs a WidgetBase and registers the PV with a pace::Context.
    /// \param ctxt The pace::Context used to manage PVs for this widget.
    /// \param pv_name The PV name.
    WidgetBase(pace::Context& ctxt, const std::string& pv_name);

    pace::Context& ctxt_;                                      ///< The pace::Context
    std::string pv_name_;                                   ///< The PV name.
    ftxui::Component component_;                            ///< Underlying FTXUI component.
    // std::shared_ptr<ConnectionMonitor> connection_monitor_; ///< Monitors PV connection status.
};

/// \brief An editable input field linked to a PV.
///
/// Supports typed PV put operations (int, double, string).
class InputWidget : public WidgetBase {
  public:
    /// \brief Constructs an InputWidget.
    /// \param ctxt The pace::Context managing the PVs used in this widget.
    /// \param pv_name The PV name.
    /// \param put_type Specifies how the input value is written to the PV.
    /// \param fg Optional ftxui color for the input foreground (cursor and text).
    /// \param hover Optional ftxui color for the input box's background when hovered.
    InputWidget(pace::Context& ctxt, const std::string& pv_name, PVPutType put_type,
                ftxui::Color fg = ftxui::Color::Black, ftxui::Color hover = ftxui::Color::GrayLight);

    /// \brief Constructs an InputWidget from an App class
    /// \param app A reference to the App.
    /// \param pv_name The PV name.
    /// \param put_type Specifies how the input value is written to the PV.
    /// \param fg Optional ftxui color for the input foreground (cursor and text).
    /// \param hover Optional ftxui color for the input box's background when hovered.
    InputWidget(App& app, const std::string& pv_name, PVPutType put_type,
                ftxui::Color fg = ftxui::Color::Black, ftxui::Color hover = ftxui::Color::GrayLight);

    /// \brief Gets the current value of the string displayed in the UI.
    /// \return The current string value from the UI.
    const std::string& value() const;

    std::string value_as_string() const override;

  private:
    std::shared_ptr<std::string> value_ptr_; ///< Value displayed on the UI
};

/// \brief A simple button widget that writes a fixed value when pressed.
class ButtonWidget : public WidgetBase {
  public:
    /// \brief Constructs a ButtonWidget.
    /// \param ctxt The pace::Context managing the PVs used in this widget.
    /// \param pv_name The PV name.
    /// \param label The text displayed on the button.
    /// \param op Button style option to use
    /// \param press_val The value written to the PV on press.
    ButtonWidget(pace::Context& ctxt, const std::string& pv_name, const std::string& label,
                 ftxui::ButtonOption op = ftxui::ButtonOption::Ascii(), int press_val = 1);

    /// \brief Constructs a ButtonWidget from an App class
    /// \param app A reference to the App.
    /// \param pv_name The PV name.
    /// \param label The text displayed on the button.
    /// \param op Button style option to use
    /// \param press_val The value written to the PV on press.
    ButtonWidget(App& app, const std::string& pv_name, const std::string& label,
                 ftxui::ButtonOption op = ftxui::ButtonOption::Ascii(), int press_val = 1);
};

/// \brief A read-only widget that monitors a PV. The underlying FTXUI component
/// renders the PV value as an ftxui::text() component.
/// \tparam T The C++ type used to store the PV value.
template <typename T>
class Monitor : public WidgetBase {
  public:
    /// \brief Constructs a Monitor.
    /// \param ctxt The pace::Context managing the PVs used in this widget.
    /// \param pv_name The PV name.
    Monitor(pace::Context& ctxt, const std::string& pv_name)
        : WidgetBase(ctxt, pv_name), value_ptr_(std::make_shared<T>()) {
        ctxt.bind(*value_ptr_, pv_name_);
        component_ = monitor_component_;
    }

    /// \brief Constructs a Monitor from an App class
    /// \param app A reference to the App.
    /// \param pv_name The PV name.
    Monitor(App& app, const std::string& pv_name)
        : WidgetBase(app.context, pv_name), value_ptr_(std::make_shared<T>()) {
        app.context.bind(*value_ptr_, pv_name_);
        component_ = monitor_component_;
    }

    /// \brief Gets the current value of the variable for use with the UI.
    /// \return The current value stored in the widget.
    const T& value() const { return *value_ptr_; };

    std::string value_as_string() const override {
        if constexpr (std::is_same_v<T, std::string>) {
            return *value_ptr_;
        } else if constexpr (std::is_arithmetic_v<T>) {
            return std::to_string(*value_ptr_);
        } else if constexpr (std::is_same_v<T, pace::Enum>) {
            return value_ptr_->choice;
        } else {
            return "<" + pv_name() + ">";
        }
    }

  private:
    std::shared_ptr<T> value_ptr_;

    ftxui::Component monitor_component_ = ftxui::Renderer([this] { return ftxui::text(value_as_string()); });
};

/// \brief A widget to display an integer in binary
class BitsWidget : public WidgetBase {
  public:
    /// \brief Constructs a BitsWidget.
    /// \param ctxt The pace::Context managing the PVs used in this widget.
    /// \param pv_name The PV name.
    /// \param range Bit range to display (start inclusive, end exclusive).
    /// \param color_on Color for set bits.
    /// \param color_off Color for unset bits.
    BitsWidget(pace::Context& ctxt, const std::string& pv_name, BitRange range,
               ftxui::Color color_on = ftxui::Color::Green,
               ftxui::Color color_off = ftxui::Color::GrayDark);

    /// \brief Constructs a BitsWidget from an App class.
    /// \param app A reference to the App.
    /// \param pv_name The PV name.
    /// \param range Bit range to display (start inclusive, end exclusive).
    /// \param color_on Color for set bits.
    /// \param color_off Color for unset bits.
    BitsWidget(App& app, const std::string& pv_name, BitRange range,
               ftxui::Color color_on = ftxui::Color::Green,
               ftxui::Color color_off = ftxui::Color::GrayDark);

    /// \brief Gets the current integer value displayed in the UI.
    /// \return The current integer value from the UI.
    const int& value() const;

    std::string value_as_string() const override;

  private:
    std::shared_ptr<int> value_ptr_;
};

/// \brief A widget for selecting an enum-style PV value from a list.
///
/// Supports vertical, horizontal, or dropdown layout styles.
class ChoiceWidget : public WidgetBase {
  public:
    /// \brief Constructs a ChoiceWidget.
    /// \param ctxt The pace::Context managing the PVs used in this widget.
    /// \param pv_name The PV name.
    /// \param style Layout style (vertical, horizontal, dropdown).
    ChoiceWidget(pace::Context& ctxt, const std::string& pv_name, ChoiceStyle style);

    /// \brief Constructs a ChoiceWidget from an App class
    /// \param app A reference to the App.
    /// \param pv_name The PV name.
    /// \param style Layout style (vertical, horizontal, dropdown).
    ChoiceWidget(App& app, const std::string& pv_name, ChoiceStyle style);

    /// \brief Gets the current enum value displayed in the UI.
    /// \return The current pace::Enum value from the UI.
    const pace::Enum& value() const;

    std::string value_as_string() const override;

  private:
    std::shared_ptr<pace::Enum> value_ptr_;
};

/// \brief A slider widget bound to a numeric PV.
class SliderWidget : public WidgetBase {
  public:

    /// \brief Constructs a SliderWidget.
    /// \param ctxt The pace::Context managing the PVs used in this widget.
    /// \param pv_name The PV name.
    /// \param range Slider range (min, max, increment).
    /// \param color_active Color of the filled portion when the slider is focused.
    /// \param color_inactive Color of the filled portion when the slider is not focused.
    SliderWidget(pace::Context& ctxt, const std::string& pv_name, SliderRange range = {},
                 ftxui::Color color_active = ftxui::Color::White,
                 ftxui::Color color_inactive = ftxui::Color::GrayDark);

    /// \brief Constructs a SliderWidget from an App class.
    /// \param app A reference to the App.
    /// \param pv_name The PV name.
    /// \param range Slider range (min, max, increment).
    /// \param color_active Color of the filled portion when the slider is focused.
    /// \param color_inactive Color of the filled portion when the slider is not focused.
    SliderWidget(App& app, const std::string& pv_name, SliderRange range = {},
                 ftxui::Color color_active = ftxui::Color::White,
                 ftxui::Color color_inactive = ftxui::Color::GrayDark);

    /// \brief Gets the current slider value.
    /// \return The current slider (double) value from the UI.
    const double& value() const;

    std::string value_as_string() const override;

  private:
    std::shared_ptr<double> value_ptr_;
};

/// \brief Functions to generate FTXUI decorators for EPICS-style UI elements.
/// To align stylistically with MEDM, caQtDM etc, when PVs are disconnected, the widget
/// is drawn as a white rectangle
namespace EPICSColor {

/// \brief White foreground and background for disconnected widgets
static const ftxui::Decorator WHITE_ON_WHITE = bgcolor(ftxui::Color::White) | color(ftxui::Color::White);

/// \brief Light blue with black text for editable controls
inline ftxui::Decorator edit(const WidgetBase& w) {
    return w.connected() ? ftxui::bgcolor(ftxui::Color::RGB(87, 202, 228)) | ftxui::color(ftxui::Color::Black)
                         : WHITE_ON_WHITE;
}

/// \brief Dark green with white text for "related display" menus
inline ftxui::Decorator menu(const WidgetBase& w) {
    return w.connected() ? ftxui::bgcolor(ftxui::Color::RGB(16, 105, 25)) | ftxui::color(ftxui::Color::White)
                         : WHITE_ON_WHITE;
}

/// \brief Dark blue text on gray background for readbacks
inline ftxui::Decorator readback(const WidgetBase& w) {
    return w.connected()
               ? ftxui::bgcolor(ftxui::Color::RGB(196, 196, 196)) | ftxui::color(ftxui::Color::DarkBlue)
               : WHITE_ON_WHITE;
}

/// \brief Pinkish/purple with black text for links
inline ftxui::Decorator link(const WidgetBase& w) {
    return w.connected()
               ? ftxui::bgcolor(ftxui::Color::RGB(148, 148, 228)) | ftxui::color(ftxui::Color::Black)
               : WHITE_ON_WHITE;
}

/// \brief A custom color
inline ftxui::Decorator custom(const WidgetBase& w, ftxui::Decorator style) {
    return w.connected() ? style : WHITE_ON_WHITE;
}

/// \brief Default gray background color
inline ftxui::Decorator background() { return ftxui::bgcolor(ftxui::Color::RGB(196, 196, 196)); }
} // namespace EPICSColor

} // namespace pvtui
