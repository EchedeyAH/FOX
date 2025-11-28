# Implementación de Comunicación CAN para ECU ATX-1610

## Objetivo

Implementar la capa de comunicación CAN completa para la ECU ATX-1610, migrando del sistema legacy basado en QNX a una arquitectura moderna usando SocketCAN en Linux. El sistema debe soportar comunicación con BMS, motores y supervisor, manteniendo la robustez del diseño original.

## Análisis del Sistema Legacy

El código original (`ECU_FOX_rc30-operativo`) implementa:

- **Proceso CAN2**: Comunicación dedicada con BMS (500 Kbps) usando colas POSIX
- **Hilos CAN Motores**: 4 hilos independientes para comunicación con controladores de motores (1 Mbps)
- **Protocolo BMS**: Mensajes estructurados con índice + parámetro + valor para voltaje, temperatura, estado y alarmas
- **Protocolo Motores**: 13 tipos de mensajes CCP (CAN Calibration Protocol) para telemetría y configuración
- **Supervisor**: Heartbeat y comandos de control del sistema

## Cambios Propuestos

### Componente: Comunicación CAN

#### [MODIFY] [socketcan_interface.cpp](file:///c:/Users/ahech/Desktop/FOX/ecu_atx1610/comunicacion_can/socketcan_interface.cpp)

**Cambios**:
- Reemplazar implementación simulada con driver SocketCAN real
- Añadir apertura de socket CAN usando `socket(PF_CAN, SOCK_RAW, CAN_RAW)`
- Implementar binding a interfaz de red CAN (`can0`, `can1`)
- Añadir configuración de filtros CAN para IDs específicos
- Implementar envío real con `write()` y recepción con `read()`
- Añadir manejo de errores de bus CAN
- Implementar modo no-bloqueante para recepción

#### [MODIFY] [socketcan_interface.hpp](file:///c:/Users/ahech/Desktop/FOX/ecu_atx1610/comunicacion_can/socketcan_interface.hpp)

**Cambios**:
- Añadir miembro `int socket_fd_` para el file descriptor del socket
- Añadir método `set_filter()` para configurar filtros CAN
- Añadir método `get_error_stats()` para estadísticas de errores
- Eliminar `std::queue` de loopback (ya no es simulación)
- Añadir includes de Linux: `<linux/can.h>`, `<linux/can/raw.h>`, `<sys/socket.h>`, `<net/if.h>`

#### [MODIFY] [can_manager.hpp](file:///c:/Users/ahech/Desktop/FOX/ecu_atx1610/comunicacion_can/can_manager.hpp)

**Cambios**:
- Expandir protocolo de mensajes basado en el código legacy
- Añadir métodos para mensajes de motores: `publish_motor_command()`, `request_motor_telemetry()`
- Añadir métodos para supervisor: `publish_supervisor_heartbeat()`, `process_supervisor_commands()`
- Mejorar `publish_battery()` con protocolo completo BMS (voltajes por celda, temperaturas, alarmas)
- Añadir decodificación de mensajes CAN recibidos según protocolo legacy
- Implementar tabla de IDs CAN para diferentes componentes

#### [NEW] [can_protocol.hpp](file:///c:/Users/ahech/Desktop/FOX/ecu_atx1610/comunicacion_can/can_protocol.hpp)

**Propósito**: Definir el protocolo CAN completo del sistema

**Contenido**:
- Definición de IDs CAN para todos los componentes (BMS, 4 motores, supervisor)
- Enumeraciones para tipos de mensajes (basado en legacy: MSG_TIPO_01 a MSG_TIPO_13)
- Constantes de protocolo BMS (VOLTAJE_T, TEMPERATURA_T, ESTADO_T, ALARMA_T)
- Estructuras para codificación/decodificación de mensajes
- Funciones helper para construir y parsear mensajes CAN

#### [NEW] [can_bms_handler.hpp](file:///c:/Users/ahech/Desktop/FOX/ecu_atx1610/comunicacion_can/can_bms_handler.hpp)

**Propósito**: Handler especializado para comunicación con BMS

**Contenido**:
- Clase `BmsCanHandler` que encapsula lógica de BMS
- Métodos para decodificar mensajes de voltaje, temperatura, estado y alarmas
- Gestión de timeout y detección de pérdida de comunicación
- Actualización de estructura `BatteryState` con datos completos (24 celdas)

#### [NEW] [can_bms_handler.cpp](file:///c:/Users/ahech/Desktop/FOX/ecu_atx1610/comunicacion_can/can_bms_handler.cpp)

Implementación del handler BMS basado en `can2_fox.c` del código legacy.

---

### Componente: Tipos Comunes

#### [MODIFY] [types.hpp](file:///c:/Users/ahech/Desktop/FOX/ecu_atx1610/common/types.hpp)

**Cambios**:
- Expandir `BatteryState` con campos adicionales del legacy:
  - `std::array<uint16_t, 24> cell_voltages_mv` - Voltajes individuales por celda
  - `std::array<uint8_t, 24> cell_temperatures_c` - Temperaturas por celda
  - `uint8_t num_cells_detected` - Número de celdas detectadas
  - `uint8_t temp_avg_c`, `temp_max_c` - Temperaturas promedio y máxima
  - `uint16_t voltage_avg_mv`, `voltage_max_mv`, `voltage_min_mv` - Estadísticas de voltaje
  - `uint8_t cell_temp_max_id`, `cell_v_max_id`, `cell_v_min_id` - IDs de celdas con valores extremos
  - Enumeración para niveles de alarma (NO_ALARMA, ALARMA, WARNING, ALARMA_CRITICA)
  - Enumeración para tipos de alarma (CELL_TEMP_HIGH, PACK_V_HIGH, PACK_I_HIGH, etc.)

---

### Componente: Scripts de Configuración

#### [MODIFY] [setup_can.sh](file:///c:/Users/ahech/Desktop/FOX/ecu_atx1610/scripts/setup_can.sh)

**Cambios**:
- Añadir configuración de dos interfaces CAN (`can0` a 1 Mbps, `can1` a 500 Kbps)
- Configurar bitrates apropiados
- Activar interfaces con `ip link set up`
- Añadir verificación de módulos kernel (`can`, `can_raw`, `vcan`)
- Añadir modo de prueba con interfaz virtual `vcan0` para testing sin hardware

---

### Componente: Build System

#### [MODIFY] [CMakeLists.txt](file:///c:/Users/ahech/Desktop/FOX/ecu_atx1610/CMakeLists.txt)

**Cambios**:
- No se requieren cambios significativos
- Los archivos `.cpp` nuevos se detectarán automáticamente por el glob pattern existente

## Plan de Verificación

### Pruebas Automáticas

#### 1. Test de Interfaz SocketCAN con Virtual CAN

**Comando**:
```bash
cd /c/Users/ahech/Desktop/FOX/ecu_atx1610
# Configurar interfaz virtual CAN (requiere permisos)
sudo modprobe vcan
sudo ip link add dev vcan0 type vcan
sudo ip link set up vcan0

# Compilar proyecto
mkdir -p build && cd build
cmake ..
make

# Ejecutar con interfaz virtual
./ecu_atx1610
```

**Validación**:
- El sistema debe iniciar sin errores
- Los logs deben mostrar "SocketCAN Interfaz vcan0 inicializada"
- Usar `candump vcan0` en otra terminal para ver mensajes transmitidos

#### 2. Test de Loopback CAN

**Comando**:
```bash
# En terminal 1: Ejecutar la ECU
./ecu_atx1610

# En terminal 2: Enviar mensaje CAN simulado de BMS
cansend vcan0 180#AA55010203
```

**Validación**:
- La ECU debe recibir y procesar el mensaje
- Los logs deben mostrar el ID y payload recibido

### Pruebas Manuales

> [!IMPORTANT]
> Las siguientes pruebas requieren hardware CAN real (tarjeta EMUC-B2S3 o similar)

#### 1. Verificación con Hardware Real

**Pasos**:
1. Conectar tarjeta CAN al sistema
2. Verificar que aparecen interfaces `can0` y `can1`: `ip link show`
3. Ejecutar script de configuración: `sudo ./scripts/setup_can.sh`
4. Conectar BMS al bus CAN2 (can1)
5. Ejecutar ECU: `./ecu_atx1610`
6. Verificar en logs que se reciben mensajes del BMS con ID correcta

**Resultado esperado**:
- Comunicación exitosa con BMS
- Datos de batería actualizándose en tiempo real
- Sin errores de timeout

#### 2. Prueba de Protocolo BMS Completo

**Pasos**:
1. Con BMS conectado, verificar recepción de todos los tipos de mensaje:
   - Voltajes de celdas (VOLTAJE_T)
   - Temperaturas (TEMPERATURA_T)
   - Estado del pack (ESTADO_T)
   - Alarmas (ALARMA_T)
2. Provocar condición de alarma (ej: voltaje bajo) y verificar detección
3. Desconectar BMS y verificar timeout y flag de error de comunicación

**Resultado esperado**:
- Todos los campos de `BatteryState` se actualizan correctamente
- Alarmas se detectan y clasifican por nivel
- Timeout se detecta en ~3 segundos

### Verificación de Usuario

Dado que este sistema requiere hardware especializado (tarjeta EMUC-B2S3, BMS, controladores de motores), solicito al usuario:

1. **¿Tienes acceso al hardware CAN para pruebas?** Si no, me enfocaré en hacer el código robusto con pruebas de interfaz virtual.

2. **¿Qué componentes están disponibles actualmente?**
   - BMS (Battery Management System)
   - Controladores de motores (¿cuántos?)
   - Supervisor
   - Tarjeta CAN EMUC-B2S3

3. **¿Prefieres que implemente primero solo BMS, o todo el protocolo completo?**

## Notas Técnicas

### Diferencias QNX vs Linux

| Aspecto | QNX (Legacy) | Linux (Nuevo) |
|---------|--------------|---------------|
| Driver CAN | Propietario (`can_dual`) | SocketCAN (kernel) |
| API | `ConnectDriver()`, `CanWrite()` | `socket()`, `write()`, `read()` |
| Procesos | Múltiples procesos + colas | Multi-hilo (ya implementado) |
| Sincronización | POSIX (compatible) | POSIX (sin cambios) |

### IDs CAN Propuestos (basado en legacy)

```cpp
// BMS
#define ID_CAN_BMS              0x180

// Motores (ECU → Motor)
#define ID_MOTOR_1_CMD          0x201
#define ID_MOTOR_2_CMD          0x202
#define ID_MOTOR_3_CMD          0x203
#define ID_MOTOR_4_CMD          0x204

// Motores (Motor → ECU)
#define ID_MOTOR_1_RESP         0x281
#define ID_MOTOR_2_RESP         0x282
#define ID_MOTOR_3_RESP         0x283
#define ID_MOTOR_4_RESP         0x284

// Supervisor
#define ID_SUPERVISOR_HB        0x100  // Heartbeat
#define ID_SUPERVISOR_CMD       0x101  // Comandos
```
