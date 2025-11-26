# Arquitectura de referencia

El código C++ reproduce la separación de responsabilidades descrita para la ECU FOX:

- **Procesamiento principal**: `logica_sistema/StateMachine` coordina sensores, controladores y comunicación CAN.
- **Adquisición**: `adquisicion_datos/SensorManager` envuelve una fuente de datos analógicos (PEX-1202L simulada) y entrega muestras normalizadas.
- **Control**: controladores desacoplados (`control_vehiculo/`) actúan sobre `SystemSnapshot` para mantener suspensión, batería y tracción.
- **CAN**: `comunicacion_can/CanManager` encapsula el driver `SocketCanInterface`, publica heartbeats y estado de batería, y procesa tramas entrantes.
- **Interfaces**: módulos en `interfaces/` sirven como puntos de integración para CLI, logging web o actualizaciones OTA.

El diseño prioriza:
1. **Portabilidad**: no se utilizan dependencias de QNX; todo es C++17 estándar.
2. **Modularidad**: cada subsistema expone fábricas o interfaces claras para intercambiar hardware real por simulación.
3. **Trazabilidad**: `SystemSnapshot` concentra el estado global para ser registrado o visualizado.
