#include <pvtui/widgets.hpp>

namespace pvtui {

namespace unicode {

std::string rectangle(int len) {
    std::string out;
    out.reserve(len * full_block.length());
    for (int i = 0; i < len; i++) {
        out.append(full_block);
    }
    return out;
}

} // namespace unicode

namespace {

ftxui::Component make_button_widget(pace::ChannelBase& pv, const std::string& label, int value,
                                    ftxui::ButtonOption op) {
    op.label = label;
    op.on_click = [&pv, value]() {
        if (pv.connected()) {
            pv.put(value);
        }
    };
    return ftxui::Button(op);
}

template <typename T>
bool put_string_as(std::string_view str, pace::ChannelBase& pv) {
    T val{};
    try {
        if constexpr (std::is_same_v<T, double>) {
            val = std::stod(str.data());
        } else if constexpr (std::is_same_v<T, int>) {
            val = std::stoi(str.data());
        } else if constexpr (std::is_same_v<T, std::string>) {
            val = str;
        }
        pv.put(val);
    } catch (...) {
        return false;
    }
    return true;
}

ftxui::Component make_input_widget(pace::ChannelBase& pv, std::shared_ptr<std::string> disp_str, PVPutType put_type, ftxui::Color fg,
                                   ftxui::Color hover) {

    ftxui::InputOption input_op;

    input_op.content = disp_str.get();
    input_op.multiline = false;

    input_op.transform = [&pv, disp_str, fg, hover](ftxui::InputState s) {
        if (not pv.connected()) {
            *disp_str = "";
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

    input_op.on_enter = [&pv, disp_str, put_type]() {
        if (pv.connected()) {
            if (put_type == PVPutType::Double) {
                put_string_as<double>(*disp_str, pv);
            } else if (put_type == PVPutType::Integer) {
                put_string_as<int>(*disp_str, pv);
            } else if (put_type == PVPutType::String) {
                put_string_as<std::string>(*disp_str, pv);
            }
        }
    };

    return ftxui::Input(input_op);
}

ftxui::Component make_choice_h_widget(pace::ChannelBase& pv, std::shared_ptr<pace::Enum> value) {
    ftxui::MenuOption op = ftxui::MenuOption::Toggle();
    op.entries = &value->choices;
    op.selected = &value->index;
    op.on_change = [&pv, value]() {
        if (pv.connected()) {
            pv.put(value->index, "value.index");
        }
    };
    return ftxui::Menu(op);
}

ftxui::Component make_choice_v_widget(pace::ChannelBase& pv, std::shared_ptr<pace::Enum> value) {
    ftxui::MenuOption op = ftxui::MenuOption::Vertical();
    op.entries = &value->choices;
    op.selected = &value->index;
    op.on_change = [&pv, value]() {
        if (pv.connected()) {
            pv.put(value->index, "value.index");
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

ftxui::Component make_dropdown_widget(pace::ChannelBase& pv, std::shared_ptr<pace::Enum> value) {
    using namespace ftxui;

    DropdownOption dropdown_op;

    dropdown_op.radiobox.entries = &value->choices;
    dropdown_op.radiobox.selected = &value->index;
    dropdown_op.radiobox.on_change = [&pv, value]() {
        if (pv.connected()) {
            pv.put(value->index, "value.index");
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

ftxui::Component make_bits_widget(std::shared_ptr<int> value, BitRange range, ftxui::Color color_on,
                                  ftxui::Color color_off) {
    using namespace ftxui;
    return Renderer([value, range, color_on, color_off] {
        Elements rows;
        for (size_t i = range.start; i < range.end; i++) {
            int v = *value & (1u << i);
            auto clr = v ? color(color_on) : color(color_off);
            rows.push_back(text(unicode::rectangle(2)) | clr);
        }
        return vbox({rows});
    });
}

ftxui::Component make_slider_widget(pace::ChannelBase& pv, std::shared_ptr<double> value,
                                    const SliderRange& range,
                                    ftxui::Color color_active, ftxui::Color color_inactive) {
    using namespace ftxui;
    auto op = SliderOption<double>();
    op.value = value.get();
    op.min = range.min;
    op.max = range.max;
    op.increment = range.increment;
    op.color_active = color_active;
    op.color_inactive = color_inactive;
    op.on_change = [&pv, value] {
        if (pv.connected()) {
            try {
                pv.put(*value);
            } catch (...) {
            }
        }
    };
    return Slider(op);
}

} // namespace

WidgetBase::WidgetBase(pace::Context& ctxt, const std::string& pv_name) : ctxt_(ctxt), pv_name_(pv_name) {
    ctxt.connect(pv_name_);
}

std::string WidgetBase::pv_name() const { return pv_name_; }

bool WidgetBase::connected() const { return ctxt_.get()[pv_name_].connected(); }

std::string WidgetBase::value_as_string() const { return value_as_string_ ? value_as_string_() : ""; }

ftxui::Component WidgetBase::component() const {
    if (component_) {
        return component_;
    } else {
        throw std::runtime_error("No component defined for " + pv_name_);
    }
}

InputWidget::InputWidget(App& app, const std::string& pv_name, PVPutType put_type, ftxui::Color fg,
                         ftxui::Color hover) : InputWidget(app.context, pv_name, put_type, fg, hover) {}

InputWidget::InputWidget(pace::Context& ctxt, const std::string& pv_name, PVPutType put_type, ftxui::Color fg,
                         ftxui::Color hover)
    : WidgetBase(ctxt, pv_name), value_ptr_(std::make_shared<std::string>()) {
    ctxt.bind(*value_ptr_, pv_name_);
    component_ = make_input_widget(ctxt.get_channel(pv_name_), value_ptr_, put_type, fg, hover);
    value_as_string_ = [v = value_ptr_] { return *v; };
}

const std::string& InputWidget::value() const { return *value_ptr_; }

BitsWidget::BitsWidget(pace::Context& ctxt, const std::string& pv_name, BitRange range, ftxui::Color color_on,
                       ftxui::Color color_off)
    : WidgetBase(ctxt, pv_name), value_ptr_(std::make_shared<int>()) {
    ctxt.bind(*value_ptr_, pv_name_);
    component_ = make_bits_widget(value_ptr_, range, color_on, color_off);
    value_as_string_ = [v = value_ptr_] { return std::to_string(*v); };
}

BitsWidget::BitsWidget(App& app, const std::string& pv_name, BitRange range, ftxui::Color color_on,
                       ftxui::Color color_off)
    : BitsWidget(app.context, pv_name, range, color_on, color_off) {}

const int& BitsWidget::value() const { return *value_ptr_; }

ChoiceWidget::ChoiceWidget(App& app, const std::string& pv_name, ChoiceStyle style)
    : ChoiceWidget(app.context, pv_name, style) {}

ChoiceWidget::ChoiceWidget(pace::Context& ctxt, const std::string& pv_name, ChoiceStyle style)
    : WidgetBase(ctxt, pv_name), value_ptr_(std::make_shared<pace::Enum>()) {
    ctxt.bind(*value_ptr_, pv_name_);
    switch (style) {
    case pvtui::ChoiceStyle::Vertical:
        component_ = make_choice_v_widget(ctxt.get_channel(pv_name_), value_ptr_);
        break;
    case pvtui::ChoiceStyle::Horizontal:
        component_ = make_choice_h_widget(ctxt.get_channel(pv_name_), value_ptr_);
        break;
    case pvtui::ChoiceStyle::Dropdown:
        component_ = make_dropdown_widget(ctxt.get_channel(pv_name_), value_ptr_);
        break;
    }
    value_as_string_ = [v = value_ptr_] { return v->choice; };
}

const pace::Enum& ChoiceWidget::value() const { return *value_ptr_; }

ButtonWidget::ButtonWidget(App& app, const std::string& pv_name, const std::string& label,
                           ftxui::ButtonOption op, int press_val)
    : WidgetBase(app.context, pv_name) {
    component_ = make_button_widget(app.context.get_channel(pv_name_), label, press_val, op);
}

ButtonWidget::ButtonWidget(pace::Context& ctxt, const std::string& pv_name, const std::string& label,
                           ftxui::ButtonOption op, int press_val)
    : WidgetBase(ctxt, pv_name) {
    component_ = make_button_widget(ctxt.get_channel(pv_name_), label, press_val, op);
}

SliderWidget::SliderWidget(App& app, const std::string& pv_name, SliderRange range,
                           ftxui::Color color_active, ftxui::Color color_inactive)
    : SliderWidget(app.context, pv_name, range, color_active, color_inactive) {}

SliderWidget::SliderWidget(pace::Context& ctxt, const std::string& pv_name, SliderRange range,
                           ftxui::Color color_active, ftxui::Color color_inactive)
    : WidgetBase(ctxt, pv_name), value_ptr_(std::make_shared<double>(0.0)) {
    ctxt.bind(*value_ptr_, pv_name_);
    component_ =
        make_slider_widget(ctxt.get_channel(pv_name_), value_ptr_, range, color_active, color_inactive);
    value_as_string_ = [v = value_ptr_] { return std::to_string(*v); };
}

const double& SliderWidget::value() const { return *value_ptr_; }

} // namespace pvtui
