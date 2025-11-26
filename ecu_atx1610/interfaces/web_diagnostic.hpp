#pragma once

#include "../common/interfaces.hpp"
#include "../common/logging.hpp"
#include "../common/types.hpp"

namespace interfaces {

class WebDiagnostic : public common::IDataLogger {
public:
    void log_snapshot(const common::SystemSnapshot &snapshot);
};

} // namespace interfaces
