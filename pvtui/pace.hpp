#pragma once
#include <atomic>
#include <cstring>
#include <iomanip>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <variant>
#include <vector>

#include "cadef.h"
#include <pvxs/client.h>

namespace pace {

struct Enum {
    std::vector<std::string> choices;
    std::string choice;
    int index;
};

namespace detail {

/// \brief std::variant type that holds the latest value from an EPICS subscription.
///
/// PVs that cannot be represented as one of the types in this variant are
/// unsupported.
using ValueVariant =
    std::variant<std::monostate, double, int, std::string, std::vector<double>, std::vector<int>, Enum>;

/// \brief Convert a ValueVariant to a target type T.
///
/// The ValueVariant holds the latest value from a subscription callback
/// This function attempts to convert it to the type T of the user's bound
/// variable. Returns std::nullopt if the conversion is not supported
template <typename T>
std::optional<T> convert(const ValueVariant& v, int precision = 4) {

    // Target type T is the same as the variant's type
    if (auto* val = std::get_if<T>(&v)) {
        return *val;
    }

    // Enum -> arithmetic: extract index
    if constexpr (std::is_arithmetic_v<T>) {
        if (auto* e = std::get_if<Enum>(&v)) {
            return static_cast<T>(e->index);
        }
    }

    // Enum -> string: extract current choice string
    if constexpr (std::is_same_v<T, std::string>) {
        if (auto* e = std::get_if<Enum>(&v)) {
            return e->choice;
        }
    }

    // Target type T and variant are different, but
    // both arithmetic types, so we cast.
    if constexpr (std::is_arithmetic_v<T>) {
        if (auto* d = std::get_if<double>(&v)) {
            return static_cast<T>(*d);
        }
        if (auto* i = std::get_if<int>(&v)) {
            return static_cast<T>(*i);
        }
    }

    // Target type T is string, and variant type is not
    // so we convert to string if we can.
    if constexpr (std::is_same_v<T, std::string>) {
        if (auto* d = std::get_if<double>(&v)) {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(precision) << *d;
            return oss.str();
        }
        if (auto* i = std::get_if<int>(&v)) {
            return std::to_string(*i);
        }
    }

    // TODO: Handle conversion between std::vector<double> and std::vector<int>

    return std::nullopt;
}

/// \brief Type-erased base class for monitor fan-out slots.
///
/// ChannelBase stores a vector of MonitorSlotBase pointers to fan out a single
/// staged ValueVariant to bound variables of potentially different types.
/// Each concrete MonitorSlot<T> handles conversion and distribution for one
/// target type. This provides type erasure so that ChannelBase doesn't need to
/// know the types of the user's bound variables.
///
/// Slots are created on demand by bind<T>():
/// - If a MonitorSlot<T> already exists, the new variable pointer is appended
///   to its targets vector.
/// - Otherwise, a new MonitorSlot<T> is created.
///
/// When sync() runs, it iterates all slots and calls copy_to_targets() on each.
/// Each slot independently converts the variant to its type T via convert<T>()
/// and writes the result to all of its target pointers.
struct MonitorSlotBase {
    virtual ~MonitorSlotBase() = default;
    virtual void copy_to_targets(const ValueVariant& staged, int precision,
                                 pvxs::Value* pvxs_value = nullptr) = 0;
};

/// \brief Concrete slot that fans out a ValueVariant to bound variables of type T.
///
/// Holds raw pointers to the user's bound variables. When copy_to_targets() is
/// called, it converts the staged value via convert<T>() and writes to every
/// target. If conversion fails (e.g. string variant to double target), the
/// targets are left unchanged.
template <typename T>
struct MonitorSlot : MonitorSlotBase {

    struct Target {
        T* pvalue;
        std::string field_path;
    };

    std::vector<Target> targets;

    void copy_to_targets(const ValueVariant& staged, int precision,
                         pvxs::Value* pvxs_value = nullptr) override {
        for (auto& target : targets) {
            if (pvxs_value && std::holds_alternative<std::monostate>(staged)) {
                pvxs::Value field = (*pvxs_value)[target.field_path];
                try {
                    if constexpr (std::is_same_v<T, std::vector<double>>) {
                        auto arr = field.as<pvxs::shared_array<const double>>();
                        *target.pvalue = std::vector<double>(arr.begin(), arr.end());
                    } else if constexpr (std::is_same_v<T, std::vector<int>>) {
                        auto arr = field.as<pvxs::shared_array<const int32_t>>();
                        *target.pvalue = std::vector<int>(arr.begin(), arr.end());
                    } else if constexpr (std::is_same_v<T, Enum>) {
                        // Handled via staged_value_, not here with pvxs field extraction
                    } else {
                        *target.pvalue = field.as<T>();
                    }
                } catch (const std::exception& e) {
                }
            } else {
                if (auto val = convert<T>(staged, precision)) {
                    *target.pvalue = *val;
                }
            }
        }
    }
};

} // namespace detail

/// \brief Abstract base class for a channel bound to a single PV.
///
/// Provides the bind/sync mechanism that decouples the network callback thread
/// from the user's polling thread. Subclasses (e.g. CAChannel) are responsible
/// for connecting to the PV and writing into staged_value_ when new data
/// arrives. The user calls sync() to copy the staged value to all bound
/// variables.
class ChannelBase {
  public:
    ChannelBase(const std::string& pv_name) : pv_name_(pv_name) {}

    // Delete copy constructor and move assignment operator
    ChannelBase(const ChannelBase&) = delete;
    ChannelBase& operator=(const ChannelBase&) = delete;

    virtual ~ChannelBase() = default;

    /// \brief Returns true if the channel is currently connected to the PV.
    virtual bool connected() const = 0;

    /// \brief Writes a value to the PV.
    ///
    /// This template wraps the value in a ValueVariant and dispatches to the
    /// protocol-specific put_var() override (CAChannel or PVAChannel). The
    /// virtual override is protected so users always call this template, and
    /// subclasses only override the ValueVariant version.
    ///
    /// \param value The value to write.
    /// \return true if the put was sent, false if the channel is disconnected
    /// or the value is not convertible to a supported type.
    template <typename T>
    bool put(const T& value, const std::string& field_path = "value") {
        return put_var(detail::ValueVariant(value), field_path);
    }

    /// \brief Write a string literal to the PV
    /// \overload
    bool put(const char* value, const std::string& field_path = "value") {
        return put_var(detail::ValueVariant(std::string(value)), field_path);
    }

    /// \brief Synchronous get. Fetches the current value from the IOC.
    ///
    /// Dispatches to the protocol-specific get_var() override. Unlike peek(),
    /// this performs a network round-trip to get a fresh value.
    ///
    /// \param timeout Timeout in seconds for the get operation.
    /// \return The value as std::optional<T>, or std::nullopt on failure/timeout.
    template <typename T>
    std::optional<T> get(double timeout = 1.0) {
        detail::ValueVariant value = get_var("value", timeout);
        return detail::convert<T>(value);
    }

    template <typename T>
    std::optional<T> get(const std::string& field_path, double timeout = 1.0) {
        detail::ValueVariant value = get_var(field_path, timeout);
        return detail::convert<T>(value);
    }

    /// \brief Bind a user variable to receive monitor updates from this channel.
    ///
    /// The variable will be updated with the latest PV value each time sync() is
    /// called. Multiple variables can be bound to the same channel, including
    /// variables of different types. The bound variable must outlive the channel.
    ///
    /// \param var Reference to the user's variable. A raw pointer to this
    ///            variable is stored internally.
    template <typename T>
    void bind(T& var, const std::string& field_path = "value") {
        std::lock_guard lock(mutex_);
        // If a MonitorSlot for this T already exists,
        // store this pointer in its target vector
        for (auto& slot : slots_) {
            if (auto* typed = dynamic_cast<detail::MonitorSlot<T>*>(slot.get())) {
                typed->targets.push_back({&var, field_path});
                return;
            }
        }
        // Create a new slot if no slot for T exists already
        auto slot = std::make_unique<detail::MonitorSlot<T>>();
        slot->targets.push_back({&var, field_path});
        slots_.push_back(std::move(slot));

        // Inform the child class we need to create a subscription
        enable_monitor();
    }

    /// \brief Start monitoring the PV without binding a variable.
    ///
    /// The PV must have been added to the context and connect first. After calling
    /// monitor(), use peek<T>() to read the latest value on demand.
    void monitor() { enable_monitor(); }

    /// \brief Copy the latest staged value to all bound variables.
    ///
    /// Returns true if new data was available since the last call to sync().
    /// This is the user's polling point. Call sync() periodically from your
    /// application loop.
    bool sync() {
        if (!new_data_.load(std::memory_order_acquire)) {
            return false;
        }
        std::lock_guard lock(mutex_);
        for (auto& slot : slots_) {
            if (pvxs_value_) {
                slot->copy_to_targets(staged_value_, metadata_.precision, &pvxs_value_.value());
            } else {
                slot->copy_to_targets(staged_value_, metadata_.precision, nullptr);
            }
        }
        new_data_.store(false, std::memory_order_relaxed);
        return true;
    }

    /// \brief Returns the latest value from the monitor as the request type.
    ///
    /// Returns The latest value from the monitor as std::optional<T>.
    /// If the conversion to T fails, the returned value is std::nullopt;
    template <typename T>
    std::optional<T> peek() {
        std::lock_guard lock(mutex_);
        return detail::convert<T>(staged_value_);
    }

  private:
    std::vector<std::unique_ptr<detail::MonitorSlotBase>> slots_;

  protected:
    /// \brief CA/PVA specific put implementation.
    virtual bool put_var(const detail::ValueVariant& value, const std::string& field_path) = 0;

    /// \brief CA/PVA specific get implementation.
    virtual detail::ValueVariant get_var(const std::string& field_path, double timeout) = 0;

    /// \brief Informs the CA/PVAChannel to create a monitor.
    virtual void enable_monitor() = 0;

    std::string pv_name_;
    std::mutex mutex_;
    std::atomic<bool> new_data_{false};
    std::optional<pvxs::Value> pvxs_value_;
    detail::ValueVariant staged_value_;
    struct PVMetaData {
        int precision = 4;
        std::vector<std::string> enum_choices;
    } metadata_;
};

/// \brief Channel Access implementation of ChannelBase.
///
/// Connects to a single PV via the EPICS Channel Access protocol. A CA context
/// (e.g. pace::Context) must exist before construction. A monitor subscription
/// is created when bind() or monitor() is called. For floating-point PVs, the
/// record's PREC field is fetched at monitor start time.
///
/// For write operations, use put(), or id() for direct CA API access.
class CAChannel : public ChannelBase {
  public:
    CAChannel(const std::string& pv_name) : ChannelBase(pv_name) {
        if (!ca_current_context()) {
            throw std::runtime_error("No CA context. Call ca_context_create() before creating a CAChannel.");
        }
        SEVCHK(
            ca_create_channel(pv_name.c_str(), connection_callback, this, CA_PRIORITY_DEFAULT, &channel_id_),
            "ca_create_channel");
        SEVCHK(ca_flush_io(), "ca_flush_io");
    }

    ~CAChannel() {
        if (evt_id_) {
            ca_clear_subscription(evt_id_);
        }
        ca_clear_channel(channel_id_);
    }

    bool connected() const override { return connected_.load(std::memory_order_relaxed); }

    /// \brief Returns the underlying CA channel identifier for direct CA API use.
    chid id() const { return channel_id_; }

    void enable_monitor() override {
        if (wants_monitor_.load(std::memory_order_relaxed)) {
            // we have already created the monitor
            return;
        }
        wants_monitor_.store(true, std::memory_order_relaxed);
        start_monitor();
    }

  private:
    std::atomic<bool> connected_{false};
    std::atomic<bool> wants_monitor_{false};
    chid channel_id_;
    evid evt_id_ = nullptr;

    /// \brief CA-specific put implementation. Sends the value via ca_put + ca_flush_io.
    bool put_var(const detail::ValueVariant& value, const std::string& field_path = "value") override {
        if (!connected()) {
            return false;
        }
        if (auto* v = std::get_if<double>(&value)) {
            dbr_double_t val = *v;
            ca_put(DBR_DOUBLE, channel_id_, &val);
        } else if (auto* v = std::get_if<int>(&value)) {
            dbr_long_t val = *v;
            ca_put(DBR_LONG, channel_id_, &val);
        } else if (auto* s = std::get_if<std::string>(&value)) {
            dbr_string_t v;
            strncpy(v, s->c_str(), sizeof(v) - 1);
            v[sizeof(v) - 1] = '\0';
            ca_put(DBR_STRING, channel_id_, &v);
        } else if (auto* e = std::get_if<Enum>(&value)) {
            dbr_enum_t val = static_cast<dbr_enum_t>(e->index);
            ca_put(DBR_ENUM, channel_id_, &val);
        } else {
            return false;
        }
        ca_flush_io();
        return true;
    }

    /// \brief Creates a monitor subscription and fetches precision on connect.
    ///
    /// Called from the connection callback when the channel comes up. Clears
    /// any existing subscription before creating a new one. For DBF_FLOAT and
    /// DBF_DOUBLE PVs, also issues a one-time ca_get_callback to fetch PREC.
    void start_monitor() {
        if (wants_monitor_.load(std::memory_order_relaxed) && connected()) {
            if (evt_id_) {
                ca_clear_subscription(evt_id_);
                evt_id_ = nullptr;
            }

            auto native = ca_field_type(channel_id_);
            SEVCHK(ca_create_subscription(dbf_type_to_DBR(native), 0, channel_id_, DBE_VALUE | DBE_ALARM,
                                          subscription_callback, this, &evt_id_),
                   "ca_create_subscription");

            if (native == DBF_FLOAT || native == DBF_DOUBLE) {
                ca_get_callback(DBR_CTRL_DOUBLE, channel_id_, precision_callback, this);
            } else if (native == DBF_ENUM) {
                ca_get_callback(DBR_CTRL_ENUM, channel_id_, enum_metadata_callback, this);
            }

            SEVCHK(ca_flush_io(), "ca_flush_io");
        }
    }

    /// \brief CA connection state callback. Starts the monitor on connect.
    static void connection_callback(struct connection_handler_args args) {
        auto* self = static_cast<CAChannel*>(ca_puser(args.chid));
        if (args.op == CA_OP_CONN_UP) {
            self->connected_.store(true, std::memory_order_relaxed);
            self->start_monitor();
        } else {
            self->connected_.store(false, std::memory_order_relaxed);
        }
    }

    /// \brief One-time ca_get_callback handler that extracts PREC from DBR_CTRL_DOUBLE.
    /// Falls back to a default precision of 4 on failure.
    static void precision_callback(struct event_handler_args evt) {
        auto* self = static_cast<CAChannel*>(evt.usr);
        std::lock_guard lock(self->mutex_);
        if (evt.status == ECA_NORMAL) {
            auto* ctrl = static_cast<const struct dbr_ctrl_double*>(evt.dbr);
            self->metadata_.precision = ctrl->precision;
        } else {
            self->metadata_.precision = 4;
        }
    }

    static void enum_metadata_callback(struct event_handler_args evt) {
        auto* self = static_cast<CAChannel*>(evt.usr);
        std::lock_guard lock(self->mutex_);
        if (evt.status == ECA_NORMAL) {
            auto* ctrl = static_cast<const struct dbr_ctrl_enum*>(evt.dbr);
            self->metadata_.enum_choices.clear();
            for (int i = 0; i < ctrl->no_str; i++) {
                self->metadata_.enum_choices.emplace_back(ctrl->strs[i]);
            }
        }
    }

    /// \brief CA-specific get implementation. Fetches the value via ca_get + ca_pend_io.
    detail::ValueVariant get_var(const std::string& field_path, double timeout) override {
        if (!connected()) {
            return {};
        }

        detail::ValueVariant value;
        auto native = ca_field_type(channel_id_);
        auto dbr = dbf_type_to_DBR(native);

        switch (dbr) {
        case DBR_DOUBLE: {
            dbr_double_t data;
            ca_get(DBR_DOUBLE, channel_id_, &data);
            if (ca_pend_io(timeout) != ECA_NORMAL) {
                return {};
            }
            value = static_cast<double>(data);
            break;
        }
        case DBR_FLOAT: {
            dbr_float_t data;
            ca_get(DBR_FLOAT, channel_id_, &data);
            if (ca_pend_io(timeout) != ECA_NORMAL) {
                return {};
            }
            value = static_cast<double>(data);
            break;
        }
        case DBR_LONG: {
            dbr_long_t data;
            ca_get(DBR_LONG, channel_id_, &data);
            if (ca_pend_io(timeout) != ECA_NORMAL) {
                return {};
            }
            value = static_cast<int>(data);
            break;
        }
        case DBR_SHORT: {
            dbr_short_t data;
            ca_get(DBR_SHORT, channel_id_, &data);
            if (ca_pend_io(timeout) != ECA_NORMAL) {
                return {};
            }
            value = static_cast<int>(data);
            break;
        }
        case DBR_CHAR: {
            dbr_char_t data;
            ca_get(DBR_CHAR, channel_id_, &data);
            if (ca_pend_io(timeout) != ECA_NORMAL) {
                return {};
            }
            value = static_cast<int>(data);
            break;
        }
        case DBR_ENUM: {
            struct dbr_ctrl_enum data;
            ca_get(DBR_CTRL_ENUM, channel_id_, &data);
            if (ca_pend_io(timeout) != ECA_NORMAL) {
                return {};
            }
            if (metadata_.enum_choices.empty()) {
                for (int i = 0; i < data.no_str; i++) {
                    metadata_.enum_choices.emplace_back(data.strs[i]);
                }
            }
            Enum e;
            e.choices = metadata_.enum_choices;
            e.index = static_cast<int>(data.value);
            e.choice = (data.value < metadata_.enum_choices.size()) ? metadata_.enum_choices[data.value]
                                                                    : std::string();
            value = std::move(e);
            break;
        }
        case DBR_STRING: {
            dbr_string_t data;
            ca_get(DBR_STRING, channel_id_, &data);
            if (ca_pend_io(timeout) != ECA_NORMAL) {
                return {};
            }
            value = static_cast<std::string>(data);
            break;
        }
        }
        return value;
    }

    /// \brief Extracts a scalar value from a CA subscription event.
    static detail::ValueVariant get_scalar_event(struct event_handler_args evt) {
        detail::ValueVariant value;
        switch (evt.type) {
        case DBR_DOUBLE:
            value = *static_cast<const dbr_double_t*>(evt.dbr);
            break;
        case DBR_FLOAT:
            value = static_cast<double>(*static_cast<const dbr_float_t*>(evt.dbr));
            break;
        case DBR_LONG:
            value = static_cast<int>(*static_cast<const dbr_long_t*>(evt.dbr));
            break;
        case DBR_SHORT:
            value = static_cast<int>(*static_cast<const dbr_short_t*>(evt.dbr));
            break;
        case DBR_CHAR:
            value = static_cast<int>(*static_cast<const dbr_char_t*>(evt.dbr));
            break;
        case DBR_ENUM:
            value = static_cast<int>(*static_cast<const dbr_enum_t*>(evt.dbr));
            break;
        case DBR_STRING:
            value = std::string(static_cast<const char*>(evt.dbr));
            break;
        }
        return value;
    }

    /// \brief Extracts an array value from a CA subscription event.
    static detail::ValueVariant get_array_event(struct event_handler_args evt) {
        detail::ValueVariant value;
        auto count = static_cast<size_t>(evt.count);
        switch (evt.type) {
        case DBR_CHAR: {
            auto* arr = static_cast<const dbr_char_t*>(evt.dbr);
            std::vector<int> vec(arr, arr + count);
            value = std::move(vec);
            break;
        }
        case DBR_DOUBLE: {
            auto* arr = static_cast<const dbr_double_t*>(evt.dbr);
            std::vector<double> vec(arr, arr + count);
            value = std::move(vec);
            break;
        }
        case DBR_FLOAT: {
            auto* arr = static_cast<const dbr_float_t*>(evt.dbr);
            std::vector<double> vec(arr, arr + count);
            value = std::move(vec);
            break;
        }
        case DBR_LONG: {
            auto* arr = static_cast<const dbr_long_t*>(evt.dbr);
            std::vector<int> vec(arr, arr + count);
            value = std::move(vec);
            break;
        }
        case DBR_SHORT: {
            auto* arr = static_cast<const dbr_short_t*>(evt.dbr);
            std::vector<int> vec(arr, arr + count);
            value = std::move(vec);
            break;
        }
        case DBR_ENUM: {
            break;
        }
        case DBR_STRING: {
            break;
        }
        }
        return value;
    }

    /// \brief CA subscription callback. Converts the incoming value to a
    /// ValueVariant and stages it for the next sync() call.
    static void subscription_callback(struct event_handler_args evt) {
        auto* self = static_cast<CAChannel*>(evt.usr);
        if (evt.status != ECA_NORMAL) {
            return;
        }

        detail::ValueVariant value;
        if (evt.count == 1) {
            value = get_scalar_event(evt);
        } else if (evt.count > 1) {
            value = get_array_event(evt);
        }

        if (std::holds_alternative<std::monostate>(value)) {
            return;
        }

        std::lock_guard lock(self->mutex_);
        if (evt.type == DBR_ENUM && !self->metadata_.enum_choices.empty()) {
            if (auto* idx = std::get_if<int>(&value)) {
                Enum e;
                e.choices = self->metadata_.enum_choices;
                e.index = *idx;
                e.choice = (*idx >= 0 && *idx < static_cast<int>(e.choices.size())) ? e.choices[*idx]
                                                                                    : std::string();
                value = std::move(e);
            }
        }
        self->staged_value_ = value;
        self->new_data_.store(true, std::memory_order_release);
    }
};

/// \brief PV Access implementation of ChannelBase using PVXS.
///
/// Connects to a single PV via the EPICS PV Access protocol. A
/// pvxs::client::Context is passed at construction. A monitor subscription is
/// created when bind() or monitor() is called. Supports NTScalar PVs (via
/// staged_value_) and custom structured PVs (via pvxs_value_ with field paths).
class PVAChannel : public ChannelBase {
  public:
    PVAChannel(pvxs::client::Context& context, const std::string& pv_name)
        : ChannelBase(pv_name), ctx_(context) {
        connection_ = ctx_.connect(pv_name)
                          .onConnect([this] { this->connected_.store(true, std::memory_order_relaxed); })
                          .onDisconnect([this] { this->connected_.store(false, std::memory_order_relaxed); })
                          .exec();
    }

    ~PVAChannel() {
        if (subscription_) {
            subscription_->cancel();
        }
    }

    bool connected() const override { return connected_.load(std::memory_order_relaxed); }

    void enable_monitor() override {
        if (wants_monitor_.load(std::memory_order_relaxed)) {
            return;
        }
        wants_monitor_.store(true, std::memory_order_relaxed);
        start_monitor();
    }

  private:
    std::atomic<bool> connected_{false};
    std::atomic<bool> wants_monitor_{false};
    std::shared_ptr<pvxs::client::Subscription> subscription_;
    std::shared_ptr<pvxs::client::Connect> connection_;
    std::shared_ptr<pvxs::client::Operation> put_operation_;
    pvxs::client::Context& ctx_;

    void start_monitor() {
        if (!wants_monitor_.load(std::memory_order_relaxed)) {
            return;
        }
        if (subscription_) {
            return;
        }

        subscription_ =
            ctx_.monitor(pv_name_)
                .event([this](pvxs::client::Subscription& sub) {
                    try {
                        while (auto update = sub.pop()) {
                            std::lock_guard lock(mutex_);
                            pvxs_value_ = update;
                            detail::ValueVariant value;
                            auto pva_value_field = update["value"];
                            if (pva_value_field) {
                                switch (pva_value_field.type().code) {
                                case pvxs::TypeCode::Struct: {
                                    auto idx_field = pva_value_field["index"];
                                    auto choices_field = pva_value_field["choices"];
                                    if (idx_field && choices_field) {
                                        Enum e;
                                        e.index = idx_field.as<int32_t>();
                                        auto arr = choices_field.as<pvxs::shared_array<const std::string>>();
                                        for (auto& s : arr) {
                                            e.choices.emplace_back(s);
                                        }
                                        e.choice =
                                            (e.index >= 0 && e.index < static_cast<int>(e.choices.size()))
                                                ? e.choices[e.index]
                                                : std::string();
                                        metadata_.enum_choices = e.choices;
                                        value = std::move(e);
                                    } else {
                                        continue;
                                    }
                                    break;
                                }
                                case pvxs::TypeCode::Float64:
                                    value = pva_value_field.as<double>();
                                    break;
                                case pvxs::TypeCode::Float32:
                                    value = static_cast<double>(pva_value_field.as<float>());
                                    break;
                                case pvxs::TypeCode::Int32:
                                    value = static_cast<int>(pva_value_field.as<int32_t>());
                                    break;
                                case pvxs::TypeCode::Int16:
                                    value = static_cast<int>(pva_value_field.as<int16_t>());
                                    break;
                                case pvxs::TypeCode::UInt16:
                                    value = static_cast<int>(pva_value_field.as<uint16_t>());
                                    break;
                                case pvxs::TypeCode::Int8:
                                    value = static_cast<int>(pva_value_field.as<int8_t>());
                                    break;
                                case pvxs::TypeCode::UInt8:
                                    value = static_cast<int>(pva_value_field.as<uint8_t>());
                                    break;
                                case pvxs::TypeCode::String:
                                    value = pva_value_field.as<std::string>();
                                    break;
                                default:
                                    continue;
                                }
                                staged_value_ = value;
                            }
                            new_data_.store(true, std::memory_order_release);
                        }
                    } catch (pvxs::client::Connected&) {
                    } catch (pvxs::client::Finished&) {
                    } catch (pvxs::client::Disconnect&) {
                    }
                })
                .exec();
    }

    /// \brief PVA-specific put implementation.
    /// TODO: document more
    bool put_var(const detail::ValueVariant& value, const std::string& field_path = "value") override {
        if (!connected()) {
            return false;
        }
        pvxs::client::PutBuilder put_builder;
        if (auto* v = std::get_if<double>(&value)) {
            put_builder = ctx_.put(pv_name_).set(field_path, static_cast<double>(*v));
        } else if (auto* v = std::get_if<int>(&value)) {
            put_builder = ctx_.put(pv_name_).set(field_path, static_cast<int>(*v));
        } else if (auto* v = std::get_if<Enum>(&value)) {
            put_builder = ctx_.put(pv_name_).set("value.index", static_cast<int>(v->index));
        } else if (auto* v = std::get_if<std::string>(&value)) {
            put_builder = ctx_.put(pv_name_).set(field_path, static_cast<std::string>(*v));
        }
        else {
            // unsupported type
            return false;
        }
        put_operation_ = put_builder.exec();
        return true;
    }

    /// \brief PVA-specific get implementation. Fetches the value via pvxs::client::Context::get.
    detail::ValueVariant get_var(const std::string& field_path, double timeout) override {
        if (!connected()) {
            return {};
        }

        try {
            auto result = ctx_.get(this->pv_name_).exec()->wait(timeout);
            if (auto val = result[field_path]) {
                detail::ValueVariant var;
                switch (val.type().code) {
                case pvxs::TypeCode::Struct: {
                    auto idx_field = val["index"];
                    auto choices_field = val["choices"];
                    if (idx_field && choices_field) {
                        Enum e;
                        e.index = idx_field.as<int>();
                        auto arr = choices_field.as<pvxs::shared_array<const std::string>>();
                        for (auto& s : arr) {
                            e.choices.emplace_back(s);
                        }
                        e.choice = (e.index >= 0 && e.index < static_cast<int>(e.choices.size()))
                                       ? e.choices[e.index]
                                       : std::string();
                        var = std::move(e);
                    }
                    break;
                }
                case pvxs::TypeCode::Float64:
                    var = val.as<double>();
                    break;
                case pvxs::TypeCode::Float32:
                    var = static_cast<double>(val.as<float>());
                    break;
                case pvxs::TypeCode::Int32:
                    var = static_cast<int>(val.as<int32_t>());
                    break;
                case pvxs::TypeCode::Int16:
                    var = static_cast<int>(val.as<int16_t>());
                    break;
                case pvxs::TypeCode::UInt16:
                    var = static_cast<int>(val.as<uint16_t>());
                    break;
                case pvxs::TypeCode::Int8:
                    var = static_cast<int>(val.as<int8_t>());
                    break;
                case pvxs::TypeCode::UInt8:
                    var = static_cast<int>(val.as<uint8_t>());
                    break;
                case pvxs::TypeCode::String:
                    var = val.as<std::string>();
                    break;
                default:
                    var = {};
                }
                return var;
            } else {
                return {};
            }
        } catch (...) {
            return {};
        }
    }
};

/// \brief Manages a collection of PV channels with a single sync() call.
///
/// Context is the primary interface for monitoring multiple PVs. It supports
/// both CA and PVA protocols (including mixed usage via protocol prefixes).
/// Add PVs with add(), bind local variables with bind(), then call sync()
/// periodically to update all bound variables at once.
///
/// Example usage:
/// \code
///     pace::Context ctxt;
///     ctxt.add("MyIOC:", {"m1.RBV", "m1.DMOV"});
///
///     double position = 0.0;
///     int done = 0;
///     ctxt.bind(position, "MyIOC:m1.RBV");
///     ctxt.bind(done, "MyIOC:m1.DMOV");
///
///     while (true) {
///         if (ctxt.sync()) {
///             // At least one PV has new data
///             printf("pos=%f done=%d\n", position, done);
///         }
///         std::this_thread::sleep_for(std::chrono::milliseconds(100));
///     }
/// \endcode
class Context {
  public:
    Context(const std::string& default_protocol = "ca") : default_protocol_(default_protocol) {
        if (default_protocol_ == "ca") {
            SEVCHK(ca_context_create(ca_enable_preemptive_callback), "ca_context_create");
            ca_initialized_ = true;
        } else if (default_protocol_ == "pva") {
            pvxs_ctxt_ = pvxs::client::Context::fromEnv();
            pva_initialized_ = true;
        } else {
            throw std::runtime_error("Unknown protocol " + default_protocol);
        }
    }

    ~Context() {
        channel_map_.clear();
        if (ca_current_context()) {
            ca_context_destroy();
        }
    }

    // Delete copy constructor and move assignment operator
    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;

    /// \brief Returns true if all PVs in the context are connected, otherwise false.
    bool all_connected() const {
        for (auto& [_, pv] : channel_map_) {
            if (!pv->connected()) {
                return false;
            }
        }
        return true;
    }

    /// \brief Bind a local variable to a PV.
    ///
    /// The PV must have been added with connect() first. The bound variable
    /// must outlive the Context. Multiple variables (including different types)
    /// can be bound to the same PV. Also starts the monitor if not already
    /// running.
    ///
    /// \param var  Reference to the local variable.
    /// \param pv_name  The PV name, must match a previous connect() call.
    /// \param field_path  Field path for PVA structured PVs (default: "value").
    template <typename T>
    void bind(T& var, const std::string& pv_name, const std::string& field_path = "value") {
        auto [pv_name_strip, _] = parse_protocol_pv_name(pv_name);
        get_channel(pv_name_strip).bind(var, field_path);
    }

    /// \brief Sync all channels, updating every bound variable with new data.
    ///
    /// Returns true if at least one channel had new data since the last sync().
    /// Call this periodically from your application loop.
    bool sync() {
        std::lock_guard lock(mutex_);
        bool new_data = false;
        for (auto& [pv_name, channel] : channel_map_) {
            if (channel->sync()) {
                new_data = true;
            }
        }
        return new_data;
    }

    /// \brief Get a reference to a channel.
    ///
    /// Throws if the PV has not been registered in the Context.
    /// The returned reference can be used to check connection status or
    /// access the underlying channel directly.
    ///
    /// \param pv_name  The PV name (e.g. "MyIOC:name.VAL").
    ChannelBase& get_channel(const std::string& pv_name) {
        std::lock_guard lock(mutex_);
        auto [pv_name_strip, _] = parse_protocol_pv_name(pv_name);
        auto it = channel_map_.find(pv_name_strip);
        if (it == channel_map_.end()) {
            throw std::runtime_error(pv_name_strip + " not registered");
        }
        return *it->second;
    }

    /// \brief Shorthand for get_channel().
    /// \overload
    ChannelBase& operator[](const std::string& pv_name) { return get_channel(pv_name); }

    /// \brief Write a value to a PV (fire-and-forget).
    ///
    /// The PV must have been added with connect() first.
    /// Returns true if the value was sent, false if disconnected.
    ///
    /// \param pv_name  The PV name, must match a previous connect() call.
    /// \param value    The value to write.
    template <typename T>
    bool put(const std::string& pv_name, const T& value, const std::string& field_path = "value") {
        return get_channel(pv_name).put(value, field_path);
    }

    /// \brief Synchronous get.
    ///
    /// Throws if the PV has not been registered in the Context.
    /// Returns an optional<T> which is nullopt on failure/timeout.
    ///
    /// \param pv_name  The PV name (e.g. "MyIOC:name.VAL").
    /// \param timeout  Optional timeout.
    template <typename T>
    std::optional<T> get(const std::string& pv_name, const std::string& field_path, double timeout = 1.0) {
        return get_channel(pv_name).get<T>(field_path, timeout);
    }

    template <typename T>
    std::optional<T> get(const std::string& pv_name, double timeout = 1.0) {
        return get_channel(pv_name).get<T>(timeout);
    }

    /// \brief Starts a monitor for the given PV
    ///
    /// \param pv_name The PV name (e.g. "MyIOC:name.VAL").
    void monitor(const std::string& pv_name) { get_channel(pv_name).monitor(); }

    /// \brief Returns the latest value from the monitor as the request type.
    ///
    /// Returns The latest value from the monitor as std::optional<T>.
    /// If the conversion to T fails, the returned value is std::nullopt;
    ///
    /// \param pv_name The PV name (e.g. "MyIOC:name.VAL").
    template <typename T>
    std::optional<T> peek(const std::string& pv_name) {
        return get_channel(pv_name).peek<T>();
    }

    /// \brief Add a PV to the context and connect
    ///
    /// \param pv_name The PV name, possibly with protocol.
    /// \param timeout Optional connection timeout. If 0, returns immediately
    /// and doesn't wait for connection
    ChannelBase& connect(const std::string& pv_name, double timeout = 0.0) {
        auto [pv_name_strip, protocol] = parse_protocol_pv_name(pv_name);
        ChannelBase& channel_base = emplace_pv_in_channel_map(protocol, pv_name_strip);
        wait_connection(pv_name_strip, timeout);
        return channel_base;
    }

    /// \brief Add several PVs to the context and connect
    ///
    /// \param pv_names List of PV names, possibly with protocol.
    /// \param timeout Optional connection timeout. If 0, returns immediately
    /// and doesn't wait for connection
    void connect(std::initializer_list<std::string> pv_names, double timeout = 0.0) {
        for (const auto& pv_name : pv_names) {
            auto [pv_name_strip, protocol] = parse_protocol_pv_name(pv_name);
            emplace_pv_in_channel_map(protocol, pv_name_strip);
            wait_connection(pv_name_strip, timeout);
        }
    }

    /// \brief Add several PVs with a common prefix to the context and connect
    ///
    /// \param prefix Prefix to apply to PV names.
    /// \param pv_names List of PV names, possibly with protocol.
    /// \param timeout Optional connection timeout. If 0, returns immediately
    /// and doesn't wait for connection
    void connect(const std::string& prefix, std::initializer_list<std::string> pv_names,
                 double timeout = 0.0) {
        for (const std::string& pv_name : pv_names) {
            auto [pv_name_strip, protocol] = parse_protocol_pv_name(pv_name);
            emplace_pv_in_channel_map(protocol, prefix + pv_name_strip);
            wait_connection(prefix + pv_name_strip, timeout);
        }
    }

  private:
    std::mutex mutex_;
    const std::string default_protocol_ = "ca";
    bool ca_initialized_ = false;
    bool pva_initialized_ = false;
    std::unordered_map<std::string, std::unique_ptr<ChannelBase>> channel_map_;
    std::optional<pvxs::client::Context> pvxs_ctxt_;

    /// \brief Waits for the given PV to connect before returning. Throws on timeout.
    void wait_connection(const std::string& pv_name, double timeout) {
        if (timeout <= 0.0) {
            return;
        }
        const auto start = std::chrono::steady_clock::now();
        while (true) {
            const long timeout_ms = timeout * 1000.0;
            const auto now = std::chrono::steady_clock::now();
            const auto elap = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
            if (elap.count() > timeout_ms) {
                throw std::runtime_error("Timed out waiting to connect to " + pv_name);
            }
            if (get_channel(pv_name).connected()) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    ChannelBase& emplace_pv_in_channel_map(const std::string& protocol, const std::string& pv_name) {
        auto it = channel_map_.find(pv_name);
        if (it == channel_map_.end()) {
            if (protocol == "ca") {
                if (!ca_initialized_) {
                    SEVCHK(ca_context_create(ca_enable_preemptive_callback), "ca_context_create");
                    ca_initialized_ = true;
                }
                it = channel_map_.emplace(pv_name, std::make_unique<CAChannel>(pv_name)).first;
            } else if (protocol == "pva") {
                if (!pva_initialized_) {
                    pvxs_ctxt_ = pvxs::client::Context::fromEnv();
                    pva_initialized_ = true;
                }
                it = channel_map_.emplace(pv_name, std::make_unique<PVAChannel>(*pvxs_ctxt_, pv_name)).first;
            }
        }
        return *it->second;
    }

    std::pair<std::string, std::string> parse_protocol_pv_name(const std::string& pv) {
        auto id = pv.find("://");
        if (id != std::string::npos) {
            auto protocol = pv.substr(0, id);
            auto pv_name = pv.substr(id + 3);
            if (protocol == "ca") {
                return {pv_name, "ca"};
            } else if (protocol == "pva") {
                return {pv_name, "pva"};
            } else {
                throw std::runtime_error("Unknown protocol " + protocol);
            }
        }
        return {pv, default_protocol_};
    }
};

} // namespace pace
