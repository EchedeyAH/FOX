#include "cli_interface.hpp"

#include <iostream>

namespace interfaces {

void CliInterface::print_snapshot(const common::SystemSnapshot &snapshot) const
{
    std::cout << "SOC: " << snapshot.battery.state_of_charge << "% | "
              << "Vbat: " << snapshot.battery.pack_voltage_mv << " mV | "
              << "Ibat: " << snapshot.battery.pack_current_ma << " mA | "
              << "Pedal: " << snapshot.vehicle.accelerator << '\n';
}

} // namespace interfaces
