#include "state_machine.hpp"
#include <iostream>
#include "../adquisicion_datos/pexda16.hpp"

namespace logica_sistema {
void RunSupervisor(StateMachine &fsm);
}

int main()
{
    logica_sistema::StateMachine fsm;
    fsm.start();
    std::cout << "ECU ATC8110 skeleton running" << std::endl;
    logica_sistema::RunSupervisor(fsm);
    return 0;
}
