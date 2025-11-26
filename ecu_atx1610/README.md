# ECU ATX-1610 (vCAN Simulation)

Este módulo implementa un esqueleto funcional inspirado en la arquitectura del proyecto original **ECU_FOX_rc30-operativo**. El objetivo es proporcionar un punto de partida en C++17 para probar la lógica de coordinación, adquisición y control sin depender de hardware real.

## Componentes
- **common/**: Tipos de datos compartidos, interfaces y utilidades de logging.
- **comunicacion_can/**: Driver SocketCAN simulado y gestor de tramas de alto nivel.
- **adquisicion_datos/**: Simulación de la tarjeta PEX-1202L y gestor de sensores.
- **control_vehiculo/**: Controladores modulares para suspensión, batería y tracción.
- **logica_sistema/**: Máquina de estados principal y supervisor de iteraciones.
- **interfaces/**: Stub para CLI, logging web y actualizaciones OTA.
- **config/**: Ejemplo de configuración en YAML.

## Flujo de ejecución
1. `logica_sistema::StateMachine` arranca los dispositivos simulados y controladores.
2. En cada ciclo se refrescan sensores, se actualizan los controladores y se publican frames CAN (heartbeat y estado de batería) mediante el `CanManager`.
3. Los estados y advertencias se almacenan en `SystemSnapshot` para su posterior registro o publicación.

## Compilación rápida
```
mkdir -p build && cd build
cmake .. && cmake --build .
./ecu_atx1610
```

Al ejecutarse se imprimen iteraciones del supervisor y mensajes de logging con el flujo de datos simulado.
