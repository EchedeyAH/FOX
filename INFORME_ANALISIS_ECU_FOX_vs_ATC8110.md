# INFORME DE ANÁLISIS COMPARATIVO
## ECU_FOX_rc30.14 (Legacy) vs ecu_atc8110 (Nuevo Desarrollo)

---

## 1️⃣ ARQUITECTURA COMPARADA

### Proyecto Legacy (ECU_FOX_rc30.14 - QNX Neutrino)

```
ECU_FOX_rc30/
├── proceso_ppal/           # Hilo principal ECU
│   ├── ecu_fox.c          # Main + inicialización + FSM
│   ├── err_fox.c          # Manejo de errores
│   ├── gestionpot_fox.c   # Gestión de potencia
│   ├── ctrl_trac_estab_fox.c  # Control tracción/estabilidad
│   ├── tad_fox.c          # Adquisición de datos sensores
│   ├── can1_fox.c         # CAN1 (motores)
│   ├── canRx_fox.c        # Recepción CAN
│   └── can_superv_fox.c   # Supervisor CAN
├── proceso_can2/          # Hilo CAN2 (BMS)
│   └── can2_fox.c
├── proceso_imu/           # Hilo IMU
└── script_fox/            # Scripts de despliegue
```

### Proyecto Nuevo (ecu_atc8110 - Linux RT)

```
ecu_atc8110/
├── logica_sistema/        # Hilos RT principales
│   ├── main.cpp          # Punto de entrada
│   └── rt_threads.hpp    # 6 hilos RT
├── common/               # Módulos comunes
│   ├── rt_thread.hpp     # Utilidades RT
│   ├── rt_context.hpp    # Contexto compartido
│   ├── error_catalog.hpp # Catálogo de errores
│   └── logging.hpp       # Log
├── comunicacion_can/     # Comunicación CAN
│   ├── can_manager.hpp   # Gestor CAN
│   └── can_protocol.hpp  # Protocolo
├── control_vehiculo/     # Control vehículo
│   └── traction_control.cpp
└── adquisicion_datos/    # Sensores hardware
    └── sensor_manager.hpp
```

---

## 2️⃣ DIFERENCIAS CLAVE

| Aspecto | Legacy (ECU_FOX) | Nuevo (ecu_atc8110) |
|---------|------------------|---------------------|
| **SO** | QNX Neutrino (RTOS) | Linux + PREEMPT_RT |
| **Lenguaje** | C | C++ |
| **Procesamiento** | Múltiples procesos + colas IPC | Hilos POSIX (pthread) |
| **Hardware E/S** | PCM-3718HG (ISA) | PEX-1202L + PEX-DA16 (PCIe) |
| **CAN Driver** | saj1000 + drivers QNX | SocketCAN (Linux) |
| **Control Motor** | CCP via CAN | CCP via CAN (compatible) |
| **Scheduling** | Señales RT + temporizadores | clock_nanosleep + prioridades |
| **Memoria** | Múltiples mutex (mut_*) | std::mutex + std::atomic |

---

## 3️⃣ FUNCIONALIDADES FALTANTES EN ecu_atc8110

### 🔴 CRÍTICAS (Implementadas en este análisis)

| # | Funcionalidad Legacy | Archivo Legacy | Estado en ecu_atc8110 |
|---|---------------------|----------------|----------------------|
| 1 | **Timeout detección motores (2s)** | `gestionpot_fox.c` | ✅ IMPLEMENTADO - `motor_timeout_detector.hpp` |
| 2 | **Protección voltaje batería** | `err_fox.c` | ✅ IMPLEMENTADO - `voltage_protection.hpp` |
| 3 | **Gestor modos (OK/LIMP/SAFE_STOP)** | `ecu_fox.c` FSM | ✅ IMPLEMENTADO - `system_mode_manager.hpp` |
| 4 | **Scheduler RT con jitter** | `ecu_fox.c` timers | ✅ IMPLEMENTADO - `rt_scheduler.hpp` |
| 5 | **Integración watchdog** | `err_fox.c` | ✅ IMPLEMENTADO - `rt_threads.hpp` |

### 🟡 PARCIALMENTE IMPLEMENTADAS

| # | Funcionalidad | Estado | Referencia |
|---|---------------|--------|------------|
| 1 | Control tracción | Implementado | `traction_control.cpp` |
| 2 | Gestión suspensión | Implementado | `suspension_controller.cpp` |
| 3 | Gestión batería | Parcial | `battery_manager.cpp` - falta validación completa |
| 4 | Telemetría motores | Parcial | `can_manager.hpp` - falta algunos MSG_TIPO |

### 🟢 YA IMPLEMENTADAS

| # | Funcionalidad | Archivo |
|---|---------------|---------|
| 1 | Lectura sensores (ADC) | `sensor_manager.hpp` |
| 2 | Escritura actuadores (DAC) | `pexda16.hpp` |
| 3 | Comunicación CAN | `can_manager.hpp` |
| 4 | Protocolo BMS | `can_bms_handler.hpp` |
| 5 | Inicialización motores | `can_initializer.hpp` |
| 6 | Máquina de estados ECU | `rt_threads.hpp` (thread_control) |

---

## 4️⃣ ANÁLISIS ESPECÍFICO

### 📊 CONTROL

#### Legacy (ECU_FOX)
- **FSM**: 5 estados (Inicializando → Operando → Error → Apagado)
- **Signals**: SIGRTMIN+0 a SIGRTMIN+7 para temporización
- **Threading**: 6 procesos separados con colas IPC

#### Nuevo (ecu_atc8110)
- **FSM**: Estados definidos en `EstadoEcu` enum
- **Threading**: 6 hilos POSIX con prioridades SCHED_FIFO
- **Falta**: Temporizador basado en signals (usar timer_create)

**ACCIÓN**: ✅ Implementado en `rt_scheduler.hpp`

---

### ⚙️ MOTOR

#### Legacy (ECU_FOX)
- **Gestión**: `gestionpot_fox.c` - verificación timeout + torque
- **Timeout**: `TIEMPO_EXP_MOT_S = 2s`
- **Control**: Zonas muertas (deadzones) para acel/freno
- **Protección**: Temperatura, corriente, voltaje

#### Nuevo (ecu_atc8110)
- **Gestión**: `can_manager.hpp` - envío comandos
- **Timeout**: ❌ FALTABA → ✅ Implementado (`motor_timeout_detector.hpp`)
- **Control**: `traction_control.cpp` - calcular torques
- **Protección**: ❌ FALTABA → ✅ Implementado (`voltage_protection.hpp`)

**ACCIÓN**: Módulos creados:
- `motor_timeout_detector.hpp` - Detecta timeout 2s
- `voltage_protection.hpp` - Protección Vbat

---

### ⚠️ MANEJO DE ERRORES

#### Legacy (ECU_FOX)
- **Códigos**: Definidos en `constantes_fox.h`
- **Sistema**: Proceso `err_fox.c` con cola de errores
- **Tipos**: WARN, GRAVE, CRITICO
- **Acciones**: LIMP_MODE, SAFE_STOP

#### Nuevo (ecu_atc8110)
- **Códigos**: `error_catalog.hpp` (ya definido)
- **Sistema**: `error_publisher.hpp` (HMI + logging)
- **Tipos**: ErrorLevel enum (INFO, WARN, GRAVE, CRITICO)
- **Acciones**: ❌ FALTABA gestor central → ✅ `system_mode_manager.hpp`

**ACCIÓN**: `system_mode_manager.hpp` implementa:
- Transiciones OK → LIMP_MODE → SAFE_STOP
- Limitación de potencia por factor
- Control de torque 0 en SAFE_STOP

---

## 5️⃣ RIESGOS DEL SISTEMA ACTUAL

### 🔴 RIESGOS CRÍTICOS

| # | Riesgo | Impacto | Mitigación |
|---|--------|---------|------------|
| 1 | **Sin timeout de motores** | Motor colgado no detectado | ✅ Implementado |
| 2 | **Sin protección Vbat** | Daño batería/motores | ✅ Implementado |
| 3 | **Sin gestión de modos** | No hay respuesta a errores | ✅ Implementado |

### 🟡 RIESGOS MEDIOS

| # | Riesgo | Impacto | Mitigación |
|---|--------|---------|------------|
| 1 | **Jitter no medido** | Control puede ser inestable | ✅ Scheduler con métricas |
| 2 | **Fallback sin privilegios RT** | Hilos pueden perder prioridad | Documentado en rt_thread.hpp |
| 3 | **Integración incompleta** | Módulos no conectados | ✅ rt_threads.hpp actualizado |

### 🟢 RIESGOS BAJOS

| # | Riesgo | Impacto |
|---|--------|---------|
| 1 | Cross-compilación Windows→Linux | Build en Linux |
| 2 | Dependencias no resueltas | CMake gestionará |

---

## 6️⃣ RECOMENDACIONES DE IMPLEMENTACIÓN

### Inmediatas (Completado en este análisis)

1. **Integrar módulos de seguridad**:
   - `motor_timeout_detector.hpp` - Timeout motores 2s
   - `voltage_protection.hpp` - Protección batería
   - `system_mode_manager.hpp` - Gestión modos

2. **Conectar en rt_threads.hpp**:
   - CAN_RX → Notificar timeout
   - Watchdog → Verificar voltage + timeout + modo

### Corto plazo

3. **Completar validación**:
   - Verificar todos los MSG_TIPO de motores
   - Integrar SOC (State of Charge) en protection
   - Integrar temperatura motor

4. **Testing**:
   - Unit tests para cada módulo
   - Integration tests con CAN emulado

### Largo plazo

5. **Mejoras**:
   - Implementar watchdog hardware
   - Agregar diagnostique (DIAG) button
   - Persistencia de errores en SD card

---

## 7️⃣ ARCHIVOS CREADOS/MODIFICADOS

### Nuevos archivos

| Archivo | Descripción |
|---------|-------------|
| `common/rt_scheduler.hpp` | Scheduler RT con métricas jitter |
| `common/motor_timeout_detector.hpp` | Detector timeout motores 2s |
| `common/voltage_protection.hpp` | Protección voltaje batería |
| `common/system_mode_manager.hpp` | Gestor modos sistema |

### Archivos modificados

| Archivo | Cambio |
|---------|--------|
| `common/rt_context.hpp` | Agregar referencias a módulos seguridad |
| `logica_sistema/rt_threads.hpp` | Integrar safety checks en CAN_RX y Watchdog |

---

## 8️⃣ CONCLUSIONES

### Estado actual

El proyecto `ecu_atc8110` tiene la **arquitectura base completa** pero **carece de los mecanismos de seguridad críticos** que hacen al sistema robusto y seguro para uso automotriz.

### Valor agregado

Con los módulos implementados, el nuevo sistema ahora cuenta con:

✅ **Timeout de motores** - Detecta falta de respuesta en 2s  
✅ **Protección de batería** - Monitoreo Vbat con umbrales  
✅ **Gestión de modos** - Transiciones OK → LIMP → SAFE_STOP  
✅ **Integración watchdog** - Verificación periódica de salud  

### Siguientes pasos

1. Compilar y probar en target Linux
2. Verificar timing de hilos
3. Testing de integración con hardware real
4. Documentar configuraciones

---

**Fecha**: 2025  
**Analista**: BLACKBOX AI - Ingeniero Senior Firmware Automotriz
