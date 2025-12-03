#pragma once

#include "types.hpp"

#include <optional>
#include <string>
#include <vector>

namespace common {

class ILifecycle {
public:
    virtual ~ILifecycle() = default;
    virtual bool start() = 0;
    virtual void stop() = 0;
};

class IDataLogger {
public:
    virtual ~IDataLogger() = default;
    virtual void log_snapshot(const SystemSnapshot &snapshot) = 0;
};

class ICanDriver : public ILifecycle {
public:
    virtual bool send(const CanFrame &frame) = 0;
    virtual std::optional<CanFrame> receive() = 0;
};

class ISensorReader : public ILifecycle {
public:
    virtual std::vector<AnalogSample> read_samples() = 0;
};

class IActuatorWriter : public ILifecycle {
public:
    virtual void write_output(const std::string &channel, double value) = 0;
};

class IController : public ILifecycle {
public:
    virtual void update(SystemSnapshot &snapshot) = 0;
};

} // namespace common
