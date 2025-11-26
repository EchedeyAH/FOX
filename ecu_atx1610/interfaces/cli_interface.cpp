#include "../common/logging.hpp"
#include "../common/types.hpp"

#include <iostream>

namespace interfaces {

class CliInterface {
public:
    void print_snapshot(const common::SystemSnapshot &snapshot)
    {
        std::cout << "SOC: " << snapshot.battery.state_of_charge << "% | "
                  << "Vbat: " << snapshot.battery.pack_voltage_mv << " mV | "
                  << "Ibat: " << snapshot.battery.pack_current_ma << " mA" << std::endl;
    }
};

} // namespace interfaces
