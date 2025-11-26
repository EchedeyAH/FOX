# ECU ATX-1610 (vCAN Simulation)

Este módulo implementa un esqueleto funcional inspirado en la arquitectura del proyecto original **ECU_FOX_rc30-operativo**. El objetivo es proporcionar un punto de partida en C++17 para probar la lógica de coordinación, adquisición y control sin depender de hardware real.

## Componentes
- **common/**: Tipos de datos compartidos, interfaces y utilidades de logging.
- **comunicacion_can/**: Driver SocketCAN simulado y gestor de tramas de alto nivel.
- **adquisicion_datos/**: Simulación de la tarjeta PEX-1202L y gestor de sensores.
- **control_vehiculo/**: Controladores modulares para suspensión, batería y tracción.
- **logica_sistema/**: Máquina de estados principal y supervisor de iteraciones.
- **interfaces/**: CLI básico, logging web y stub para actualizaciones OTA.
- **config/**: Ejemplo de configuración en YAML.

## Flujo de ejecución
1. `logica_sistema::StateMachine` arranca los dispositivos simulados y controladores.
2. En cada ciclo se refrescan sensores, se actualizan los controladores y se publican frames CAN (heartbeat y estado de batería) mediante el `CanManager`.
3. Los actuadores simulados PEX-DA16 reflejan la demanda de pedal, freno y par por motor.
4. Los estados y advertencias se almacenan en `SystemSnapshot`, se muestran en la CLI y se envían al logger web simulado.

## Requisitos en Ubuntu 18.04
Ubuntu 18.04 incluye CMake 3.10 y `g++-7`, que soportan C++17. Instala las dependencias básicas de compilación y CAN virtual:

```
sudo apt update
sudo apt install -y build-essential cmake libsocketcan-dev
```

Para habilitar pruebas de red CAN virtual ejecuta `scripts/setup_can.sh`, que carga el módulo `vcan` disponible en el kernel estándar de 18.04.

## Compilación rápida
```
mkdir -p build && cd build
cmake .. && cmake --build .
./ecu_atx1610
```

Al ejecutarse se imprimen iteraciones del supervisor y mensajes de logging con el flujo de datos simulado.
