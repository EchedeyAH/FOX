#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <thread>

namespace common {

class Logger {
public:
    struct Config {
        std::string base_dir;
        Config() : base_dir("datos_pruebas") {}
    };

    static Logger& Instance();

    bool Init();
    bool Init(const Config& cfg);
    void Shutdown();

    const std::string& log_path() const { return log_path_; }

private:
    Logger() = default;
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void ReaderLoop();
    void WriteLine(const std::string& line);

    std::atomic<bool> initialized_{false};
    std::atomic<bool> stop_{false};
    std::thread reader_thread_;
    std::mutex write_mutex_;

    int pipe_read_fd_ = -1;
    int log_fd_ = -1;
    int console_out_fd_ = -1;
    int console_err_fd_ = -1;

    std::string log_path_;
};

} // namespace common
