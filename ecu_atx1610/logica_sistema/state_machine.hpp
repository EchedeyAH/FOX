#pragma once

namespace logica_sistema {

class StateMachine {
public:
    void start();
    void step();
};

inline void StateMachine::start()
{
    // TODO: bootstrap system state.
}

inline void StateMachine::step()
{
    // TODO: advance the ECU state machine.
}

} // namespace logica_sistema
