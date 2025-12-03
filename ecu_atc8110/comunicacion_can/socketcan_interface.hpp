#pragma once

#include "../common/interfaces.hpp"
#include "../common/logging.hpp"

#include <string>
#include <cstdint>

// Linux SocketCAN headers
#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>

namespace comunicacion_can {

class SocketCanInterface : public common::ICanDriver {
public:
    explicit SocketCanInterface(std::string interface_name) : interface_name_{std::move(interface_name)} {}
    ~SocketCanInterface();

    bool start() override;
    void stop() override;

    bool send(const common::CanFrame &frame) override;
    std::optional<common::CanFrame> receive() override;

    // Configurar filtros CAN
    bool set_filter(uint32_t can_id, uint32_t can_mask = 0x7FF);
    
    // Obtener estadísticas de errores
    struct ErrorStats {
        uint32_t tx_errors{0};
        uint32_t rx_errors{0};
        uint32_t bus_off_count{0};
    };
    ErrorStats get_error_stats() const { return error_stats_; }

private:
    std::string interface_name_;
    int socket_fd_{-1};
    bool ready_{false};
    ErrorStats error_stats_;
};

} // namespace comunicacion_can
