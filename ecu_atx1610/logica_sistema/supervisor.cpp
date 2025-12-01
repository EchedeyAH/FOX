#include "state_machine.hpp"
#include "../common/logging.hpp"

#include <thread>

namespace logica_sistema {

void RunSupervisor(StateMachine &fsm)
{
    while (true) {
        // LOG_INFO("Supervisor", "Iteración..."); // Reducir log spam
        fsm.step();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

} // namespace logica_sistema
