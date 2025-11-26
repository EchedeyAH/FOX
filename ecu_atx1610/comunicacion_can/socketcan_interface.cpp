#include "socketcan_interface.hpp"

namespace comunicacion_can {

bool SocketCanInterface::start()
{
    ready_ = true;
    LOG_INFO("SocketCAN", "Interfaz " + interface_name_ + " inicializada en modo simulación");
    return true;
}

void SocketCanInterface::stop()
{
    ready_ = false;
    LOG_INFO("SocketCAN", "Interfaz " + interface_name_ + " detenida");
}

bool SocketCanInterface::send(const common::CanFrame &frame)
{
    if (!ready_) {
        LOG_ERROR("SocketCAN", "Interfaz no inicializada");
        return false;
    }
    loopback_.push(frame);
    LOG_DEBUG("SocketCAN", "TX id=" + std::to_string(frame.id) + " bytes=" + std::to_string(frame.payload.size()));
    return true;
}

std::optional<common::CanFrame> SocketCanInterface::receive()
{
    if (!ready_ || loopback_.empty()) {
        return std::nullopt;
    }
    auto frame = loopback_.front();
    loopback_.pop();
    LOG_DEBUG("SocketCAN", "RX id=" + std::to_string(frame.id));
    return frame;
}

} // namespace comunicacion_can
