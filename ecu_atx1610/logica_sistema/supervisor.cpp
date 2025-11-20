#include "state_machine.hpp"
#include <iostream>

namespace logica_sistema {

void RunSupervisor(StateMachine &fsm)
{
    std::cout << "[logica_sistema] Supervisor tick" << std::endl;
    fsm.step();
}

} // namespace logica_sistema
