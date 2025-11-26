#pragma once

#include "../common/types.hpp"

namespace interfaces {

class CliInterface {
public:
    void print_snapshot(const common::SystemSnapshot &snapshot) const;
};

} // namespace interfaces
