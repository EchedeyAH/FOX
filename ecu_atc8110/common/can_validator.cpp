#include "can_validator.hpp"
#include "logging.hpp"

#include <algorithm>
#include <sstream>

namespace common {

// ============================================================================
// IMPLEMENTACIÓN DEL CAN VALIDATOR
// ============================================================================

CanValidator::CanValidator() {
    reset_all();
}

CanPeerStatus& CanValidator::get_peer_ref(uint8_t motor_id) {
    if (motor_id >= 1 && motor_id <= 4) {
        return peers_[motor_id];  // 1-4 → index 1-4
    } else if (motor_id == 0) {
        return peers_[0];  // BMS
    } else if (motor_id == 255) {
        return peers_[5];  // Supervisor
    }
    return peers_[0];  // Default to BMS
}

const CanPeerStatus* CanValidator::get_peer_ptr(uint8_t motor_id) const {
    if (motor_id >= 1 && motor_id <= 4) {
        return &peers_[motor_id];
    } else if (motor_id == 0) {
        return &peers_[0];
    } else if (motor_id == 255) {
        return &peers_[5];
    }
    return &peers_[0];
}

CanPeerStatus CanValidator::get_peer_status(uint8_t motor_id) const {
    const auto* peer = get_peer_ptr(motor_id);
    return peer ? *peer : CanPeerStatus{};
}

bool CanValidator::register_tx(uint8_t motor_id) {
    auto now = std::chrono::steady_clock::now();
    auto& peer = get_peer_ref(motor_id);
    
    peer.last_tx = now;
    peer.messages_sent++;
    
    LOG_DEBUG("CanValidator", "TX registered for peer " + std::to_string(motor_id));
    return true;
}

void CanValidator::register_tx_success(uint8_t motor_id) {
    auto& peer = get_peer_ref(motor_id);
    peer.responding = true;
    peer.consecutive_failures = 0;
    peer.retries = 0;
}

void CanValidator::register_rx(uint8_t motor_id) {
    auto now = std::chrono::steady_clock::now();
    auto& peer = get_peer_ref(motor_id);
    
    peer.last_rx = now;
    peer.messages_received++;
    peer.connected = true;
    peer.responding = true;
    peer.consecutive_failures = 0;
    
    // Actualizar rate limiting
    peer.msgs_last_second++;
    
    LOG_DEBUG("CanValidator", "RX from peer " + std::to_string(motor_id) + 
              " (total: " + std::to_string(peer.messages_received) + ")");
}

CanValidationResult CanValidator::validate_frame(const std::vector<uint8_t>& payload,
                                                 size_t expected_min_size) const {
    // Check minimum size
    if (payload.empty()) {
        return CanValidationResult::INVALID_FRAME;
    }
    
    if (payload.size() < expected_min_size) {
        LOG_WARN("CanValidator", "Frame too short: " + std::to_string(payload.size()) +
                 " < " + std::to_string(expected_min_size));
        return CanValidationResult::INVALID_FRAME;
    }
    
    // Check for obviously invalid data patterns
    if (config_.validate_ranges) {
        // Could add range checks here based on expected data types
    }
    
    return CanValidationResult::OK;
}

bool CanValidator::check_communication_health() {
    auto now = std::chrono::steady_clock::now();
    bool has_critical = false;
    
    // Update message rate
    update_message_rate();
    
    // Check each peer
    for (size_t i = 0; i < peers_.size(); i++) {
        auto& peer = peers_[i];
        if (!peer.connected) continue;
        
        // Skip BMS and supervisor in normal operation
        if (i == 0 || i == 5) continue;
        
        // Check motor timeouts
        uint32_t time_since_rx = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - peer.last_rx).count();
        
        uint32_t timeout = config_.motor_response_timeout_ms;
        if (i == 0) timeout = config_.bms_response_timeout_ms;
        
        if (time_since_rx > timeout && peer.responding) {
            handle_timeout(peer, static_cast<uint8_t>(i));
            if (peer.consecutive_failures >= config_.consecutive_timeouts_to_error) {
                has_critical = true;
            }
        }
        
        // Check rate limits
        if (peer.msgs_last_second < config_.min_msg_rate_hz) {
            LOG_WARN("CanValidator", "Peer " + std::to_string(i) + 
                     " low message rate: " + std::to_string(peer.msgs_last_second) + " msg/s");
        }
    }
    
    // Check global bus health
    if (total_errors_.load() > config_.max_consecutive_errors) {
        bus_healthy_ = false;
        has_critical = true;
        LOG_ERROR("CanValidator", "Bus unhealthy, too many errors");
    }
    
    return has_critical;
}

void CanValidator::handle_timeout(CanPeerStatus& peer, uint8_t motor_id) {
    peer.timeouts++;
    peer.responding = false;
    peer.consecutive_failures++;
    total_errors_++;
    
    LOG_WARN("CanValidator", "Timeout for peer " + std::to_string(motor_id) + 
             " (consecutive: " + std::to_string(peer.consecutive_failures) + ")");
    
    if (timeout_callback_) {
        timeout_callback_(motor_id);
    }
    
    if (peer.consecutive_failures >= config_.consecutive_timeouts_to_error) {
        peer.degraded_mode = true;
        handle_error(peer, CanValidationResult::TIMEOUT);
    }
}

void CanValidator::handle_error(CanPeerStatus& peer, CanValidationResult result) {
    peer.errors++;
    peer.last_error = std::chrono::steady_clock::now();
    total_errors_++;
    
    LOG_ERROR("CanValidator", "Error: " + std::string(can_validation_result_str(result)));
    
    if (error_callback_) {
        error_callback_(result, can_validation_result_str(result));
    }
    
    if (peer.consecutive_failures >= config_.max_consecutive_errors) {
        peer.connected = false;
        
        if (bus_off_callback_) {
            bus_off_callback_();
        }
    }
}

void CanValidator::update_message_rate() {
    auto now = std::chrono::steady_clock::now();
    uint16_t total_msgs = 0;
    
    for (const auto& peer : peers_) {
        total_msgs += peer.msgs_last_second;
        
        // Reset counter if 1 second passed
        if (std::chrono::duration_cast<std::chrono::seconds>(now - peer.rate_check_time).count() >= 1) {
            peer.rate_check_time = now;
            peer.msgs_last_second = 0;
        }
    }
    
    messages_per_second_ = total_msgs;
}

void CanValidator::reset_peer(uint8_t motor_id) {
    auto& peer = get_peer_ref(motor_id);
    peer = CanPeerStatus{};
    peer.rate_check_time = std::chrono::steady_clock::now();
    LOG_INFO("CanValidator", "Peer " + std::to_string(motor_id) + " reset");
}

void CanValidator::reset_all() {
    for (auto& peer : peers_) {
        peer = CanPeerStatus{};
    }
    
    auto now = std::chrono::steady_clock::now();
    for (auto& peer : peers_) {
        peer.rate_check_time = now;
    }
    
    bus_healthy_ = true;
    total_errors_ = 0;
    messages_per_second_ = 0;
    
    LOG_INFO("CanValidator", "All peers reset");
}

std::string CanValidator::dump_status() const {
    std::ostringstream out;
    out << "=== CAN Validator Status ===" << std::endl;
    out << "Bus healthy: " << (bus_healthy_.load() ? "YES" : "NO") << std::endl;
    out << "Total errors: " << total_errors_.load() << std::endl;
    out << "Messages/sec: " << messages_per_second_.load() << std::endl;
    out << std::endl;
    
    const char* names[] = {"BMS", "M1", "M2", "M3", "M4", "SUP"};
    for (size_t i = 0; i < peers_.size(); i++) {
        const auto& peer = peers_[i];
        out << names[i] << ": "
            << "rx=" << peer.messages_received
            << " tx=" << peer.messages_sent
            << " to=" << peer.timeouts
            << " err=" << peer.errors
            << " ok=" << (peer.responding ? "yes" : "no")
            << " deg=" << (peer.degraded_mode ? "yes" : "no")
            << std::endl;
    }
    
    return out.str();
}

} // namespace common
