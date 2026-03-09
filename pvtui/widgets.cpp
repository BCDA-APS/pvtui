#include <stdexcept>

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_options.hpp>

#include <pvtui/widgets.hpp>

namespace pvtui {

namespace unicode {

std::string rectangle(int len) {
    std::string out;
    out.reserve(len * full_block.length());
    for (int i = 0; i < len; i++) {
        out.append(full_block);
        out.append("\n");
    }
    return out;
}

} // namespace unicode

namespace {

ftxui::Component make_button_widget(PVHandler& pv, const std::string& label, int value) {
    auto op = ftxui::ButtonOption::Ascii();
    op.label = label;
    op.on_click = [&pv, value]() {
        if (pv.connected()) {
            pv.channel.put().set("value", value).exec();
        }
    };
    return ftxui::Button(op);
}

template <typename T>
bool put_string_as(std::string_view str, PVHandler& pv) {
    T val{};
    try {
        if constexpr (std::is_same_v<T, double>) {
            val = std::stod(str.data());
        } else if constexpr (std::is_same_v<T, int>) {
            val = std::stoi(str.data());
        } else if constexpr (std::is_same_v<T, std::string>) {
            val = str;
        }
        pv.channel.put().set("value", val).exec();
    } catch (...) {
        return false;
    }
    return true;
}

ftxui::Component make_input_widget(PVHandler& pv, std::string& disp_str, PVPutType put_type, ftxui::Color fg,
                                   ftxui::Color hover) {

    ftxui::InputOption input_op;

    input_op.content = &disp_str;
    input_op.multiline = false;

    input_op.transform = [&pv, &disp_str, fg, hover](ftxui::InputState s) {
        if (not pv.connected()) {
            disp_str = "";
        }
        if (s.is_placeholder) {
            s.element |= ftxui::dim;
        }
        if (s.focused) {
            s.element |= ftxui::inverted;
        } else if (s.hovered) {
            s.element |= ftxui::bgcolor(hover);
        }
        return s.element | ftxui::color(fg);
    };

    input_op.on_enter = [&pv, &disp_str, put_type]() {
        if (pv.connected()) {
            if (put_type == PVPutType::Double) {
                put_string_as<double>(disp_str, pv);
            } else if (put_type == PVPutType::Integer) {
                put_string_as<int>(disp_str, pv);
            } else if (put_type == PVPutType::String) {
                put_string_as<std::string>(disp_str, pv);
            }
        }
    };

    return ftxui::Input(input_op);
}

ftxui::Component make_choice_h_widget(PVHandler& pv, const std::vector<std::string>& labels, int& selected) {
    ftxui::MenuOption op = ftxui::MenuOption::Toggle();
    op.entries = &labels;
    op.selected = &selected;
    op.on_change = [&]() {
        if (pv.connected()) {
            pv.channel.put().set("value.index", selected).exec();
        }
    };
    return ftxui::Menu(op);
}

ftxui::Component make_choice_v_widget(PVHandler& pv, const std::vector<std::string>& labels, int& selected) {
    ftxui::MenuOption op = ftxui::MenuOption::Vertical();
    op.entries = &labels;
    op.selected = &selected;
    op.on_change = [&]() {
        if (pv.connected()) {
            pv.channel.put().set("value.index", selected).exec();
        }
    };
    op.entries_option.transform = [&pv](const ftxui::EntryState& state) {
        ftxui::Element e = pv.connected() ? ftxui::text(state.label) : ftxui::text("    ");
        if (state.focused) {
            e |= ftxui::inverted;
        }
        if (state.active) {
            e |= ftxui::bold;
        }
        if (!state.focused && !state.active) {
            e |= ftxui::dim;
        }
        return e;
    };
    return ftxui::Menu(op);
}

ftxui::Component make_dropdown_widget(PVHandler& pv, const std::vector<std::string>& labels, int& selected) {
    using namespace ftxui;

    DropdownOption dropdown_op;

    dropdown_op.radiobox.entries = &labels;
    dropdown_op.radiobox.selected = &selected;
    dropdown_op.radiobox.on_change = [&]() {
        if (pv.connected()) {
            pv.channel.put().set("value.index", selected).exec();
        }
    };

    dropdown_op.transform = [](bool open, ftxui::Element checkbox, ftxui::Element radiobox) {
        if (open) {
            return ftxui::vbox({
                checkbox | inverted,
                radiobox | vscroll_indicator | frame | size(HEIGHT, LESS_THAN, 10),
                filler(),
            });
        }
        return vbox({
            checkbox,
            filler(),
        });
    };
    return ftxui::Dropdown(dropdown_op);
}

ftxui::Component make_bits_widget(int& value, size_t nbits) {
    using namespace ftxui;
    return Renderer([&value, nbits] {
        Elements rows;
        for (size_t i = 0; i < nbits; i++) {
            int v = value & (1u << i);
            auto clr = v ? color(Color::Green) : color(Color::GrayDark);
            rows.push_back(text(unicode::rectangle(2)) | clr);
        }
        return vbox({rows});
    });
}

} // namespace

WidgetBase::WidgetBase(PVGroup& pvgroup, const ArgParser& args, const std::string& pv_name)
    : pvgroup_(pvgroup), pv_name_(args.replace(pv_name)) {
    pvgroup.add(pv_name_);
    connection_monitor_ = pvgroup[pv_name_].get_connection_monitor();
}

WidgetBase::WidgetBase(PVGroup& pvgroup, const std::string& pv_name) : pvgroup_(pvgroup), pv_name_(pv_name) {
    pvgroup.add(pv_name_);
    connection_monitor_ = pvgroup[pv_name_].get_connection_monitor();
}

std::string WidgetBase::pv_name() const { return pv_name_; }

bool WidgetBase::connected() const { return connection_monitor_->connected(); }

ftxui::Component WidgetBase::component() const {
    if (component_) {
        return component_;
    } else {
        throw std::runtime_error("No component defined for " + pv_name_);
    }
}

InputWidget::InputWidget(PVGroup& pvgroup, const ArgParser& args, const std::string& pv_name,
                         PVPutType put_type, ftxui::Color fg, ftxui::Color hover)
    : WidgetBase(pvgroup, args, pv_name), value_ptr_(std::make_shared<std::string>()) {
    pvgroup.set_monitor(pv_name_, *value_ptr_);
    component_ = make_input_widget(pvgroup.get_pv(pv_name_), *value_ptr_, put_type, fg, hover);
}

InputWidget::InputWidget(App& app, const std::string& pv_name, PVPutType put_type, ftxui::Color fg,
                         ftxui::Color hover)
    : WidgetBase(app.pvgroup, app.args, pv_name), value_ptr_(std::make_shared<std::string>()) {
    app.pvgroup.set_monitor(pv_name_, *value_ptr_);
    component_ = make_input_widget(app.pvgroup.get_pv(pv_name_), *value_ptr_, put_type, fg, hover);
}

InputWidget::InputWidget(PVGroup& pvgroup, const std::string& pv_name, PVPutType put_type, ftxui::Color fg,
                         ftxui::Color hover)
    : WidgetBase(pvgroup, pv_name), value_ptr_(std::make_shared<std::string>()) {
    pvgroup.set_monitor(pv_name_, *value_ptr_);
    component_ = make_input_widget(pvgroup.get_pv(pv_name_), *value_ptr_, put_type, fg, hover);
}

const std::string& InputWidget::value() const { return *value_ptr_; }

BitsWidget::BitsWidget(PVGroup& pvgroup, const ArgParser& args, const std::string& pv_name, size_t nbits)
    : WidgetBase(pvgroup, args, pv_name), value_ptr_(std::make_shared<int>()) {
    pvgroup.set_monitor(pv_name_, *value_ptr_);
    component_ = make_bits_widget(*value_ptr_, nbits);
}

BitsWidget::BitsWidget(PVGroup& pvgroup, const std::string& pv_name, size_t nbits)
    : WidgetBase(pvgroup, pv_name), value_ptr_(std::make_shared<int>()) {
    pvgroup.set_monitor(pv_name_, *value_ptr_);
    component_ = make_bits_widget(*value_ptr_, nbits);
}

BitsWidget::BitsWidget(App& app, const std::string& pv_name, size_t nbits)
    : WidgetBase(app.pvgroup, app.args, pv_name), value_ptr_(std::make_shared<int>()) {
    app.pvgroup.set_monitor(pv_name_, *value_ptr_);
    component_ = make_bits_widget(*value_ptr_, nbits);
}

const int& BitsWidget::value() const { return *value_ptr_; }

ChoiceWidget::ChoiceWidget(PVGroup& pvgroup, const ArgParser& args, const std::string& pv_name,
                           ChoiceStyle style)
    : WidgetBase(pvgroup, args, pv_name), value_ptr_(std::make_shared<PVEnum>()) {
    pvgroup.set_monitor(pv_name_, *value_ptr_);
    switch (style) {
    case pvtui::ChoiceStyle::Vertical:
        component_ = make_choice_v_widget(pvgroup.get_pv(pv_name_), value_ptr_->choices, value_ptr_->index);
        break;
    case pvtui::ChoiceStyle::Horizontal:
        component_ = make_choice_h_widget(pvgroup.get_pv(pv_name_), value_ptr_->choices, value_ptr_->index);
        break;
    case pvtui::ChoiceStyle::Dropdown:
        component_ = make_dropdown_widget(pvgroup.get_pv(pv_name_), value_ptr_->choices, value_ptr_->index);
        break;
    }
}

ChoiceWidget::ChoiceWidget(App& app, const std::string& pv_name, ChoiceStyle style)
    : WidgetBase(app.pvgroup, app.args, pv_name), value_ptr_(std::make_shared<PVEnum>()) {
    app.pvgroup.set_monitor(pv_name_, *value_ptr_);
    switch (style) {
    case pvtui::ChoiceStyle::Vertical:
        component_ =
            make_choice_v_widget(app.pvgroup.get_pv(pv_name_), value_ptr_->choices, value_ptr_->index);
        break;
    case pvtui::ChoiceStyle::Horizontal:
        component_ =
            make_choice_h_widget(app.pvgroup.get_pv(pv_name_), value_ptr_->choices, value_ptr_->index);
        break;
    case pvtui::ChoiceStyle::Dropdown:
        component_ =
            make_dropdown_widget(app.pvgroup.get_pv(pv_name_), value_ptr_->choices, value_ptr_->index);
        break;
    }
}

ChoiceWidget::ChoiceWidget(PVGroup& pvgroup, const std::string& pv_name, ChoiceStyle style)
    : WidgetBase(pvgroup, pv_name), value_ptr_(std::make_shared<PVEnum>()) {
    pvgroup.set_monitor(pv_name_, *value_ptr_);
    switch (style) {
    case pvtui::ChoiceStyle::Vertical:
        component_ = make_choice_v_widget(pvgroup.get_pv(pv_name_), value_ptr_->choices, value_ptr_->index);
        break;
    case pvtui::ChoiceStyle::Horizontal:
        component_ = make_choice_h_widget(pvgroup.get_pv(pv_name_), value_ptr_->choices, value_ptr_->index);
        break;
    case pvtui::ChoiceStyle::Dropdown:
        component_ = make_dropdown_widget(pvgroup.get_pv(pv_name_), value_ptr_->choices, value_ptr_->index);
        break;
    }
}

const PVEnum& ChoiceWidget::value() const { return *value_ptr_; }

ButtonWidget::ButtonWidget(PVGroup& pvgroup, const ArgParser& args, const std::string& pv_name,
                           const std::string& label, int press_val)
    : WidgetBase(pvgroup, args, pv_name) {
    component_ = make_button_widget(pvgroup.get_pv(pv_name_), label, press_val);
}

ButtonWidget::ButtonWidget(App& app, const std::string& pv_name, const std::string& label, int press_val)
    : WidgetBase(app.pvgroup, app.args, pv_name) {
    component_ = make_button_widget(app.pvgroup.get_pv(pv_name_), label, press_val);
}

ButtonWidget::ButtonWidget(PVGroup& pvgroup, const std::string& pv_name, const std::string& label,
                           int press_val)
    : WidgetBase(pvgroup, pv_name) {
    component_ = make_button_widget(pvgroup.get_pv(pv_name_), label, press_val);
}

} // namespace pvtui
