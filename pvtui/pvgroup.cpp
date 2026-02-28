#include <pvtui/pvgroup.hpp>
#include <regex>

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
            this->get_monitored_variable(monitor_.root.get());
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

void PVHandler::get_monitored_variable(const epics::pvData::PVStructure* pstruct) {
    namespace pvd = epics::pvData;

    MonitorVar incoming;
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        if (std::holds_alternative<std::monostate>(monitor_var_internal_))
            return;
        incoming = monitor_var_internal_;
    }

    // get the display precision
    constexpr int DEFAULT_PRECISION = 4;
    static std::regex fmt_regex(R"(F\d+\.(\d+))");
    int precision = DEFAULT_PRECISION;
    auto display_struct = pstruct->getSubField<pvd::PVStructure>("display");
    if (display_struct) {
        auto format_field = display_struct->getSubField<pvd::PVString>("format");
        if (format_field) {
            std::string prec_str = format_field->get();
            std::smatch match;
            if (std::regex_match(prec_str, match, fmt_regex) && match.size() == 2) {
                precision = std::stoi(match[1]);
            }
        }
    }

    bool success = false;
    std::visit(
        [&](auto& var) {
            using VarType = std::decay_t<decltype(var)>;

            if constexpr (std::is_arithmetic_v<VarType> || std::is_same_v<VarType, std::string>) {
                if (auto val_field = pstruct->getSubField<pvd::PVScalar>("value")) {
		    // TODO: if VarType==string, but pfield != pvScalar::string,
		    // need to deal with precision
                    var = val_field->getAs<VarType>();
                    success = true;
                }
            }

	    // When does byte[] come up?
            // else if constexpr (std::is_same_v<VarType, std::string>) {
                // std::string type_str = pstruct->getStructure()->getField("value")->getID();
                // if (type_str == "string") {
                    // if (auto val_field = pstruct->getSubField<pvd::PVString>("value")) {
                        // var = val_field->getAs<std::string>();
                        // success = true;
                    // }
                // } else if (type_str == "byte[]") {
                    // pvd::shared_vector<const signed char> vals =
                        // pstruct->getSubFieldT<pvd::PVByteArray>("value")->view();
                    // auto last_ind = std::find_if(vals.rbegin(), vals.rend(), [](const signed char c) {
                        // return std::isalnum(static_cast<unsigned char>(c));
                    // });
                    // var = std::string(vals.begin(), last_ind.base());
                    // success = true;
                // } else {
                    // std::ostringstream oss;
                    // oss << std::fixed << std::setprecision(precision);
                    // if (auto val_field = pstruct->getSubField("value")) {
                        // val_field->dumpValue(oss);
                        // var = oss.str();
                        // success = true;
                    // }
                // }
            // }

            else if constexpr (std::is_same_v<VarType, PVEnum>) {
                pvd::shared_vector<const std::string> choices =
                    pstruct->getSubFieldT<pvd::PVStringArray>("value.choices")->view();
                size_t index = pstruct->getSubFieldT<pvd::PVInt>("value.index")->getAs<int>();
                if (choices.size() > index) {
                    var.index = index;
                    var.choice = choices.at(index);
                    if (var.choices.size() != choices.size()) {
                        var.choices.resize(choices.size());
                    }
                    std::copy(choices.begin(), choices.end(), var.choices.begin());
                    success = true;
                }
            }

            else if constexpr (std::is_same_v<VarType, std::vector<double>>) {
                pvd::shared_vector<const double> vals =
                    pstruct->getSubFieldT<pvd::PVDoubleArray>("value")->view();
                if (var.size() != vals.size()) {
                    var.resize(vals.size());
                }
                std::copy(vals.begin(), vals.end(), var.begin());
                success = true;
            }

            else if constexpr (std::is_same_v<VarType, std::vector<int>>) {
		std::cout << "here?\n";
                pvd::shared_vector<const int> vals = pstruct->getSubFieldT<pvd::PVIntArray>("value")->view();
                if (var.size() != vals.size()) {
                    var.resize(vals.size());
                }
                std::copy(vals.begin(), vals.end(), var.begin());
                success = true;
            }

            else if constexpr (std::is_same_v<VarType, std::vector<std::string>>) {
                pvd::shared_vector<const std::string> vals =
                    pstruct->getSubFieldT<pvd::PVStringArray>("value")->view();
                if (var.size() != vals.size()) {
                    var.resize(vals.size());
                }
                std::copy(vals.begin(), vals.end(), var.begin());
                success = true;
            }

            else {
                // unsupported type
                success = false;
            }
        },
        incoming);

    if (success) {
        {
            const std::lock_guard<std::mutex> lock(mutex_);
            monitor_var_internal_ = std::move(incoming);
        }
        new_data_.store(true, std::memory_order_release);
    } else {
	std::cerr << "Incompatible types for monitor: " << this->channel.name() << "\n";
	std::abort();
    }
}

bool PVHandler::sync() {
    if (!new_data_.load(std::memory_order_acquire))
        return false;

    const std::lock_guard<std::mutex> lock(mutex_);
    for (auto& task : sync_tasks_) {
        task(monitor_var_internal_);
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
