#include "socketcan_interface.hpp"

#include <cstring>
#include <fcntl.h>
#include <errno.h>

namespace comunicacion_can {

SocketCanInterface::~SocketCanInterface()
{
    stop();
}

bool SocketCanInterface::start()
{
    if (ready_) {
        LOG_WARN("SocketCAN", "Interfaz ya inicializada");
        return true;
    }

    // Crear socket CAN
    socket_fd_ = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (socket_fd_ < 0) {
        LOG_ERROR("SocketCAN", "Error creando socket CAN: " + std::string(strerror(errno)));
        return false;
    }

    // Obtener índice de la interfaz de red
    struct ifreq ifr;
    std::strncpy(ifr.ifr_name, interface_name_.c_str(), IFNAMSIZ - 1);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';
    
    if (ioctl(socket_fd_, SIOCGIFINDEX, &ifr) < 0) {
        LOG_ERROR("SocketCAN", "Interfaz " + interface_name_ + " no encontrada: " + std::string(strerror(errno)));
        close(socket_fd_);
        socket_fd_ = -1;
        return false;
    }

    // Vincular socket a la interfaz CAN
    struct sockaddr_can addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(socket_fd_, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        LOG_ERROR("SocketCAN", "Error vinculando socket: " + std::string(strerror(errno)));
        close(socket_fd_);
        socket_fd_ = -1;
        return false;
    }

    // Configurar socket en modo no-bloqueante para recepción
    int flags = fcntl(socket_fd_, F_GETFL, 0);
    if (flags >= 0) {
        fcntl(socket_fd_, F_SETFL, flags | O_NONBLOCK);
    }

    ready_ = true;
    LOG_INFO("SocketCAN", "Interfaz " + interface_name_ + " inicializada correctamente");
    return true;
}

void SocketCanInterface::stop()
{
    if (socket_fd_ >= 0) {
        close(socket_fd_);
        socket_fd_ = -1;
    }
    ready_ = false;
    LOG_INFO("SocketCAN", "Interfaz " + interface_name_ + " detenida");
}

bool SocketCanInterface::send(const common::CanFrame &frame)
{
    if (!ready_ || socket_fd_ < 0) {
        LOG_ERROR("SocketCAN", "Interfaz no inicializada");
        return false;
    }

    // Convertir de common::CanFrame a struct can_frame de Linux
    struct can_frame can_msg;
    std::memset(&can_msg, 0, sizeof(can_msg));
    
    can_msg.can_id = frame.id;
    if (frame.extended) {
        can_msg.can_id |= CAN_EFF_FLAG;  // Extended Frame Format
    }
    
    can_msg.can_dlc = static_cast<uint8_t>(frame.payload.size());
    if (can_msg.can_dlc > 8) {
        LOG_ERROR("SocketCAN", "Payload demasiado grande: " + std::to_string(can_msg.can_dlc));
        return false;
    }
    
    std::memcpy(can_msg.data, frame.payload.data(), can_msg.can_dlc);

    // Enviar frame
    ssize_t nbytes = write(socket_fd_, &can_msg, sizeof(can_msg));
    if (nbytes != sizeof(can_msg)) {
        LOG_ERROR("SocketCAN", "Error enviando frame: " + std::string(strerror(errno)));
        error_stats_.tx_errors++;
        return false;
    }

    LOG_DEBUG("SocketCAN", "TX id=0x" + std::to_string(frame.id) + 
              " dlc=" + std::to_string(can_msg.can_dlc));
    return true;
}

std::optional<common::CanFrame> SocketCanInterface::receive()
{
    if (!ready_ || socket_fd_ < 0) {
        return std::nullopt;
    }

    struct can_frame can_msg;
    ssize_t nbytes = read(socket_fd_, &can_msg, sizeof(can_msg));
    
    if (nbytes < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // No hay datos disponibles (modo no-bloqueante)
            return std::nullopt;
        }
        LOG_ERROR("SocketCAN", "Error recibiendo frame: " + std::string(strerror(errno)));
        error_stats_.rx_errors++;
        return std::nullopt;
    }

    if (nbytes != sizeof(can_msg)) {
        LOG_ERROR("SocketCAN", "Frame incompleto recibido");
        error_stats_.rx_errors++;
        return std::nullopt;
    }

    // Convertir de struct can_frame a common::CanFrame
    common::CanFrame frame;
    frame.id = can_msg.can_id & CAN_EFF_MASK;
    frame.extended = (can_msg.can_id & CAN_EFF_FLAG) != 0;
    frame.payload.assign(can_msg.data, can_msg.data + can_msg.can_dlc);

    LOG_DEBUG("SocketCAN", "RX id=0x" + std::to_string(frame.id) + 
              " dlc=" + std::to_string(can_msg.can_dlc));
    
    return frame;
}

bool SocketCanInterface::set_filter(uint32_t can_id, uint32_t can_mask)
{
    if (!ready_ || socket_fd_ < 0) {
        LOG_ERROR("SocketCAN", "Interfaz no inicializada");
        return false;
    }

    struct can_filter filter;
    filter.can_id = can_id;
    filter.can_mask = can_mask;

    if (setsockopt(socket_fd_, SOL_CAN_RAW, CAN_RAW_FILTER, &filter, sizeof(filter)) < 0) {
        LOG_ERROR("SocketCAN", "Error configurando filtro CAN: " + std::string(strerror(errno)));
        return false;
    }

    LOG_INFO("SocketCAN", "Filtro CAN configurado: ID=0x" + std::to_string(can_id) + 
             " Mask=0x" + std::to_string(can_mask));
    return true;
}

} // namespace comunicacion_can
