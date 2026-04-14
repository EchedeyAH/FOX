#include "logger.hpp"

#include <chrono>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <cctype>
#include <ctime>
#include <filesystem>
#include <fcntl.h>
#include <poll.h>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace {

std::string TimestampForLine()
{
    using namespace std::chrono;
    const auto now = system_clock::now();
    const auto t = system_clock::to_time_t(now);
    std::tm tm{};
    localtime_r(&t, &tm);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%F %T", &tm);
    return std::string(buf);
}

std::string TimestampForFile()
{
    using namespace std::chrono;
    const auto now = system_clock::now();
    const auto t = system_clock::to_time_t(now);
    std::tm tm{};
    localtime_r(&t, &tm);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%F_%H-%M-%S", &tm);
    return std::string(buf);
}

int NextTestNumber(const std::filesystem::path& dir)
{
    std::error_code ec;
    if (!std::filesystem::exists(dir, ec) || ec) {
        return 1;
    }

    int max_num = 0;
    for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
        if (ec) {
            break;
        }
        if (!entry.is_regular_file()) {
            continue;
        }
        const auto name = entry.path().filename().string();
        const std::string prefix = "Prueba_";
        if (name.rfind(prefix, 0) != 0) {
            continue;
        }
        const size_t num_start = prefix.size();
        const size_t num_end = name.find('_', num_start);
        if (num_end == std::string::npos || num_end <= num_start) {
            continue;
        }
        const std::string num_str = name.substr(num_start, num_end - num_start);
        char* end_ptr = nullptr;
        const long num = std::strtol(num_str.c_str(), &end_ptr, 10);
        if (end_ptr == num_str.c_str() || *end_ptr != '\0' || num <= 0) {
            continue;
        }
        if (num > max_num) {
            max_num = static_cast<int>(num);
        }
    }
    return max_num + 1;
}

void WriteAll(int fd, const char* data, size_t len)
{
    size_t written = 0;
    while (written < len) {
        const ssize_t n = ::write(fd, data + written, len - written);
        if (n > 0) {
            written += static_cast<size_t>(n);
            continue;
        }
        if (n < 0 && errno == EINTR) {
            continue;
        }
        break;
    }
}

bool HasTimestampPrefix(const std::string& line)
{
    if (line.size() < 21) {
        return false;
    }
    return line[0] == '[' &&
           std::isdigit(static_cast<unsigned char>(line[1])) &&
           std::isdigit(static_cast<unsigned char>(line[2])) &&
           std::isdigit(static_cast<unsigned char>(line[3])) &&
           std::isdigit(static_cast<unsigned char>(line[4])) &&
           line[5] == '-' &&
           std::isdigit(static_cast<unsigned char>(line[6])) &&
           std::isdigit(static_cast<unsigned char>(line[7])) &&
           line[8] == '-' &&
           std::isdigit(static_cast<unsigned char>(line[9])) &&
           std::isdigit(static_cast<unsigned char>(line[10])) &&
           line[11] == ' ' &&
           std::isdigit(static_cast<unsigned char>(line[12])) &&
           std::isdigit(static_cast<unsigned char>(line[13])) &&
           line[14] == ':' &&
           std::isdigit(static_cast<unsigned char>(line[15])) &&
           std::isdigit(static_cast<unsigned char>(line[16])) &&
           line[17] == ':' &&
           std::isdigit(static_cast<unsigned char>(line[18])) &&
           std::isdigit(static_cast<unsigned char>(line[19])) &&
           line[20] == ']';
}

} // namespace

namespace common {

Logger& Logger::Instance()
{
    static Logger instance;
    return instance;
}

Logger::~Logger()
{
    Shutdown();
}

bool Logger::Init(const Config& cfg)
{
    bool expected = false;
    if (!initialized_.compare_exchange_strong(expected, true)) {
        return true;
    }

    const std::filesystem::path dir(cfg.base_dir);
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    if (ec) {
        initialized_.store(false);
        return false;
    }

    const int test_number = NextTestNumber(dir);
    const std::string ts = TimestampForFile();
    const std::string filename = "Prueba_" + std::to_string(test_number) + "_" + ts + ".log";
    log_path_ = (dir / filename).string();

    log_fd_ = ::open(log_path_.c_str(), O_CREAT | O_APPEND | O_WRONLY, 0644);
    if (log_fd_ < 0) {
        initialized_.store(false);
        return false;
    }

    console_out_fd_ = ::dup(STDOUT_FILENO);
    console_err_fd_ = ::dup(STDERR_FILENO);
    if (console_out_fd_ < 0 || console_err_fd_ < 0) {
        Shutdown();
        return false;
    }

    // Redirección obligatoria de stdout/stderr al fichero.
    if (!std::freopen(log_path_.c_str(), "a", stdout) ||
        !std::freopen(log_path_.c_str(), "a", stderr)) {
        ::dup2(console_out_fd_, STDOUT_FILENO);
        ::dup2(console_err_fd_, STDERR_FILENO);
        Shutdown();
        return false;
    }
    std::fflush(stdout);
    std::fflush(stderr);

    int pipe_fds[2];
    if (::pipe(pipe_fds) != 0) {
        ::dup2(console_out_fd_, STDOUT_FILENO);
        ::dup2(console_err_fd_, STDERR_FILENO);
        Shutdown();
        return false;
    }

    // Aumentar el buffer del pipe si el kernel lo permite (reduce riesgo de bloqueo).
    #ifdef F_SETPIPE_SZ
    (void)::fcntl(pipe_fds[0], F_SETPIPE_SZ, 1 << 20);
    #endif

    // Redirigir stdout/stderr al pipe para hacer tee a consola + fichero.
    ::dup2(pipe_fds[1], STDOUT_FILENO);
    ::dup2(pipe_fds[1], STDERR_FILENO);
    ::close(pipe_fds[1]);

    pipe_read_fd_ = pipe_fds[0];
    const int flags = ::fcntl(pipe_read_fd_, F_GETFL, 0);
    if (flags >= 0) {
        ::fcntl(pipe_read_fd_, F_SETFL, flags | O_NONBLOCK);
    }

    std::setvbuf(stdout, nullptr, _IONBF, 0);
    std::setvbuf(stderr, nullptr, _IONBF, 0);

    stop_.store(false);
    reader_thread_ = std::thread(&Logger::ReaderLoop, this);
    return true;
}

void Logger::Shutdown()
{
    if (!initialized_.load()) {
        return;
    }
    stop_.store(true);
    if (reader_thread_.joinable()) {
        reader_thread_.join();
    }
    if (pipe_read_fd_ >= 0) {
        ::close(pipe_read_fd_);
        pipe_read_fd_ = -1;
    }
    if (log_fd_ >= 0) {
        ::close(log_fd_);
        log_fd_ = -1;
    }
    if (console_out_fd_ >= 0) {
        ::close(console_out_fd_);
        console_out_fd_ = -1;
    }
    if (console_err_fd_ >= 0) {
        ::close(console_err_fd_);
        console_err_fd_ = -1;
    }
    initialized_.store(false);
}

void Logger::ReaderLoop()
{
    std::string buffer;
    buffer.reserve(4096);
    char temp[4096];

    while (!stop_.load()) {
        pollfd pfd{};
        pfd.fd = pipe_read_fd_;
        pfd.events = POLLIN;
        const int pr = ::poll(&pfd, 1, 200);
        if (pr <= 0) {
            continue;
        }
        if ((pfd.revents & POLLIN) == 0) {
            continue;
        }

        const ssize_t n = ::read(pipe_read_fd_, temp, sizeof(temp));
        if (n <= 0) {
            continue;
        }
        buffer.append(temp, static_cast<size_t>(n));

        size_t pos = 0;
        while ((pos = buffer.find('\n')) != std::string::npos) {
            std::string line = buffer.substr(0, pos);
            buffer.erase(0, pos + 1);
            WriteLine(line);
        }
    }

    if (!buffer.empty()) {
        WriteLine(buffer);
    }
}

void Logger::WriteLine(const std::string& line)
{
    const std::string output = HasTimestampPrefix(line)
        ? (line + "\n")
        : ("[" + TimestampForLine() + "] " + line + "\n");

    std::lock_guard<std::mutex> lock(write_mutex_);
    if (log_fd_ >= 0) {
        WriteAll(log_fd_, output.c_str(), output.size());
    }
    if (console_out_fd_ >= 0) {
        WriteAll(console_out_fd_, output.c_str(), output.size());
    } else if (console_err_fd_ >= 0) {
        WriteAll(console_err_fd_, output.c_str(), output.size());
    }
}

} // namespace common
