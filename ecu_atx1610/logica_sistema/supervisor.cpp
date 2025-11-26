#include "state_machine.hpp"
#include "../common/logging.hpp"

#include <thread>

namespace logica_sistema {

void RunSupervisor(StateMachine &fsm)
{
    for (int i = 0; i < 5; ++i) {
        LOG_INFO("Supervisor", "Iteración " + std::to_string(i));
        fsm.step();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

} // namespace logica_sistema
