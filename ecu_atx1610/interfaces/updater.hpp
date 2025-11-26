#pragma once

#include "../common/logging.hpp"

namespace interfaces {

class Updater {
public:
    void check_for_updates();
};

} // namespace interfaces
