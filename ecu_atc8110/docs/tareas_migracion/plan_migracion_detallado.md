# Plan de Migración y Puesta en Marcha - Vehículo Experimental FOX

## 📋 Resumen Ejecutivo

Este documento detalla el plan completo para migrar el sistema de control del vehículo FOX desde la arquitectura legacy basada en QNX hacia la nueva plataforma Linux con ECU ATX-1610. El plan está organizado en 6 fases principales que abarcan desde la preparación inicial hasta la puesta en marcha completa del vehículo.

---

## 🎯 Objetivos Principales

1. **Migrar** el sistema de control desde QNX a Linux (Ubuntu 18.04 LTS)
2. **Implementar** comunicación CAN completa (BMS, motores, supervisor)
3. **Integrar** todos los sensores y actuadores del vehículo
4. **Validar** el funcionamiento del sistema completo
5. **Poner en marcha** el vehículo de forma segura y controlada

---

## 📊 Fase 1: Preparación del Sistema Base

### 1.1 Verificación de Hardware

**Objetivo**: Confirmar que todo el hardware necesario está disponible y funcional.

#### Checklist de Hardware:
- [ ] **ECU ATC-8110** - Unidad de control principal
  - Verificar alimentación eléctrica
  - Confirmar acceso físico
  - Verificar conectores
  
- [ ] **Tarjeta EMUC-B2S3** - Comunicación CAN
  - Verificar instalación en ECU
  - Confirmar 2 puertos CAN disponibles
  - Verificar drivers en sistema
  
- [ ] **Tarjeta PEX-1202L** - Entradas analógicas
  - Verificar instalación
  - Confirmar canales disponibles
  
- [ ] **Tarjeta PEX-DA16** - Salidas analógicas/digitales
  - Verificar instalación
  - Confirmar canales disponibles
  
- [ ] **GPS ublox SM-76G** - Posicionamiento
  - Verificar conexión
  - Confirmar puerto serial
  
- [ ] **BMS** - Sistema de gestión de baterías (24 celdas)
  - Verificar conexión CAN
  - Confirmar protocolo de comunicación
  
- [ ] **Controladores de Motores** (4 unidades)
  - Verificar conexiones CAN
  - Confirmar IDs CAN asignados
  - Verificar protocolo CCP

#### Comandos de Verificación:
```bash
# Verificar tarjetas PCI
lspci | grep -i can
lspci | grep -i analog

# Verificar puertos seriales
ls -l /dev/ttyUSB* /dev/ttyS*

# Verificar módulos del kernel
lsmod | grep can
```

### 1.2 Configuración del Sistema Operativo

**Objetivo**: Preparar el entorno Linux en la ECU.

#### Tareas:
- [ ] **Verificar versión de Ubuntu**
  ```bash
  lsb_release -a
  # Debe ser: Ubuntu 18.04 LTS
  ```

- [ ] **Actualizar sistema**
  ```bash
  sudo apt update
  sudo apt upgrade -y
  ```

- [ ] **Instalar dependencias**
  ```bash
  sudo apt install -y \
    build-essential \
    cmake \
    git \
    can-utils \
    libsocketcan-dev \
    linux-modules-extra-$(uname -r)
  ```

- [ ] **Cargar módulos CAN**
  ```bash
  sudo modprobe can
  sudo modprobe can_raw
  sudo modprobe vcan
  
  # Hacer permanente
  echo "can" | sudo tee -a /etc/modules
  echo "can_raw" | sudo tee -a /etc/modules
  ```

### 1.3 Configuración de Acceso Remoto

**Objetivo**: Establecer acceso SSH seguro para desarrollo y despliegue.

#### Tareas:
- [ ] **Configurar SSH**
  ```bash
  # Desde máquina de desarrollo
  ssh-keygen -t rsa -b 4096
  ssh-copy-id fox@193.147.165.236
  ```

- [ ] **Verificar conectividad**
  ```bash
  ssh fox@193.147.165.236 "uname -a"
  ```

- [ ] **Configurar GitHub Actions runner** (si aplica)
  - Instalar runner en Windows
  - Configurar credenciales SSH
  - Probar workflow de despliegue

---

## 🔌 Fase 2: Implementación de Comunicación CAN

### 2.1 Implementación del Driver SocketCAN

**Objetivo**: Reemplazar la implementación simulada con driver SocketCAN real.

#### Archivos a Modificar:

##### `comunicacion_can/socketcan_interface.cpp`
- [ ] Implementar apertura de socket CAN
  ```cpp
  socket_fd_ = socket(PF_CAN, SOCK_RAW, CAN_RAW);
  ```
  
- [ ] Implementar binding a interfaz
  ```cpp
  struct sockaddr_can addr;
  addr.can_family = AF_CAN;
  addr.can_ifindex = if_nametoindex("can0");
  bind(socket_fd_, (struct sockaddr *)&addr, sizeof(addr));
  ```
  
- [ ] Implementar envío de mensajes
  ```cpp
  write(socket_fd_, &frame, sizeof(struct can_frame));
  ```
  
- [ ] Implementar recepción no-bloqueante
  ```cpp
  fcntl(socket_fd_, F_SETFL, O_NONBLOCK);
  read(socket_fd_, &frame, sizeof(struct can_frame));
  ```
  
- [ ] Añadir manejo de errores de bus

##### `comunicacion_can/socketcan_interface.hpp`
- [ ] Añadir miembro `int socket_fd_`
- [ ] Añadir método `set_filter()`
- [ ] Añadir método `get_error_stats()`
- [ ] Incluir headers de Linux CAN

### 2.2 Definición del Protocolo CAN

**Objetivo**: Crear definiciones completas del protocolo CAN del vehículo.

#### Archivo Nuevo: `comunicacion_can/can_protocol.hpp`

- [ ] **Definir IDs CAN**
  ```cpp
  // BMS
  constexpr uint32_t ID_CAN_BMS = 0x180;
  
  // Motores (ECU → Motor)
  constexpr uint32_t ID_MOTOR_1_CMD = 0x201;
  constexpr uint32_t ID_MOTOR_2_CMD = 0x202;
  constexpr uint32_t ID_MOTOR_3_CMD = 0x203;
  constexpr uint32_t ID_MOTOR_4_CMD = 0x204;
  
  // Motores (Motor → ECU)
  constexpr uint32_t ID_MOTOR_1_RESP = 0x281;
  constexpr uint32_t ID_MOTOR_2_RESP = 0x282;
  constexpr uint32_t ID_MOTOR_3_RESP = 0x283;
  constexpr uint32_t ID_MOTOR_4_RESP = 0x284;
  
  // Supervisor
  constexpr uint32_t ID_SUPERVISOR_HB = 0x100;
  constexpr uint32_t ID_SUPERVISOR_CMD = 0x101;
  ```

- [ ] **Definir tipos de mensajes BMS**
  ```cpp
  enum class BmsMsgType : char {
    VOLTAJE = 'V',      // Voltaje de celda
    TEMPERATURA = 'T',  // Temperatura de celda
    ESTADO = 'E',       // Estado del pack
    ALARMA = 'A'        // Alarmas
  };
  ```

- [ ] **Definir tipos de mensajes CCP (motores)**
  ```cpp
  enum class CcpMsgType : uint8_t {
    MSG_TIPO_01 = 0x01,  // Comando de velocidad
    MSG_TIPO_02 = 0x02,  // Comando de torque
    // ... hasta MSG_TIPO_13
  };
  ```

- [ ] **Crear estructuras de mensajes**
- [ ] **Implementar funciones de codificación/decodificación**

### 2.3 Implementación del Handler BMS

**Objetivo**: Implementar comunicación completa con el sistema de gestión de baterías.

#### Archivos Nuevos: `can_bms_handler.hpp` y `can_bms_handler.cpp`

- [ ] **Crear clase `BmsCanHandler`**
  ```cpp
  class BmsCanHandler {
  public:
    void process_message(const can_frame& frame);
    BatteryState get_battery_state() const;
    bool is_communication_ok() const;
  private:
    void decode_voltage_message(const can_frame& frame);
    void decode_temperature_message(const can_frame& frame);
    void decode_state_message(const can_frame& frame);
    void decode_alarm_message(const can_frame& frame);
  };
  ```

- [ ] **Implementar decodificación de voltajes** (24 celdas)
- [ ] **Implementar decodificación de temperaturas** (24 celdas)
- [ ] **Implementar decodificación de estado del pack**
- [ ] **Implementar decodificación de alarmas**
- [ ] **Implementar detección de timeout** (3 segundos)
- [ ] **Calcular estadísticas** (voltaje min/max/avg, temperatura min/max/avg)

### 2.4 Implementación de Comunicación con Motores

**Objetivo**: Implementar protocolo CCP para los 4 controladores de motores.

#### Tareas:
- [ ] **Extender `can_manager.hpp`**
  - Añadir `publish_motor_command(motor_id, command)`
  - Añadir `request_motor_telemetry(motor_id)`
  - Añadir `process_motor_response(frame)`

- [ ] **Implementar 13 tipos de mensajes CCP**
  - MSG_TIPO_01: Comando de velocidad
  - MSG_TIPO_02: Comando de torque
  - MSG_TIPO_03: Solicitud de telemetría
  - ... (según protocolo legacy)

- [ ] **Implementar gestión de 4 motores independientes**
  - Motor 1 (Delantero Izquierdo)
  - Motor 2 (Delantero Derecho)
  - Motor 3 (Trasero Izquierdo)
  - Motor 4 (Trasero Derecho)

### 2.5 Implementación de Comunicación con Supervisor

**Objetivo**: Establecer comunicación con módulo supervisor.

#### Tareas:
- [ ] **Implementar heartbeat periódico**
  ```cpp
  void publish_supervisor_heartbeat();
  // Enviar cada 100ms
  ```

- [ ] **Implementar procesamiento de comandos**
  ```cpp
  void process_supervisor_commands(const can_frame& frame);
  ```

- [ ] **Definir comandos del supervisor**
  - START_SYSTEM
  - STOP_SYSTEM
  - EMERGENCY_STOP
  - RESET_ALARMS

### 2.6 Configuración de Interfaces CAN

**Objetivo**: Configurar automáticamente las interfaces CAN del hardware.

#### Archivo: `scripts/setup_can.sh`

- [ ] **Configurar CAN0 @ 1 Mbps** (Motores + Supervisor)
  ```bash
  sudo ip link set can0 type can bitrate 1000000
  sudo ip link set up can0
  ```

- [ ] **Configurar CAN1 @ 500 Kbps** (BMS)
  ```bash
  sudo ip link set can1 type can bitrate 500000
  sudo ip link set up can1
  ```

- [ ] **Añadir modo virtual para testing**
  ```bash
  if [ "$1" == "--virtual" ]; then
    sudo ip link add dev vcan0 type vcan
    sudo ip link set up vcan0
  fi
  ```

- [ ] **Añadir verificación de módulos kernel**
- [ ] **Añadir configuración de filtros CAN**

---

## 📡 Fase 3: Integración de Sensores y Actuadores

### 3.1 Integración de Tarjeta PEX-1202L (ADC)

**Objetivo**: Leer entradas analógicas de sensores del vehículo.

#### Tareas:
- [ ] **Verificar driver de la tarjeta**
  ```bash
  lspci -v | grep -i analog
  ```

- [ ] **Implementar interfaz de lectura**
  - Archivo: `adquisicion_datos/pex1202l_interface.cpp`
  - Métodos: `read_channel()`, `read_all_channels()`

- [ ] **Mapear sensores a canales**
  - Canal 0: Sensor de presión suspensión FL
  - Canal 1: Sensor de presión suspensión FR
  - Canal 2: Sensor de presión suspensión RL
  - Canal 3: Sensor de presión suspensión RR
  - Canal 4-7: Sensores adicionales

- [ ] **Implementar calibración de sensores**
- [ ] **Implementar filtrado de señales**

### 3.2 Integración de Tarjeta PEX-DA16 (DAC)

**Objetivo**: Controlar actuadores del vehículo.

#### Tareas:
- [ ] **Verificar driver de la tarjeta**
  ```bash
  lspci -v | grep -i analog
  ```

- [ ] **Implementar interfaz de escritura**
  - Archivo: `adquisicion_datos/pexda16_interface.cpp`
  - Métodos: `write_channel()`, `write_digital_output()`

- [ ] **Mapear actuadores a canales**
  - Canal 0: Válvula suspensión FL
  - Canal 1: Válvula suspensión FR
  - Canal 2: Válvula suspensión RL
  - Canal 3: Válvula suspensión RR
  - Canales digitales: Relés, indicadores

- [ ] **Implementar límites de seguridad**
- [ ] **Implementar rampa de cambio suave**

### 3.3 Integración de GPS ublox SM-76G

**Objetivo**: Obtener posicionamiento y velocidad del vehículo.

#### Tareas:
- [ ] **Identificar puerto serial**
  ```bash
  ls -l /dev/ttyUSB* /dev/ttyS*
  ```

- [ ] **Implementar parser NMEA**
  - Archivo: `adquisicion_datos/gps_interface.cpp`
  - Parsear mensajes: GGA, RMC, VTG

- [ ] **Extraer datos relevantes**
  - Latitud/Longitud
  - Velocidad
  - Rumbo
  - Altitud
  - Número de satélites

- [ ] **Implementar detección de señal válida**
- [ ] **Implementar timeout de GPS**

### 3.4 Configuración de Sensores de Suspensión

**Objetivo**: Leer sensores de posición y presión de la suspensión activa.

#### Tareas:
- [ ] **Identificar sensores disponibles**
  - Sensores de altura (4)
  - Sensores de presión (4)
  - Acelerómetros (si disponibles)

- [ ] **Implementar lectura de sensores de altura**
- [ ] **Implementar lectura de sensores de presión**
- [ ] **Calibrar rangos de operación**
- [ ] **Implementar detección de fallos**

### 3.5 Configuración de Sensores de Tracción

**Objetivo**: Leer sensores relacionados con el sistema de tracción.

#### Tareas:
- [ ] **Identificar sensores disponibles**
  - Sensores de velocidad de rueda (4)
  - Sensores de aceleración
  - Sensor de ángulo de dirección

- [ ] **Implementar lectura de velocidad de ruedas**
- [ ] **Implementar lectura de aceleración**
- [ ] **Implementar lectura de ángulo de dirección**
- [ ] **Calcular deslizamiento de ruedas**

---

## 🎮 Fase 4: Control del Vehículo

### 4.1 Sistema de Gestión de Batería

**Objetivo**: Implementar control y monitoreo completo del BMS.

#### Tareas:
- [ ] **Actualizar estructura `BatteryState`**
  ```cpp
  struct BatteryState {
    std::array<uint16_t, 24> cell_voltages_mv;
    std::array<uint8_t, 24> cell_temperatures_c;
    uint8_t num_cells_detected;
    uint16_t voltage_avg_mv, voltage_max_mv, voltage_min_mv;
    uint8_t temp_avg_c, temp_max_c;
    uint8_t cell_v_max_id, cell_v_min_id, cell_temp_max_id;
    float soc_percent;
    float current_a;
    AlarmLevel alarm_level;
    std::vector<AlarmType> active_alarms;
  };
  ```

- [ ] **Implementar detección de alarmas**
  - CELL_TEMP_HIGH (>60°C)
  - CELL_TEMP_LOW (<0°C)
  - CELL_V_HIGH (>4.2V)
  - CELL_V_LOW (<2.8V)
  - PACK_V_HIGH
  - PACK_V_LOW
  - PACK_I_HIGH

- [ ] **Implementar niveles de alarma**
  - NO_ALARMA: Operación normal
  - WARNING: Advertencia
  - ALARMA: Reducir potencia
  - ALARMA_CRITICA: Detener vehículo

- [ ] **Implementar acciones de protección**
  - Limitar corriente de carga/descarga
  - Reducir potencia de motores
  - Activar modo seguro

### 4.2 Sistema de Control de Tracción

**Objetivo**: Implementar control de tracción para las 4 ruedas.

#### Archivo: `control_vehiculo/traction_controller.cpp`

- [ ] **Implementar detección de deslizamiento**
  ```cpp
  float calculate_slip_ratio(float wheel_speed, float vehicle_speed);
  ```

- [ ] **Implementar control de torque por rueda**
  ```cpp
  void adjust_motor_torque(int motor_id, float target_torque);
  ```

- [ ] **Implementar distribución de torque**
  - Modo 2WD (trasero)
  - Modo 4WD
  - Modo vectorización de torque

- [ ] **Implementar límites de seguridad**
  - Torque máximo por motor
  - Aceleración máxima
  - Velocidad máxima

- [ ] **Implementar modos de conducción**
  - ECO: Eficiencia máxima
  - NORMAL: Balance
  - SPORT: Rendimiento máximo

### 4.3 Sistema de Suspensión Activa

**Objetivo**: Implementar control de suspensión activa de 4 ruedas.

#### Archivo: `control_vehiculo/suspension_controller.cpp`

- [ ] **Implementar control de altura**
  ```cpp
  void set_ride_height(SuspensionCorner corner, float target_height_mm);
  ```

- [ ] **Implementar control de rigidez**
  ```cpp
  void set_damping_level(SuspensionCorner corner, DampingLevel level);
  ```

- [ ] **Implementar compensación de balanceo**
  - Anti-roll en curvas
  - Anti-dive en frenado
  - Anti-squat en aceleración

- [ ] **Implementar modos de suspensión**
  - COMFORT: Máximo confort
  - NORMAL: Balance
  - SPORT: Máxima respuesta

- [ ] **Implementar nivelación automática**
  - Compensar carga
  - Mantener altura constante

### 4.4 Sistema de Alarmas Multinivel

**Objetivo**: Implementar sistema centralizado de alarmas.

#### Archivo: `control_vehiculo/alarm_manager.cpp`

- [ ] **Definir tipos de alarmas**
  ```cpp
  enum class AlarmType {
    // Batería
    BATTERY_TEMP_HIGH,
    BATTERY_VOLTAGE_LOW,
    BATTERY_CURRENT_HIGH,
    
    // Motores
    MOTOR_TEMP_HIGH,
    MOTOR_OVERCURRENT,
    MOTOR_COMM_LOST,
    
    // Sistema
    CAN_BUS_ERROR,
    SENSOR_FAULT,
    ACTUATOR_FAULT
  };
  ```

- [ ] **Implementar priorización de alarmas**
- [ ] **Implementar acciones automáticas**
  - Reducir potencia
  - Activar modo seguro
  - Detener vehículo

- [ ] **Implementar registro de alarmas**
- [ ] **Implementar notificación al conductor**

### 4.5 Máquina de Estados del Vehículo

**Objetivo**: Implementar control de estados del sistema.

#### Archivo: `logica_sistema/state_machine.cpp`

- [ ] **Definir estados del sistema**
  ```cpp
  enum class SystemState {
    INIT,           // Inicialización
    STANDBY,        // En espera
    READY,          // Listo para arrancar
    RUNNING,        // En funcionamiento
    SAFE_MODE,      // Modo seguro
    ERROR,          // Error crítico
    SHUTDOWN        // Apagado
  };
  ```

- [ ] **Implementar transiciones de estado**
  ```
  INIT → STANDBY → READY → RUNNING
                    ↓         ↓
                ERROR ← SAFE_MODE
  ```

- [ ] **Implementar condiciones de transición**
  - INIT → STANDBY: Hardware inicializado
  - STANDBY → READY: BMS OK, motores OK
  - READY → RUNNING: Comando de arranque
  - * → SAFE_MODE: Alarma no crítica
  - * → ERROR: Alarma crítica

- [ ] **Implementar acciones por estado**
  - INIT: Inicializar hardware
  - STANDBY: Monitoreo pasivo
  - READY: Sistemas listos
  - RUNNING: Control activo
  - SAFE_MODE: Funcionalidad limitada
  - ERROR: Detener todo

---

## 🧪 Fase 5: Pruebas y Validación

### 5.1 Pruebas con Interfaces Virtuales

**Objetivo**: Validar el código sin hardware real.

#### Tareas:
- [ ] **Configurar interfaz virtual CAN**
  ```bash
  sudo modprobe vcan
  sudo ip link add dev vcan0 type vcan
  sudo ip link set up vcan0
  ```

- [ ] **Compilar proyecto**
  ```bash
  cd ecu_atc8110
  mkdir build && cd build
  cmake ..
  make
  ```

- [ ] **Ejecutar con interfaz virtual**
  ```bash
  ./ecu_atc8110
  ```

- [ ] **Verificar logs de inicialización**
  - SocketCAN inicializado
  - Interfaces CAN detectadas
  - Managers iniciados

### 5.2 Pruebas de Loopback CAN

**Objetivo**: Validar envío y recepción de mensajes CAN.

#### Tareas:
- [ ] **Enviar mensaje de prueba BMS**
  ```bash
  # Terminal 1: Ejecutar ECU
  ./ecu_atc8110
  
  # Terminal 2: Enviar mensaje simulado
  cansend vcan0 180#5601AA55  # Voltaje celda 1
  ```

- [ ] **Verificar recepción en logs**
- [ ] **Probar todos los tipos de mensajes**
  - Mensajes BMS (V, T, E, A)
  - Mensajes motores (13 tipos)
  - Mensajes supervisor

- [ ] **Usar `candump` para monitoreo**
  ```bash
  candump vcan0
  ```

### 5.3 Pruebas con Hardware Real - BMS

**Objetivo**: Validar comunicación con BMS real.

> [!IMPORTANT]
> Estas pruebas requieren hardware CAN real conectado.

#### Tareas:
- [ ] **Conectar BMS al bus CAN1**
- [ ] **Configurar interfaz CAN1**
  ```bash
  sudo ./scripts/setup_can.sh --real
  ```

- [ ] **Ejecutar ECU**
  ```bash
  sudo ./ecu_atc8110
  ```

- [ ] **Verificar recepción de datos**
  - Voltajes de 24 celdas
  - Temperaturas de 24 celdas
  - Estado del pack
  - Alarmas (si existen)

- [ ] **Monitorear con `candump`**
  ```bash
  candump can1
  ```

- [ ] **Provocar condiciones de alarma**
  - Simular voltaje bajo
  - Simular temperatura alta
  - Verificar detección

- [ ] **Probar timeout**
  - Desconectar BMS
  - Verificar detección de pérdida de comunicación (~3s)

### 5.4 Pruebas con Motores

**Objetivo**: Validar comunicación y control de motores.

#### Tareas:
- [ ] **Conectar 4 controladores al bus CAN0**
- [ ] **Verificar IDs CAN de cada motor**
  - Motor 1: CMD=0x201, RESP=0x281
  - Motor 2: CMD=0x202, RESP=0x282
  - Motor 3: CMD=0x203, RESP=0x283
  - Motor 4: CMD=0x204, RESP=0x284

- [ ] **Enviar comandos de prueba**
  - Comando de velocidad (MSG_TIPO_01)
  - Solicitud de telemetría (MSG_TIPO_03)

- [ ] **Verificar respuestas**
  - Velocidad actual
  - Corriente
  - Temperatura
  - Estado

- [ ] **Probar control de torque**
  - Enviar torque bajo
  - Incrementar gradualmente
  - Verificar respuesta del motor

- [ ] **Probar detección de fallos**
  - Desconectar un motor
  - Verificar timeout
  - Verificar alarma

### 5.5 Pruebas de Integración Completa

**Objetivo**: Validar funcionamiento del sistema completo.

#### Tareas:
- [ ] **Conectar todos los componentes**
  - BMS en CAN1
  - 4 motores en CAN0
  - Supervisor en CAN0
  - Sensores en PEX-1202L
  - Actuadores en PEX-DA16
  - GPS en puerto serial

- [ ] **Ejecutar sistema completo**
  ```bash
  sudo ./ecu_atc8110
  ```

- [ ] **Verificar inicialización**
  - Todos los managers iniciados
  - Todas las comunicaciones establecidas
  - Sin errores críticos

- [ ] **Probar secuencia de arranque**
  - Estado INIT → STANDBY
  - STANDBY → READY (cuando BMS y motores OK)
  - READY → RUNNING (comando de arranque)

- [ ] **Probar funcionalidad de control**
  - Control de tracción
  - Control de suspensión
  - Gestión de batería

- [ ] **Probar sistema de alarmas**
  - Provocar alarma no crítica → SAFE_MODE
  - Provocar alarma crítica → ERROR

### 5.6 Validación del Sistema de Alarmas

**Objetivo**: Verificar que todas las alarmas funcionan correctamente.

#### Tareas:
- [ ] **Probar alarmas de batería**
  - Voltaje alto de celda
  - Voltaje bajo de celda
  - Temperatura alta
  - Corriente alta

- [ ] **Probar alarmas de motores**
  - Temperatura alta
  - Sobrecorriente
  - Pérdida de comunicación

- [ ] **Probar alarmas de sistema**
  - Error de bus CAN
  - Fallo de sensor
  - Fallo de actuador

- [ ] **Verificar acciones automáticas**
  - Reducción de potencia
  - Activación de modo seguro
  - Detención de vehículo

---

## 🚀 Fase 6: Despliegue y Puesta en Marcha

### 6.1 Compilación en ECU

**Objetivo**: Compilar el código final en la ECU de producción.

#### Tareas:
- [ ] **Transferir código a ECU**
  ```bash
  scp -r ecu_atc8110 fox@193.147.165.236:/home/fox/
  ```

- [ ] **Conectar a ECU**
  ```bash
  ssh fox@193.147.165.236
  ```

- [ ] **Compilar en modo Release**
  ```bash
  cd /home/fox/ecu_atc8110
  mkdir build && cd build
  cmake -DCMAKE_BUILD_TYPE=Release ..
  make -j4
  ```

- [ ] **Verificar binario**
  ```bash
  ls -lh ecu_atc8110
  file ecu_atc8110
  ```

### 6.2 Configuración de Hardware Real

**Objetivo**: Configurar todas las interfaces de hardware.

#### Tareas:
- [ ] **Ejecutar script de configuración**
  ```bash
  sudo /home/fox/ecu_atc8110/scripts/setup_can.sh --real
  ```

- [ ] **Verificar interfaces CAN**
  ```bash
  ip link show can0
  ip link show can1
  ip -s link show can0
  ip -s link show can1
  ```

- [ ] **Verificar tarjetas PEX**
  ```bash
  lspci | grep -i analog
  ```

- [ ] **Verificar GPS**
  ```bash
  ls -l /dev/ttyUSB*
  cat /dev/ttyUSB0  # Ver datos NMEA
  ```

### 6.3 Ejecución del Sistema Completo

**Objetivo**: Poner en marcha el sistema de control del vehículo.

#### Tareas:
- [ ] **Ejecutar ECU**
  ```bash
  sudo /home/fox/ecu_atc8110/build/ecu_atc8110
  ```

- [ ] **Monitorear logs en tiempo real**
  ```bash
  # En otra terminal
  tail -f /var/log/ecu_atc8110.log
  ```

- [ ] **Verificar estado del sistema**
  - Estado: INIT → STANDBY → READY
  - BMS: Comunicación OK
  - Motores: 4/4 conectados
  - Sensores: Leyendo datos
  - Sin alarmas críticas

### 6.4 Monitoreo y Ajuste de Parámetros

**Objetivo**: Ajustar parámetros para operación óptima.

#### Tareas:
- [ ] **Monitorear bus CAN**
  ```bash
  # Terminal 1: CAN0 (Motores)
  candump can0
  
  # Terminal 2: CAN1 (BMS)
  candump can1
  ```

- [ ] **Ajustar parámetros de control**
  - Ganancias PID de tracción
  - Ganancias PID de suspensión
  - Límites de torque
  - Límites de corriente

- [ ] **Calibrar sensores**
  - Sensores de altura
  - Sensores de presión
  - Sensores de velocidad

- [ ] **Ajustar umbrales de alarmas**
  - Temperatura máxima
  - Voltaje mínimo/máximo
  - Corriente máxima

### 6.5 Documentación de Configuración Final

**Objetivo**: Documentar la configuración final del sistema.

#### Tareas:
- [ ] **Documentar parámetros finales**
  - Archivo: `config/production.yaml`
  - Incluir todos los parámetros calibrados

- [ ] **Documentar IDs CAN utilizados**
  - BMS: 0x180
  - Motores: 0x201-0x204, 0x281-0x284
  - Supervisor: 0x100-0x101

- [ ] **Documentar mapeo de sensores/actuadores**
  - Canales PEX-1202L
  - Canales PEX-DA16
  - Puerto GPS

- [ ] **Crear guía de operación**
  - Procedimiento de arranque
  - Procedimiento de apagado
  - Manejo de alarmas
  - Troubleshooting

- [ ] **Crear checklist de pre-arranque**
  - Verificar batería cargada
  - Verificar conexiones CAN
  - Verificar sensores
  - Verificar ausencia de alarmas

### 6.6 Pruebas Finales en Vehículo

**Objetivo**: Validar el sistema en condiciones reales de operación.

> [!CAUTION]
> Estas pruebas deben realizarse en un entorno controlado y seguro.

#### Tareas:
- [ ] **Prueba estática (vehículo detenido)**
  - Arranque del sistema
  - Verificar todos los sensores
  - Verificar comunicaciones
  - Probar actuadores (sin movimiento)

- [ ] **Prueba de tracción (baja velocidad)**
  - Acelerar suavemente
  - Verificar respuesta de motores
  - Verificar control de tracción
  - Verificar consumo de batería

- [ ] **Prueba de suspensión**
  - Cambiar modos de suspensión
  - Verificar respuesta de actuadores
  - Verificar lectura de sensores
  - Verificar compensación de balanceo

- [ ] **Prueba de sistema de alarmas**
  - Simular condiciones de alarma
  - Verificar detección
  - Verificar acciones automáticas
  - Verificar recuperación

- [ ] **Prueba de autonomía**
  - Conducción normal durante 30 minutos
  - Monitorear estado de batería
  - Verificar estabilidad del sistema
  - Verificar ausencia de errores

---

## 📊 Criterios de Aceptación

### Sistema Base
- ✅ Ubuntu 18.04 LTS instalado y configurado
- ✅ Todas las dependencias instaladas
- ✅ Acceso SSH funcionando
- ✅ Módulos CAN cargados

### Comunicación CAN
- ✅ Interfaces can0 y can1 configuradas
- ✅ Comunicación con BMS establecida (24 celdas)
- ✅ Comunicación con 4 motores establecida
- ✅ Comunicación con supervisor establecida
- ✅ Timeout de comunicación funcionando

### Sensores y Actuadores
- ✅ PEX-1202L leyendo sensores
- ✅ PEX-DA16 controlando actuadores
- ✅ GPS obteniendo posición
- ✅ Todos los sensores calibrados

### Control del Vehículo
- ✅ Control de tracción funcionando
- ✅ Control de suspensión funcionando
- ✅ Gestión de batería funcionando
- ✅ Sistema de alarmas funcionando
- ✅ Máquina de estados funcionando

### Pruebas
- ✅ Todas las pruebas unitarias pasando
- ✅ Pruebas de integración pasando
- ✅ Pruebas con hardware real exitosas
- ✅ Pruebas en vehículo exitosas

### Documentación
- ✅ Configuración final documentada
- ✅ Guía de operación creada
- ✅ Procedimientos de mantenimiento documentados

---

## ⚠️ Riesgos y Mitigaciones

### Riesgo 1: Hardware incompatible
**Mitigación**: Verificar compatibilidad antes de comenzar. Tener plan B con hardware alternativo.

### Riesgo 2: Drivers no disponibles
**Mitigación**: Investigar drivers antes de comenzar. Considerar desarrollo de drivers propios si es necesario.

### Riesgo 3: Protocolo CAN desconocido
**Mitigación**: Analizar código legacy. Usar analizador CAN para capturar tráfico real.

### Riesgo 4: Problemas de timing en tiempo real
**Mitigación**: Usar hilos con prioridad alta. Considerar kernel RT si es necesario.

### Riesgo 5: Alarmas no detectadas
**Mitigación**: Pruebas exhaustivas de todos los escenarios de alarma. Implementar watchdog.

---

## 📅 Estimación de Tiempo

| Fase | Duración Estimada | Dependencias |
|------|-------------------|--------------|
| Fase 1: Preparación | 1-2 días | Ninguna |
| Fase 2: CAN | 3-5 días | Fase 1 |
| Fase 3: Sensores | 2-3 días | Fase 1 |
| Fase 4: Control | 3-4 días | Fases 2 y 3 |
| Fase 5: Pruebas | 2-3 días | Fase 4 |
| Fase 6: Despliegue | 1-2 días | Fase 5 |
| **TOTAL** | **12-19 días** | |

> [!NOTE]
> Estas estimaciones asumen trabajo a tiempo completo y disponibilidad de hardware.

---

## 📞 Contactos y Recursos

### Documentación Técnica
- [Implementation Plan](file:///c:/Users/ahech/Desktop/FOX/ecu_atc8110/docs/implementation_plan.md)
- [README Principal](file:///c:/Users/ahech/Desktop/FOX/README.md)
- [README ECU](file:///c:/Users/ahech/Desktop/FOX/ecu_atc8110/README.md)

### Hardware
- **ECU**: ATC-8110 @ 193.147.165.236
- **Usuario**: fox
- **CAN0**: 1 Mbps (Motores + Supervisor)
- **CAN1**: 500 Kbps (BMS)

### Herramientas
- **can-utils**: candump, cansend, canconfig
- **CMake**: Sistema de compilación
- **SSH**: Acceso remoto

---

## ✅ Checklist Final

Antes de considerar la migración completa:

- [ ] Todos los componentes de hardware verificados
- [ ] Todas las comunicaciones CAN funcionando
- [ ] Todos los sensores leyendo datos válidos
- [ ] Todos los actuadores respondiendo correctamente
- [ ] Sistema de alarmas probado exhaustivamente
- [ ] Máquina de estados funcionando correctamente
- [ ] Pruebas en vehículo completadas exitosamente
- [ ] Documentación completa
- [ ] Personal capacitado en operación y mantenimiento

---

**Última actualización**: 2025-11-28
**Versión**: 1.0
**Estado**: ✅ Listo para ejecución
