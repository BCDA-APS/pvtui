#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include <pv/caProvider.h>
#include <pva/client.h>

namespace pvtui {

/**
 * @brief Represents the state of an EPICS enumeration (e.g., mbbo/mbbi).
 */
struct PVEnum {
    int index = 0;                    ///< The current integer index of the selected choice.
    std::vector<std::string> choices; ///< The list of all available string choices for the enum.
    std::string choice = "";          ///< The string value of the currently selected choice.
};

/**
 * @brief A variant type that holds a variable monitored by a PV.
 *
 * This allows a single mechanism to update variables of different types.
 */
using MonitorVar = std::variant<std::monostate, std::string, int, double, std::vector<std::string>,
                                std::vector<int>, std::vector<double>, PVEnum>;

/**
 * @brief Monitors a pvac::ClientChannel's connection status.
 */
class ConnectionMonitor : public pvac::ClientChannel::ConnectCallback {
  public:
    /**
     * @brief Constructs a ConnectionMonitor.
     */
    ConnectionMonitor() = default;

    /**
     * @brief Destroys the ConnectionMonitor.
     */
    virtual ~ConnectionMonitor() override = default;

    /**
     * @brief Callback invoked when the channel's connection status changes.
     * @param event The connection event details provided by the client channel.
     */
    virtual void connectEvent(const pvac::ConnectEvent& event) override final;

    /**
     * @brief Checks if connected.
     * @return True if connected, false otherwise.
     */
    bool connected() const;

  private:
    std::atomic<bool> connected_{false}; ///< Connection status flag.
};

/**
 * @brief Manages a single EPICS Process Variable (PV).
 *
 * Handles connection, monitoring, and value updates for a PV.
 */
struct PVHandler : public pvac::ClientChannel::MonitorCallback {
  public:
    pvac::ClientChannel channel; ///< PVA client channel.
    std::string name;            ///< Name of the process variable.

    /**
     * @brief Constructs a PVHandler.
     * @param provider PVA client provider.
     * @param pv_name Name of the process variable.
     */
    PVHandler(pvac::ClientProvider& provider, const std::string& pv_name);

    /**
     * @brief Checks if the PV channel is connected.
     * @return True if connected, false otherwise.
     */
    bool connected() const;

    /**
     * @brief Safely copies the internal monitored value to the user variable.
     * @return True if new data is available, false otherwise.
     */
    bool sync();

    /**
     * @brief Registers a variable to be updated when the PV monitor receives new data and sync() is called.
     * @tparam T The type of the variable to monitor.
     * @param var A reference to the variable that will be updated.
     */
    template <typename T>
    void set_monitor(T& var) {
        if (std::holds_alternative<std::monostate>(monitor_var_internal_)) {
            monitor_var_internal_ = T{};
        }

        if (!std::holds_alternative<T>(monitor_var_internal_)) {
            throw std::runtime_error("Cannot set multiple monitors of different types for a single PV: " +
                                     name);
        }

        sync_tasks_.push_back([&var](const MonitorVar& latest_data) {
            if (auto* val = std::get_if<T>(&latest_data)) {
                var = *val;
            }
        });
    }

    /**
     * @brief Gets the underlying PVA monitor instance.
     * @return A reference to the pvac::Monitor object.
     */
    pvac::Monitor& get_monitor() { return monitor_; }

    /**
     * @brief Gets a shared_ptr to the ConnectionMonitor
     * @return A shared_ptr to the ConnectionMonitor
     */
    std::shared_ptr<ConnectionMonitor> get_connection_monitor() const { return connection_monitor_; }

  private:
    std::mutex mutex_;
    pvac::Monitor monitor_;                                 ///< PVA data monitor.
    MonitorVar monitor_var_internal_;                       ///< Internal variable updated by monitor
    std::shared_ptr<ConnectionMonitor> connection_monitor_; ///< Monitors connection status.
    std::vector<std::function<void(const MonitorVar&)>>
        sync_tasks_; ///< Functions to copy internal value to user value
    std::atomic<bool> new_data_ = false;

    /**
     * @brief Callback invoked when a monitor event occurs (e.g., new data).
     * @param evt The monitor event containing the new data and status.
     */
    void monitorEvent(const pvac::MonitorEvent& evt) override final;

    /**
     * @brief Extracts the PV value from the event and copies it to
     * monitored variable via the sync callback
     * @param pstruct A pointer to the PVStructure containing the new data.
     */
    void update_monitored_variable(const epics::pvData::PVStructure* pstruct);
};

/**
 * @brief Manages a collection of EPICS Process Variables (PVs).
 *
 * This class provides a centralized way to add, access, and monitor a group of
 * PVs, handling the underlying connections and data updates.
 */
struct PVGroup {
  public:
    /**
     * @brief Constructs a PVGroup and initializes it with a list of PVs.
     * @param provider The PVA client provider.
     * @param pv_list A list of PV names to add to the group.
     */
    PVGroup(pvac::ClientProvider& provider, const std::vector<std::string>& pv_list);

    /**
     * @brief Constructs an empty PVGroup.
     * @param provider The PVA client provider.
     */
    PVGroup(pvac::ClientProvider& provider);

    /**
     * @brief Adds a new PV to the group.
     * @param pv_name The name of the PV to add.
     * @throws std::runtime_error if a PV with the same name already exists.
     */
    void add(const std::string& pv_name);

    /**
     * @brief Registers a variable to be updated by a specific PV in the group.
     * @tparam T The type of the variable to monitor.
     * @param pv_name The name of the PV to monitor.
     * @param var A reference to the variable that will be updated.
     * @throws std::runtime_error if the PV is not found in the group.
     */
    template <typename T>
    void set_monitor(const std::string& pv_name, T& var) {
        PVHandler& pv = this->get_pv(pv_name);
        pv.set_monitor(var);
    }

    /**
     * @brief Retrieves a PVHandler from the group by its name.
     * @param pv_name The name of the PV to retrieve.
     * @return A reference to the corresponding PVHandler object.
     * @throws std::runtime_error if the PV is not found.
     */
    PVHandler& get_pv(const std::string& pv_name);

    /**
     * @brief Returns a shared_ptr<PVHandler> from the group by its name.
     * @param pv_name The name of the PV to retrieve.
     * @return A shared_ptr<PVHandler> to the corresponding PVHandler object.
     * @throws std::runtime_error if the PV is not found.
     */
    std::shared_ptr<PVHandler> get_pv_shared(const std::string& pv_name);

    /**
     * @brief Provides array-like access to a PVHandler in the group.
     * @param pv_name The name of the PV to access.
     * @return A reference to the corresponding PVHandler object.
     * @throws std::runtime_error if the PV is not found.
     */
    PVHandler& operator[](const std::string& pv_name);

    /**
     * @brief Checks if any PV in the group has received new data.
     * @return True if new data is available in any monitor, false otherwise.
     */
    bool sync();

  private:
    std::mutex mutex_;
    pvac::ClientProvider& provider_;                                    ///< PVA client provider.
    std::unordered_map<std::string, std::shared_ptr<PVHandler>> pv_map; ///< Map of PVs by name.
};
} // namespace pvtui
