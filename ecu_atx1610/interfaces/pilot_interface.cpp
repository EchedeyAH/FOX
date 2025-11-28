#include "pilot_interface.hpp"
#include <iostream>

namespace interfaces {

PilotInterface::PilotInterface() 
    : session_manager_(std::make_unique<common::SessionManager>()) {
}

PilotInterface::~PilotInterface() {
    // El destructor de SessionManager se encargará de finalizar la sesión si está activa
}

bool PilotInterface::start_session(const std::string& pilot_name,
                                   const std::string& notes,
                                   const std::string& conditions) {
    return session_manager_->start_session(pilot_name, notes, conditions);
}

bool PilotInterface::end_session() {
    return session_manager_->end_session();
}

std::optional<common::SessionInfo> PilotInterface::get_current_session_info() const {
    return session_manager_->get_current_session();
}

bool PilotInterface::is_session_active() const {
    return session_manager_->is_session_active();
}

std::string PilotInterface::get_session_directory() const {
    auto session = session_manager_->get_current_session();
    if (session.has_value()) {
        return session->session_directory.string();
    }
    return "";
}

std::string PilotInterface::get_current_pilot_name() const {
    auto session = session_manager_->get_current_session();
    if (session.has_value()) {
        return session->pilot_name;
    }
    return "";
}

} // namespace interfaces
