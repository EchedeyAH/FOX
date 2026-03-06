# PRIORIDAD 1: Implementación de Sistema de Errores, Validación CAN y Gestión de Potencia

## 1️⃣ DISEÑO (MÓDULOS + API)

### 1.1 Diagrama de Módulos y Dependencias

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         SYSTEM SUPERVISOR (Unified API)                     │
│                    common/system_supervisor.hpp/cpp                          │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐        │
│  │  ErrorManager  │    │ CanValidator   │    │ PowerManager   │        │
│  │ error_system   │    │ can_validator  │    │ power_manager  │        │
│  │    .h/.cpp     │    │    .h/.cpp     │    │    .h/.cpp     │        │
│  └────────┬────────┘    └────────┬────────┘    └────────┬────────┘        │
│           │                       │                       │                 │
│           │   ┌───────────────────┼───────────────────┐   │                 │
│           │   │           SystemSnapshot              │   │                 │
│           │   │  (battery, motors, vehicle, faults)    │   │                 │
│           │   └───────────────────┬───────────────────┘   │                 │
│           │                       │                       │                 │
│           ▼                       ▼                       ▼                 │
│  ┌─────────────────────────────────────────────────────────────────┐       │
│  │                     RtContext (shared state)                     │       │
│  │                   logica_sistema/rt_context.hpp                 │       │
│  └─────────────────────────────────────────────────────────────────┘       │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
                                       │
                                       ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                           HILOS EXISTENTES                                  │
├─────────────────────────────────────────────────────────────────────────────┤
│  thread_can_rx  ──────► CanValidator.register_rx()                         │
│  thread_adc     ──────► (reads sensors → snapshot)                         │
│  thread_control ──────► SystemSupervisor.update()                          │
│                       ──────► get_power_limit_factor()                      │
│                       ──────► get_max_allowed_torque()                      │
│  thread_can_tx  ──────► (sends commands)                                  │
│  thread_watchdog ─────► SystemSupervisor.check_system_health()             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 1.2 Estados del Sistema Relacionados con Errores

| Estado | Descripción | Acción |
|--------|-------------|--------|
| `OK` | Sistema funcionando normalmente | Torque = 100% |
| `WARNING` | Error leve detectado | Log + Warning, torque = 100% |
| `ERROR` | Error grave detectado | Log + Warning, torque = 70% |
| `CRITICAL` | Error crítico detectado | Modo seguro, torque = 0% |
| `LIMP_MODE` | Modo degradado por límite potencia | Torque = 30% |
| `EMERGENCY_STOP` | Parada de emergencia | Todo a 0, motors OFF |

### 1.3 Tabla de Códigos de Error

| ID | Código Hex | Nivel | Condición de Disparo | Acción |
|----|------------|-------|---------------------|--------|
| ECU_HILOS | 0x3001 | CRÍTICO | Hilos no arrancan | Emergency Stop |
| ECU_COM_TCAN1 | 0x3004 | CRÍTICO | Timeout CAN1 | Retry + Warning |
| BMS_COM | 0x1101 | GRAVE | Timeout BMS 1s | Warning + Limp Mode |
| BMS_TEMP | 0x1702 | GRAVE | Temp celda > 65°C | Reducir potencia |
| MOTOR_M1_COM | 0x2101 | CRÍTICO | Timeout M1 2s | Emergency Stop |
| MOTOR_M2_COM | 0x2201 | CRÍTICO | Timeout M2 2s | Emergency Stop |
| MOTOR_M3_COM | 0x2301 | CRÍTICO | Timeout M3 2s | Emergency Stop |
| MOTOR_M4_COM | 0x2401 | CRÍTICO | Timeout M4 2s | Emergency Stop |
| MOTOR_M1_TEMP | 0x2703 | LEVE | Temp M1 > 60°C | Warning |
| MOTOR_M1_TEMP | 0x2703 | GRAVE | Temp M1 > 80°C | Reducir potencia |
| SUPERV_HB_TIMEOUT | 0x6801 | CRÍTICO | Timeout heartbeat 500ms | Emergency Stop |
| CAN_TX_FAIL | 0x7101 | GRAVE | Error envío CAN | Retry |
| SENSOR_ACEL | 0x4407 | LEVE | Sensor acelerador outlier | Warning |
| SENSOR_FRENO | 0x4406 | LEVE | Sensor freno outlier | Warning |

---

## 2️⃣ IMPLEMENTACIÓN PASO A PASO

### Paso 1: Módulo de Errores (Data Model + API + Contadores + Debounce)

**Archivos a crear:**
- `common/error_system.hpp` ✅ CREADO
- `common/error_system.cpp` ✅ CREADO

**Funcionalidad implementada:**
- Sistema de 3 niveles (LEVE/GRAVE/CRÍTICO)
- Contadores de errores con debounce
- Pool de 32 errores activos
- APIs: `raise_error()`, `clear_error()`, `get_system_level()`

### Paso 2: Watchdogs y Heartbeats

**Integración en `rt_context.hpp`:**
```
cpp
// Añadir al RtContext
common::ErrorManager error_manager;
```

**Integración en `can_manager.hpp`:**
```
cpp
// En process_rx(), después de procesar mensaje:
ctx.error_manager.update_heartbeat(Subsystem::MOTOR1 + motor_idx);
```

**Integración en `thread_watchdog`:**
```
cpp
// En el bucle del hilo watchdog:
bool critical = ctx.error_manager.check_watchdogs();
if (critical) {
    ctx.estado.store(EstadoEcu::Error);
}
```

### Paso 3: Validación CAN (Timeout, Retry, Fallos)

**Archivos a crear:**
- `common/can_validator.hpp` ✅ CREADO
- `common/can_validator.cpp` ✅ CREADO

**Funcionalidad:**
- Timeout por mensaje esperado (2s motors, 1s BMS)
- Conteo de reintentos
- Callbacks para eventos de timeout

### Paso 4: Gestión de Potencia

**Archivos a crear:**
- `common/power_manager.hpp` ✅ CREADO
- `common/power_manager.cpp` ✅ CREADO

**Funcionalidad:**
- Cálculo potencia demandada
- Limitaciones por SOC, temperatura, voltaje
- Rampa de potencia (10%/s)
- Distribución entre motores

### Paso 5: Integración con Hilos Existentes

**Archivos a modificar:**
- `logica_sistema/rt_threads.hpp`
- `logica_sistema/main.cpp`

---

## 3️⃣ CÓDIGO PROPUESTO (ESTRUCTURAS + FUNCIONES + INTEGRACIÓN)

### 3.1 Modificaciones en rt_context.hpp

```
cpp
// En rt_context.hpp, añadir:
#include "error_system.hpp"
#include "can_validator.hpp"
#include "power_manager.hpp"
#include "system_supervisor.hpp"

struct RtContext {
    // ... existentes ...
    
    // === NUEVO: Sistema de supervisión ===
    common::SystemSupervisor supervisor;
};
```

### 3.2 Modificaciones en rt_threads.hpp

```
cpp
// === thread_control (Hilo 3) - AÑADIR ===

// Al inicio del update():
void* thread_control(void* arg) {
    // ... código existente ...
    
    while (ctx->running.load()) {
        EstadoEcu estado = ctx->estado.load();
        
        if (estado == EstadoEcu::Operando) {
            // NUEVO: Update del supervisor
            auto snap = ctx->read_snapshot();
            ctx->supervisor.update(snap);
            
            // NUEVO: Obtener factor de limitación de potencia
            double power_limit = ctx->supervisor.get_power_limit_factor();
            
            // NUEVO: Obtener torque máximo permitido
            double max_torque = ctx->supervisor.get_max_allowed_torque();
            
            // Aplicar límites a los controladores
            for (size_t i = 0; i < snap.motors.size(); ++i) {
                snap.motors[i].torque_nm = std::min(
                    snap.motors[i].torque_nm * power_limit, 
                    max_torque
                );
            }
            
            // Ejecutar controladores
            battery_mgr->update(snap);
            suspension_ctrl->update(snap);
            traction_ctrl->update(snap);
            
            // ... resto existente ...
        }
        
        // ... 
    }
}

// === thread_watchdog (Hilo 6) - MODIFICAR ===

void* thread_watchdog(void* arg) {
    auto* ctx = static_cast<RtContext*>(arg);
    
    while (ctx->running.load()) {
        // NUEVO: Usar supervisor en lugar de lógica simple
        bool critical = ctx->supervisor.check_system_health();
        
        auto snap = ctx->read_snapshot();
        
        if (critical || snap.faults.critical) {
            LOG_ERROR("WDG", "Error crítico detectado");
            ctx->estado.store(EstadoEcu::Error);
        }
        
        // NUEVO: Log de estado del supervisor cada 10 ciclos
        static int log_cnt = 0;
        if (log_cnt++ % 10 == 0) {
            LOG_INFO("WDG", ctx->supervisor.dump_status());
        }
        
        common::periodic_sleep(next, common::periods::WATCHDOG_NS);
    }
}
```

### 3.3 Modificaciones en can_manager.hpp

```
cpp
// En CanManager::process_rx(), añadir:
void process_rx(common::SystemSnapshot &snapshot) {
    auto frame = driver_.receive();
    if (!frame) return;
    
    // Determinar qué peer envió el mensaje
    uint8_t peer_id = 255; // default supervisor
    
    if (frame->id == ID_CAN_BMS) {
        peer_id = 0; // BMS
        bms_handler_->process_message(*frame, snapshot.battery);
    }
    else if (frame->id >= ID_MOTOR_1_RESP && frame->id <= ID_MOTOR_4_RESP) {
        peer_id = frame->id - ID_MOTOR_1_RESP + 1; // 1-4
        process_motor_response(*frame, snapshot);
    }
    else if (frame->id == ID_SUPERVISOR_HB) {
        peer_id = 5; // Supervisor
    }
    
    // IMPORTANTE: Registrar el heartbeat/mensaje recibido
    // Esto debe integrarse con el contexto para que el supervisor lo use
    // Por ahora, el supervisor se encargará en su update()
}
```

### 3.4 Firmas de Funciones Clave

```
cpp
// ErrorManager API
namespace common {
    class ErrorManager {
    public:
        bool raise_error(uint16_t code, ErrorLevel level = ErrorLevel::OK);
        void clear_error(uint16_t code);
        void clear_all();
        ErrorLevel get_system_level() const;
        void update_heartbeat(Subsystem subsystem);
        bool check_watchdogs();
        uint32_t get_time_since_heartbeat(Subsystem subsystem) const;
        std::string dump_active_errors() const;
    };
}

// CanValidator API  
namespace common {
    class CanValidator {
    public:
        void register_rx(uint8_t motor_id);
        bool check_communication_health();
        CanPeerStatus get_peer_status(uint8_t motor_id) const;
        bool is_bus_healthy() const;
        std::string dump_status() const;
    };
}

// PowerManager API
namespace common {
    class PowerManager {
    public:
        PowerResult calculate_power(double accelerator, double brake,
            double soc, uint32_t battery_voltage_mv, int32_t battery_current_ma,
            const std::array<double, 4>& motor_temps);
        PowerState get_state() const;
        double get_power_limit_factor() const;
        bool is_degraded() const;
        std::string dump_status() const;
    };
}

// SystemSupervisor API (unificado)
namespace common {
    class SystemSupervisor {
    public:
        bool init();
        void update(const SystemSnapshot& snapshot);
        bool check_system_health();
        double get_power_limit_factor() const;
        ErrorLevel get_error_level() const;
        bool should_enter_safe_mode() const;
        double get_max_allowed_torque() const;
        void register_heartbeat(Subsystem subsystem);
        void register_can_message(uint8_t peer_id);
        std::string dump_status() const;
    };
}
```

---

## 4️⃣ PLAN DE VERIFICACIÓN (TESTS + FALLOS + MÉTRICAS)

### 4.1 Pruebas Unitarias / Simuladas

**Test Harness Simple (test_supervisor.cpp):**
```
cpp
// Ejecutar con: g++ -std=c++17 -o test_supervisor test_supervisor.cpp \
//   error_system.cpp can_validator.cpp power_manager.cpp system_supervisor.cpp logging.cpp

// Casos de prueba:
1. test_error_raise_clear()        - Verificar raise/clear de errores
2. test_error_level_escalation()   - Verificar escalada de niveles
3. test_watchdog_timeout()         - Simular timeout de heartbeat
4. test_power_limit_soc()          - Verificar limitación por SOC
5. test_power_limit_temp()         - Verificar limitación por temperatura
6. test_power_ramp()               - Verificar rampa de potencia
7. test_integrated_update()        - Test completo del supervisor
```

### 4.2 Pruebas en Tiempo Real por Hilo

| Hilo | Período | Verificación |
|------|---------|--------------|
| CAN_RX | 1ms | Verificar que no hay bloqueo > 1ms |
| ADC | 10ms | Verificar que no hay bloqueo > 10ms |
| CONTROL | 20ms | Verificar latencia < 5ms adicional |
| CAN_TX | 50ms | Verificar que no hay bloqueo > 50ms |
| WATCHDOG | 500ms | Verificar period accuracy |

**Script de verificación:**
```
bash
# Medir latencia de hilos
sudo ./ecu_atc8110 &
sleep 5
# Verificar con cyclictest
sudo cyclictest -t 6 -p 80 -n -i 1000 -l 10000
```

### 4.3 Pruebas de Fallos

| Escenario | Acción Esperada | Indicador de PASS |
|-----------|-----------------|-------------------|
| Desconectar motor CAN (M1) | Error crítico, torque 0 en M1 | `Errors::MOTOR_M1_COM` activo |
| BMS sin mensajes (1s) | Warning, modo degradado | `Errors::BMS_COM` activo |
| Supervisor sin heartbeat (500ms) | Emergency stop | Estado = Error |
| Mensaje corrupto | Warning, frame ignorado | `CAN_FRAME_INVALID` log |
| SOC < 10% | Limitación potencia 30% | `power_limit_factor` = 0.3 |
| Temp motor > 80°C | Limitación potencia | `limited_by_temperature` = true |

### 4.4 Logs y Contadores Esperados

**En operación normal:**
```
[INFO] SystemSupervisor: Supervisión iniciada
[INFO] ErrorSystem: Error cleared: 0x0
[INFO] PowerManager: Power limited: none factor=1.0
```

**En modo degradado:**
```
[WARN] ErrorSystem: Error raised: 0x1101 [GRAVE] BMS count=1
[WARN] PowerManager: Power limited: soc factor=0.3 available=16800W
[INFO] SystemSupervisor: Modo degradado activado
```

**En emergencia:**
```
[ERROR] Watchdog: Motor M1 heartbeat timeout: 2005ms
[ERROR] ErrorSystem: Error raised: 0x2101 [CRITICO] M1 count=1
[ERROR] SystemSupervisor: ERROR CRÍTICO - Entrando en modo seguro
```

### 4.5 Métricas a Exportar

| Métrica | Fuente | Frecuencia |
|---------|--------|------------|
| `supervisor.error_level` | SystemSupervisor | Cada ciclo |
| `supervisor.power_limit_factor` | PowerManager | Cada ciclo |
| `supervisor.max_torque_nm` | SystemSupervisor | Cada ciclo |
| `supervisor.safe_mode` | SystemSupervisor | Cada ciclo |
| `can.bus_healthy` | CanValidator | Cada check |
| `can.m1_timeouts` | CanValidator | Acumulado |
| `can.m2_timeouts` | CanValidator | Acumulado |
| `can.m3_timeouts` | CanValidator | Acumulado |
| `can.m4_timeouts` | CanValidator | Acumulado |
| `can.bms_timeouts` | CanValidator | Acumulado |
| `error.active_count` | ErrorManager | Cada ciclo |
| `error.total_count` | ErrorManager | Acumulado |
| `power.demanded_w` | PowerManager | Cada ciclo |
| `power.available_w` | PowerManager | Cada ciclo |

### 4.6 Criterios de PASS/FAIL

| Criterio | Condición PASS | Condición FAIL |
|----------|----------------|----------------|
| Compilación | ✅ Sin errores/warnings | ❌ Errores de compilación |
| Runtime inicio | ✅ Supervisor init OK | ❌ Crash al iniciar |
| Error raising | ✅ raise_error() funciona | ❌ No se registra |
| Watchdog timeout | ✅ Detecta timeout 2s | ❌ No detecta |
| Power limiting | ✅ SOC<10% → factor<0.4 | ❌ Sin limitación |
| Modo seguro | ✅ CRÍTICO → torque=0 | ❌ No entra en modo seguro |
| Latencia CONTROL | ✅ < 5ms adicional | ❌ > 5ms |
| Integración CAN | ✅ Mensajes procesan OK | ❌ Crash/timeout |

---

## 5️⃣ CHECKLIST PASS/FAIL Y CONDICIÓN PARA PRIORIDAD 2

### Checklist de Implementación PRIORIDAD 1

- [ ] **Paso 1: Módulo de Errores**
  - [ ] `error_system.hpp` creado con enum ErrorLevel, Subsystem, códigos
  - [ ] `error_system.cpp` implementado con raise/clear/get_level
  - [ ] Test unitario: raise_error() y clear_error() funcionan
  - [ ] Test unitario:escalada de niveles (LEVE→GRAVE→CRÍTICO)

- [ ] **Paso 2: Watchdogs y Heartbeats**
  - [ ] ErrorManager.check_watchdogs() implementado
  - [ ] Integración en rt_context
  - [ ] Test: timeout de supervisor genera error crítico
  - [ ] Test: timeout de motor genera error crítico

- [ ] **Paso 3: Validación CAN**
  - [ ] `can_validator.hpp` creado
  - [ ] `can_validator.cpp` implementado
  - [ ] register_rx() detecta mensajes
  - [ ] check_communication_health() detecta timeouts

- [ ] **Paso 4: Gestión de Potencia**
  - [ ] `power_manager.hpp` creado
  - [ ] `power_manager.cpp` implementado
  - [ ] calculate_power() con limitación SOC
  - [ ] calculate_power() con limitación temperatura
  - [ ] Rampa de potencia funciona

- [ ] **Paso 5: Integración**
  - [ ] `system_supervisor.hpp/cpp` creado
  - [ ] Integración en thread_control
  - [ ] Integración en thread_watchdog
  - [ ] Compilación completa sin errores
  - [ ] Ejecución sin crash

### Métricas de Verificación

| Métrica | Target | Actual |
|---------|--------|--------|
| Latencia thread_control | < 25ms (período 20ms) | ___ |
| Latencia thread_watchdog | < 550ms (período 500ms) | ___ |
| Tiempo inicio supervisor | < 100ms | ___ |
| Memoria adicional | < 50KB | ___ |

### Condición para Avanzar a PRIORIDAD 2

✅ **PASS - Avanzar a PRIORIDAD 2 si:**
1. Todos los items de checklist marcados ✅
2. Compilación sin warnings
3. Tests unitarios pasando
4. Latencia de hilos dentro de specs
5. Sistema entra en modo seguro ante error crítico

❌ **FAIL - Corregir antes de avanzar si:**
1. Cualquier item de checklist incompleto
2. Errors de compilación
3. Crash en ejecución
4. Latencia de hilos超标 (> período)
5. No entra en modo seguro correctamente

---

## ANEXO: Valores por Defecto Asumidos

Basados en el sistema legacy y best practices ECU:

```
cpp
// Timeouts
constexpr uint32_t DEFAULT_HB_TIMEOUT_MS = 500;      // Legacy: no equivalente directo
constexpr uint32_t DEFAULT_MOTOR_HB_TIMEOUT_MS = 2000; // Legacy: TIEMPO_EXP_MOT_S
constexpr uint32_t DEFAULT_BMS_TIMEOUT_MS = 1000;      // Legacy: no equivalente directo

// Límites de temperatura
constexpr int8_t DEFAULT_MOTOR_TEMP_LEVE = 60;   // Legacy: MOTOR_TEMP_ALTA_LEVE
constexpr int8_t DEFAULT_MOTOR_TEMP_GRAVE = 80;  // Legacy: MOTOR_TEMP_ALTA_GRAVE
constexpr int8_t DEFAULT_BMS_TEMP_LEVE = 50;
constexpr int8_t DEFAULT_BMS_TEMP_GRAVE = 65;

// Límites de voltaje
constexpr uint8_t DEFAULT_MOTOR_V_AUX_LOW = 4;   // Legacy: MOTOR_V_AUX_L_LEVE
constexpr uint8_t DEFAULT_MOTOR_V_AUX_HIGH = 6;  // Legacy: MOTOR_V_AUX_H_LEVE
constexpr uint8_t DEFAULT_MOTOR_V_BAT_LOW = 60;  // Legacy: MOTOR_V_BAJA_LEVE
constexpr uint8_t DEFAULT_MOTOR_V_BAT_HIGH = 90; // Legacy: MOTOR_V_ALTA_LEVE

// Potencia
constexpr double DEFAULT_MAX_POWER_W = 56000;     // Legacy: POTMAXFC
constexpr double DEFAULT_MAX_BATTERY_W = 18000;   // Legacy: POTMAXBAT

// SOC
constexpr double DEFAULT_SOC_MIN = 5.0;           // Legacy: BATT_M_B
constexpr double DEFAULT_SOC_CRITICAL = 10.0;
constexpr double DEFAULT_SOC_NORMAL = 20.0;
constexpr double DEFAULT_SOC_FULL = 95.0;          // Legacy: BATT_A_F
```

---

**Documento generado:** 2025-01-17  
**Proyecto:** ecu_atc8110  
**Objetivo:** PRIORIDAD 1 - Errores, CAN Validation, Power Management
