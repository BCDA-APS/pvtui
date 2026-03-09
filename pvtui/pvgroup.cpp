#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>

#include <pvtui/pvgroup.hpp>
#include <type_traits>

namespace pvd = epics::pvData;

namespace {

template <typename T>
struct is_vector : std::false_type {};

template <typename T, typename Alloc>
struct is_vector<std::vector<T, Alloc>> : std::true_type {};

template <typename T>
inline constexpr bool is_vector_v = is_vector<T>::value;

} // namespace

namespace pvtui {

void ConnectionMonitor::connectEvent(const pvac::ConnectEvent& event) {
    connected_.store(event.connected, std::memory_order_relaxed);
}

bool ConnectionMonitor::connected() const { return connected_.load(std::memory_order_relaxed); }

PVHandler::PVHandler(pvac::ClientProvider& provider, const std::string& pv_name)
    : channel(provider.connect(pv_name)), name(pv_name), monitor_(channel.monitor(this)),
      connection_monitor_(std::make_shared<ConnectionMonitor>()) {
    channel.addConnectListener(connection_monitor_.get());
}

void PVHandler::monitorEvent(const pvac::MonitorEvent& evt) {
    switch (evt.event) {
    case pvac::MonitorEvent::Data:
        while (monitor_.poll()) {
            this->update_monitored_variable(monitor_.root.get());
        }
        break;
    case pvac::MonitorEvent::Disconnect:
        break;
    case pvac::MonitorEvent::Fail:
        break;
    case pvac::MonitorEvent::Cancel:
        break;
    }
}

bool PVHandler::connected() const { return connection_monitor_->connected(); }

namespace {

size_t get_precision(const epics::pvData::PVStructure* pstruct) {
    size_t prec = 4;
    if (auto display_struct = pstruct->getSubField<epics::pvData::PVStructure>("display")) {
        if (auto format_field = display_struct->getSubField<epics::pvData::PVString>("format")) {
            std::string fstr = format_field->get();
            size_t iF = fstr.find('F');
            size_t idot = fstr.find('.');
            if (iF != std::string::npos && idot != std::string::npos) {
                try {
                    prec = std::stoi(fstr.substr(idot + 1));
                } catch (...) {
                }
            }
        }
    }
    return prec;
}

// type map for convenience in vector<T> branch
// of visitor in update_monitored_variable
template <typename T>
struct pvd_type_map;
template <>
struct pvd_type_map<int> {
    using array_type = pvd::PVIntArray;
};
template <>
struct pvd_type_map<double> {
    using array_type = pvd::PVDoubleArray;
};
template <>
struct pvd_type_map<std::string> {
    using array_type = pvd::PVStringArray;
};

} // namespace

void PVHandler::update_monitored_variable(const pvd::PVStructure* pstruct) {

    // Copy all monitor slots under lock
    std::unordered_map<std::type_index, MonitorVar> slots_copy;
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        for (auto& [type_id, slot] : monitor_slots_) {
            if (!std::holds_alternative<std::monostate>(slot.data)) {
                slots_copy.emplace(type_id, slot.data);
            }
        }
    }

    if (slots_copy.empty())
        return;

    // Extract data from PVStructure into each slot's type
    for (auto& [type_id, incoming] : slots_copy) {
        bool success = false;
        std::visit(
            [&](auto& var) {
                using VarType = std::decay_t<decltype(var)>;

                if constexpr (std::is_arithmetic_v<VarType>) {
                    if (auto val_field = pstruct->getSubField<pvd::PVScalar>("value")) {
                        var = val_field->getAs<VarType>();
                        success = true;
                    }
                }

                else if constexpr (std::is_same_v<VarType, std::string>) {
                    if (auto val_field = pstruct->getSubField<pvd::PVString>("value")) {
                        var = val_field->getAs<std::string>();
                        success = true;
                    } else if (auto val_field = pstruct->getSubField<pvd::PVByteArray>("value")) {
                        auto pbytearr = val_field->view();
                        var.assign(pbytearr.begin(), pbytearr.end());
                        success = true;
                    } else if (auto val_field = pstruct->getSubField("value")) {
                        std::ostringstream oss;
                        oss << std::fixed << std::setprecision(get_precision(pstruct));
                        val_field->dumpValue(oss);
                        var = oss.str();
                        success = true;
                    }
                }

                else if constexpr (std::is_same_v<VarType, PVEnum>) {
                    auto pchoices = pstruct->getSubField<pvd::PVStringArray>("value.choices");
                    auto pindex = pstruct->getSubField<pvd::PVInt>("value.index");
                    if (pchoices && pindex) {
                        pvd::shared_vector<const std::string> choices = pchoices->view();
                        size_t index = pindex->getAs<size_t>();
                        if (choices.size() > index) {
                            var.index = index;
                            var.choice = choices.at(index);
                            var.choices.assign(choices.begin(), choices.end());
                            success = true;
                        }
                    }
                }

                else if constexpr (is_vector_v<VarType>) {
                    using ElementType = typename VarType::value_type;
                    using PVDArray = typename pvd_type_map<ElementType>::array_type;
                    if (auto parr = pstruct->getSubField<PVDArray>("value")) {
                        auto vec = parr->view();
                        var.assign(vec.begin(), vec.end());
                        success = true;
                    }
                }

                else {
                    success = false;
                }
            },
            incoming);

        if (!success) {
            std::cerr << "Incompatible types for monitor: " << this->channel.name() << "\n";
            std::abort();
        }
    }

    // Write updated values back under lock
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        for (auto& [type_id, incoming] : slots_copy) {
            monitor_slots_[type_id].data = std::move(incoming);
        }
    }
    new_data_.store(true, std::memory_order_release);
}

bool PVHandler::sync() {
    if (!new_data_.load(std::memory_order_acquire))
        return false;

    const std::lock_guard<std::mutex> lock(mutex_);
    for (auto& [type_id, slot] : monitor_slots_) {
        for (auto& task : slot.tasks) {
            task(slot.data);
        }
    }

    new_data_.store(false, std::memory_order_relaxed);
    return true;
}

PVGroup::PVGroup(pvac::ClientProvider& provider, const std::vector<std::string>& pv_names)
    : provider_(provider) {
    for (const auto& name : pv_names) {
        this->add(name);
    }
}

PVGroup::PVGroup(pvac::ClientProvider& provider) : provider_(provider) {}

void PVGroup::add(const std::string& pv_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!pv_map.count(pv_name)) {
        pv_map.emplace(pv_name, std::make_shared<PVHandler>(provider_, pv_name));
    }
}

PVHandler& PVGroup::get_pv(const std::string& pv_name) {
    auto it = pv_map.find(pv_name);
    if (it == pv_map.end()) {
        throw std::runtime_error(pv_name + " not registered in PVGroup");
    }
    return *it->second;
}

std::shared_ptr<PVHandler> PVGroup::get_pv_shared(const std::string& pv_name) {
    auto it = pv_map.find(pv_name);
    if (it == pv_map.end()) {
        throw std::runtime_error(pv_name + " not registered in PVGroup");
    }
    return it->second;
}

PVHandler& PVGroup::operator[](const std::string& pv_name) { return this->get_pv(pv_name); }

bool PVGroup::sync() {
    std::lock_guard<std::mutex> lock(mutex_);
    bool new_data = false;
    for (auto& [name, pv] : pv_map) {
        if (pv->sync()) {
            new_data = true;
        }
    }
    return new_data;
}
} // namespace pvtui
