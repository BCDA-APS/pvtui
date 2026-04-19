#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>

#include <pv/caProvider.h>
#include <pva/client.h>
#include <pvtui/pvtui.hpp>

// To catch CTRL+C to quit
volatile std::sig_atomic_t g_signal_caught = 0;
void signal_handler(int signal) {
    if (signal == SIGINT) {
        std::cout << "\nKeyboard interrupt (Ctrl+C)" << std::endl;
        g_signal_caught = 1;
    }
}

template<typename T>
void print_vec(const std::vector<T>& vec) {
    std::cout << "[";
    for (auto it = vec.begin(); it != vec.end(); it++) {
        std::cout << *it;
        if (std::next(it) != vec.end()) {
            std::cout << ",";
        }
    }
    std::cout << "]\n";
}

int main(int argc, char *argv[]) {

    std::string prefix;
    if (argc < 2) {
        std::cout << "Please provide IOC prefix" << std::endl;
        return EXIT_FAILURE;
    } else {
        prefix = argv[1];
    }

    signal(SIGINT, signal_handler);

    // Start the client
    epics::pvAccess::ca::CAClientFactory::start();
    pvac::ClientProvider provider("pva");

    // Create the group and add our PVs
    pvtui::PVGroup pvgroup(provider, {
        prefix+"m1.DESC",
        prefix+"m1.RBV",
        prefix+"double_array.VAL",
        prefix+"string_array.VAL",
        prefix+"Position",
    });

    double rbv_double;
    pvgroup.bind<double>(rbv_double, prefix+"m1.RBV");

    std::string rbv_string;
    pvgroup.bind<std::string>(rbv_string, prefix+"m1.RBV");

    std::string desc;
    pvgroup.bind<std::string>(desc, prefix+"m1.DESC");

    // Using PVA structured data
    double pos_x = 0.0;
    pvgroup.bind<double>(pos_x, prefix+"Position", "X.value");
    double pos_y = 0.0;
    pvgroup.bind<double>(pos_y, prefix+"Position", "Y.value");
    double pos_z = 0.0;
    pvgroup.bind<double>(pos_z, prefix+"Position", "Z.value");

    std::vector<double> double_arr;
    pvgroup.bind<std::vector<double>>(double_arr, prefix+"double_array.VAL");

    std::string string_arr;
    pvgroup.bind<std::string>(string_arr, prefix+"string_array.VAL");

    while (g_signal_caught == 0) {
        if (pvgroup.sync()) {
            std::cout << "DESC = " << desc << std::endl;

            std::cout << "RBV[double] = " << rbv_double << std::endl;

            std::cout << "RBV[string] = " << rbv_string << std::endl;

            std::cout << "POS X[double] = " << pos_x << std::endl;
            std::cout << "POS Y[double] = " << pos_y << std::endl;
            std::cout << "POS Z[double] = " << pos_z << std::endl;

            std::cout << "double_array = ";
            print_vec(double_arr);

            std::cout << "string_array = ";
            // print_vec(string_arr);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
