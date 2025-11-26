#pragma once

#include "../common/interfaces.hpp"
#include "../common/logging.hpp"

#include <queue>
#include <string>

namespace comunicacion_can {

class SocketCanInterface : public common::ICanDriver {
public:
    explicit SocketCanInterface(std::string interface_name) : interface_name_{std::move(interface_name)} {}

    bool start() override;
    void stop() override;

    bool send(const common::CanFrame &frame) override;
    std::optional<common::CanFrame> receive() override;

private:
    std::string interface_name_;
    std::queue<common::CanFrame> loopback_;
    bool ready_{false};
};

} // namespace comunicacion_can
