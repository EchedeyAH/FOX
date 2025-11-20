#include "state_machine.hpp"
#include <iostream>

namespace logica_sistema {
void RunSupervisor(StateMachine &fsm);
}

int main()
{
    logica_sistema::StateMachine fsm;
    fsm.start();
    std::cout << "ECU ATX1610 skeleton running" << std::endl;
    logica_sistema::RunSupervisor(fsm);
    return 0;
}
