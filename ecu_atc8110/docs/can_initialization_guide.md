# Diagnóstico y Solución: Motores y Supervisor CAN No Responden

## Problema Identificado

**Síntoma**: El BMS funciona correctamente en CAN, pero los motores (CAN1) y el supervisor (CAN2) no se detectan o no responden.

**Causa Raíz**: Los controladores de motor y el supervisor requieren una **secuencia de activación** desde la ECU antes de comenzar a transmitir datos. No responden automáticamente al encenderse.

## Solución Implementada

### 1. Clase `CanInitializer`

Se ha creado [`can_initializer.hpp`](file:///C:/Users/ahech/Desktop/FOX/ecu_atc8110/comunicacion_can/can_initializer.hpp) que implementa la secuencia de activación correcta:

**Secuencia para Motores** (cada uno de los 4 motores):
1. Enviar comando inicial con throttle=0 (despertar controlador)
2. Solicitar información del módulo (MSG_TIPO_01)
3. Solicitar versión de software (MSG_TIPO_02)
4. Activar switches:
   - Switch acelerador (MSG_TIPO_11)
   - Switch freno (MSG_TIPO_12)
   - Switch reversa (MSG_TIPO_13)

**Secuencia para Supervisor**:
1. Enviar 3 heartbeats iniciales para anunciar presencia de ECU
2. Esperar respuesta del supervisor

### 2. Integración en StateMachine

El `StateMachine` ahora usa el `CanInitializer` automáticamente al arrancar:

```cpp
// En state_machine.hpp
#include "../comunicacion_can/can_initializer.hpp"

bool StateMachine::initialize_motors() {
    CanInitializer initializer(can_);
    return initializer.initialize_all();  // Activa motores Y supervisor
}
```

## Verificación

### Paso 1: Configurar Interfaces CAN

```bash
cd ecu_atc8110
sudo ./scripts/setup_can.sh --real
```

**Salida esperada**:
```
✓ emuccan0 activada (hardware EMUC-B2S3)
✓ emuccan1 activada (hardware EMUC-B2S3)
```

### Paso 2: Verificar Interfaces Activas

```bash
ip link show | grep emuccan
```

**Salida esperada**:
```
emuccan0: <NOARP,UP,LOWER_UP> ...
emuccan1: <NOARP,UP,LOWER_UP> ...
```

### Paso 3: Monitorear Tráfico CAN

En una terminal separada, monitorear el tráfico CAN:

```bash
# Terminal 1: Monitorear CAN de motores/supervisor
candump emuccan0

# Terminal 2: Monitorear CAN de BMS
candump emuccan1
```

### Paso 4: Ejecutar ECU

```bash
./build/ecu_atc8110
```

**Salida esperada en logs**:
```
[INFO] StateMachine: Iniciando secuencia de activación CAN...
[INFO] CanInitializer: === INICIALIZACIÓN CAN - MOTORES Y SUP ===
[INFO] CanInitializer: === Iniciando comunicación con supervisor ===
[INFO] CanInitializer: === Heartbeat enviado al supervisor ===
[INFO] CanInitializer: === Iniciando secuencia de activación de motores ===
[INFO] CanInitializer: Activando motor 1...
[INFO] CanInitializer: Motor 1 activado
[INFO] CanInitializer: Activando motor 2...
[INFO] CanInitializer: Motor 2 activado
[INFO] CanInitializer: Activando motor 3...
[INFO] CanInitializer: Motor 3 activado
[INFO] CanInitializer: Activando motor 4...
[INFO] CanInitializer: Motor 4 activado
[INFO] CanInitializer: === INICIALIZACIÓN CAN COMPLETADA ✓ ===
[INFO] StateMachine: Motores y supervisor activados correctamente
```

**En candump (emuccan0)** deberías ver:
```
emuccan0  100   [3]  AA 55 01           # Heartbeat ECU → Supervisor
emuccan0  201   [2]  00 00              # Comando inicial Motor 1
emuccan0  281   [8]  F4 00 ...          # Respuesta Motor 1 (info módulo)
emuccan0  202   [2]  00 00              # Comando inicial Motor 2
emuccan0  282   [8]  F4 00 ...          # Respuesta Motor 2
...
```

## Diagnóstico de Problemas

### Problema: "No se pudo contactar motor X"

**Posibles causas**:
1. Motor no conectado físicamente
2. Interfaz CAN incorrecta
3. Baudrate incorrecto
4. Terminación CAN faltante

**Solución**:
```bash
# Verificar que la interfaz está UP
ip link show emuccan0

# Verificar baudrate (debe ser 1 Mbps para motores)
ip -details link show emuccan0 | grep bitrate

# Reiniciar interfaz
sudo ip link set emuccan0 down
sudo ip link set emuccan0 up
```

### Problema: "Interfaz emuccan0 no encontrada"

**Causa**: Driver EMUC-B2S3 no cargado

**Solución**:
```bash
# Verificar módulos cargados
lsmod | grep can

# Cargar módulos manualmente
sudo modprobe can
sudo modprobe can_raw

# Si usa driver específico EMUC
sudo modprobe emuc_can  # (nombre puede variar)
```

### Problema: Supervisor no responde

**Causa**: El supervisor puede estar en modo de espera

**Solución**:
1. Verificar que el supervisor está encendido
2. Aumentar número de heartbeats iniciales en `can_initializer.hpp`:
   ```cpp
   // Cambiar de 3 a 5 heartbeats
   for (int i = 0; i < 5; ++i) {
       can_manager_.publish_heartbeat();
       std::this_thread::sleep_for(std::chrono::milliseconds(100));
   }
   ```

## Mapeo de Interfaces CAN

| Interfaz | Baudrate | Dispositivos | ID CAN |
|----------|----------|--------------|--------|
| `emuccan0` | 1 Mbps | Motores 1-4 | 0x201-0x204 (CMD)<br>0x281-0x284 (RESP) |
| `emuccan0` | 1 Mbps | Supervisor | 0x100 (HB)<br>0x101 (CMD) |
| `emuccan1` | 500 Kbps | BMS | 0x180 |

## Comandos Útiles

```bash
# Ver estadísticas de CAN
ip -s link show emuccan0

# Ver errores de CAN
ip -s -s link show emuccan0

# Enviar mensaje CAN manual (prueba)
cansend emuccan0 201#0000

# Grabar tráfico CAN a archivo
candump -l emuccan0

# Reproducir tráfico CAN
canplayer -I candump-2025-11-27.log
```

## Próximos Pasos

1. **Compilar y probar** con hardware real
2. **Ajustar timings** si algunos motores no responden (aumentar delays en `can_initializer.hpp`)
3. **Configurar filtros CAN** para optimizar recepción (actualmente recibe todos los mensajes)
4. **Implementar watchdog** para detectar pérdida de comunicación con motores/supervisor

## Referencias

- Protocolo CAN: [`can_protocol.hpp`](file:///C:/Users/ahech/Desktop/FOX/ecu_atc8110/comunicacion_can/can_protocol.hpp)
- Gestor CAN: [`can_manager.hpp`](file:///C:/Users/ahech/Desktop/FOX/ecu_atc8110/comunicacion_can/can_manager.hpp)
- Inicializador: [`can_initializer.hpp`](file:///C:/Users/ahech/Desktop/FOX/ecu_atc8110/comunicacion_can/can_initializer.hpp)
- Máquina de estados: [`state_machine.hpp`](file:///C:/Users/ahech/Desktop/FOX/ecu_atc8110/logica_sistema/state_machine.hpp)
