#include "csv_logger_base.hpp"
#include <iomanip>
#include <sstream>
#include <iostream>

namespace common {

CsvLoggerBase::CsvLoggerBase(const Config& config)
    : config_(config) {
}

CsvLoggerBase::~CsvLoggerBase() {
    stop();
}

bool CsvLoggerBase::start() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (is_active_) {
        return true;
    }
    
    if (!open_new_file()) {
        return false;
    }
    
    is_active_ = true;
    return true;
}

void CsvLoggerBase::stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!is_active_) {
        return;
    }
    
    flush();
    
    if (file_.is_open()) {
        file_.close();
    }
    
    is_active_ = false;
}

void CsvLoggerBase::write_line(const std::string& line) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!is_active_) {
        return;
    }
    
    buffer_.push_back(line);
    
    // Flush si el buffer está lleno
    if (buffer_.size() >= config_.buffer_size) {
        flush();
    }
    
    // Rotar archivo si es necesario
    if (should_rotate()) {
        flush();
        file_.close();
        open_new_file();
    }
}

std::string CsvLoggerBase::get_timestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::microseconds>(
        now.time_since_epoch()) % 1000000;
    
    std::tm tm = *std::localtime(&time_t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(6) << ms.count();
    
    return oss.str();
}

void CsvLoggerBase::flush() {
    if (buffer_.empty() || !file_.is_open()) {
        return;
    }
    
    for (const auto& line : buffer_) {
        file_ << line << '\n';
    }
    
    file_.flush();
    buffer_.clear();
}

bool CsvLoggerBase::open_new_file() {
    // Crear directorio del subsistema si no existe
    fs::path subsystem_dir = config_.session_directory / config_.subsystem_name;
    
    try {
        fs::create_directories(subsystem_dir);
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error al crear directorio " << subsystem_dir << ": " << e.what() << "\n";
        return false;
    }
    
    // Generar nombre de archivo
    std::string filename = generate_filename();
    current_file_path_ = subsystem_dir / filename;
    
    // Abrir archivo
    file_.open(current_file_path_, std::ios::out | std::ios::app);
    
    if (!file_.is_open()) {
        std::cerr << "Error al abrir archivo " << current_file_path_ << "\n";
        return false;
    }
    
    // Escribir encabezado si el archivo está vacío
    file_.seekp(0, std::ios::end);
    if (file_.tellp() == 0) {
        file_ << get_csv_header() << '\n';
    }
    
    file_creation_time_ = std::chrono::system_clock::now();
    
    std::cout << "Logger CSV iniciado: " << current_file_path_ << "\n";
    
    return true;
}

bool CsvLoggerBase::should_rotate() const {
    if (!file_.is_open()) {
        return false;
    }
    
    // Verificar tamaño del archivo
    try {
        auto file_size = fs::file_size(current_file_path_);
        if (file_size >= config_.max_file_size_mb * 1024 * 1024) {
            return true;
        }
    } catch (const fs::filesystem_error&) {
        // Ignorar errores al obtener tamaño
    }
    
    // Verificar tiempo desde creación
    auto now = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::minutes>(
        now - file_creation_time_);
    
    if (duration.count() >= config_.rotation_interval_minutes) {
        return true;
    }
    
    return false;
}

std::string CsvLoggerBase::generate_filename() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&time_t);
    
    std::ostringstream oss;
    oss << config_.subsystem_name << "_";
    oss << std::put_time(&tm, "%Y-%m-%d_%H-%M-%S");
    oss << ".csv";
    
    return oss.str();
}

} // namespace common
