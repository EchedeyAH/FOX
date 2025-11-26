#pragma once

#include "socketcan_interface.hpp"
#include "../common/interfaces.hpp"
#include "../common/types.hpp"

#include <string>

namespace comunicacion_can {

class CanManager : public common::ILifecycle {
public:
    explicit CanManager(std::string iface) : driver_{std::move(iface)} {}

    bool start() override
    {
        return driver_.start();
    }

    void stop() override
    {
        driver_.stop();
    }

    void publish_heartbeat()
    {
        common::CanFrame hb{0x100, {0xAA, 0x55, 0x01}, false};
        driver_.send(hb);
    }

    void publish_battery(const common::BatteryState &bat)
    {
        common::CanFrame frame{0x180, {}, false};
        frame.payload = {
            static_cast<uint8_t>(bat.state_of_charge),
            static_cast<uint8_t>(bat.alarm_level),
        };
        driver_.send(frame);
    }

    void process_rx(common::SystemSnapshot &snapshot)
    {
        if (auto frame = driver_.receive()) {
            snapshot.faults.warning = frame->id == 0x200;
            if (snapshot.faults.warning) {
                snapshot.faults.description = "Heartbeat supervisor";
            }
        }
    }

private:
    SocketCanInterface driver_;
};

} // namespace comunicacion_can
