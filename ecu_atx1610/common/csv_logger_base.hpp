#pragma once

#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <vector>
#include <chrono>

namespace common {

/**
 * Clase base abstracta para todos los loggers CSV
 * Proporciona funcionalidad común de buffering, rotación y escritura
 */
class CsvLoggerBase {
public:
    struct Config {
        std::filesystem::path session_directory;
        std::string subsystem_name;  // "bms", "motors", etc.
        size_t buffer_size{1000};    // Número de líneas antes de flush
        size_t max_file_size_mb{100}; // Tamaño máximo antes de rotar
        int rotation_interval_minutes{60}; // Tiempo antes de rotar
    };
    
    explicit CsvLoggerBase(const Config& config);
    virtual ~CsvLoggerBase();
    
    // Deshabilitar copia
    CsvLoggerBase(const CsvLoggerBase&) = delete;
    CsvLoggerBase& operator=(const CsvLoggerBase&) = delete;
    
    // Inicia el logging (abre archivo y escribe encabezado)
    bool start();
    
    // Detiene el logging (flush y cierra archivo)
    void stop();
    
    // Verifica si está activo
    bool is_active() const { return is_active_; }

protected:
    // Método virtual puro: debe devolver el encabezado CSV
    virtual std::string get_csv_header() const = 0;
    
    // Escribe una línea al buffer
    void write_line(const std::string& line);
    
    // Obtiene timestamp actual en formato ISO 8601
    std::string get_timestamp() const;
    
    // Hace flush del buffer al archivo
    void flush();

private:
    Config config_;
    std::ofstream file_;
    std::vector<std::string> buffer_;
    std::mutex mutex_;
    bool is_active_{false};
    std::chrono::system_clock::time_point file_creation_time_;
    std::filesystem::path current_file_path_;
    
    // Abre un nuevo archivo CSV
    bool open_new_file();
    
    // Verifica si necesita rotar el archivo
    bool should_rotate() const;
    
    // Genera nombre de archivo con timestamp
    std::string generate_filename() const;
};

} // namespace common
