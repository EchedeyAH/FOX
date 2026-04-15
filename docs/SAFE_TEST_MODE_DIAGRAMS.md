# 🎯 Safe Test Mode - Diagramas Técnicos

## 1️⃣ MÁQUINA DE ESTADOS (FSM)

```
                    ┌─────────────────────────────────────┐
                    │         ESTADO INICIAL              │
                    │          [IDLE]                     │
                    │  (Motor deshabilitado)              │
                    │  throttle_target = 0.0              │
                    │  current_voltage = 0.0              │
                    └──────────────┬──────────────────────┘
                                   │
                    condición: brake >= 0.2 (FRENO PRESIONADO)
                                   │
                                   ▼
                    ┌──────────────────────────────────┐
                    │         ARMED STATE              │
                    │  (Secuencia de arranque)         │
                    │  stable_time_s += dt             │
                    │  requiere: 2.0 segundos          │
                    │  waitfor: freno presionado       │
                    └──────────┬───────────────────────┘
                               │
                    condición: stable_time >= 2.0s
                               │
                               ▼
            ┌──────────────────────────────────────┐
            │         RUNNING STATE                │
            │  (Motor en marcha)                   │
            │  throttle_target = throttle_in       │
            │  sigue acelerador                    │
            │  state puede cambiar si freno        │
            └──────┬───────────────────────────────┘
                   │
    ← CUALQUIER MOMENTO si brake >= 0.2
                   │
                   ▼
    ┌──────────────────────────────────┐
    │     SAFE_STOP STATE              │
    │  (Parada suave - CRÍTICOS)       │
    │  throttle_target = 0.0           │
    │  waiting: brake < (0.2 × 0.5)    │
    │  para retornar a IDLE            │
    └──────┬───────────────────────────┘
           │
    (si freno se suelta)
           │
           ▼
    vuelve a IDLE


    ┌──────────────────────────────┐
    │  EMERGENCY_STOP (Terminal)   │
    │  Solo por crítico (raro)     │
    │  Requiere reset() manual      │
    └──────────────────────────────┘
```

---

## 2️⃣ FLUJO DE PROCESAMIENTO (10 Capas)

```
┌─── INPUT: throttle_raw (0-1), brake_raw (0-1) ───┐
│                                                    │
├─ 1️⃣ VALIDACIÓN DE ENTRADA ────────────────────┐ │
│  ├─ ¿ADC OK?                                   │ │
│  ├─ ¿CAN OK?                                   │ │
│  └─ ¿Rango válido?                             │ │
│      → SÍ: continuar | NO: SAFE_STOP           │ │
│                                                 │ │
├─ 2️⃣ WATCHDOG TEMPORAL (200ms) ─────────────┐ │
│  ├─ time_since_last_update += dt            │ │
│  └─ if time > 200ms → throttle = 0           │ │
│                                              │ │
├─ 3️⃣ FAILSAFE DE FRENO ⚡ ────────────────┐ │
│  └─ if brake >= 0.2 → SAFE_STOP            │ │
│     (MÁXIMA PRIORIDAD)                      │ │
│                                              │ │
├─ 4️⃣ DEADZONE (Ruido) ──────────────────────┐ │
│  └─ if |throttle| < 0.05 → throttle = 0    │ │
│                                              │ │
├─ 5️⃣ MÁQUINA DE ESTADOS ────────────────────┐ │
│  └─ IDLE → ARMED (2s) → RUNNING → SAFE_STOP│ │
│                                              │ │
├─ 6️⃣ RAMPA DE THROTTLE (0.5 V/s) ──────────┐ │
│  ├─ delta = throttle_target - throttle_cur │ │
│  ├─ max_step = 0.5 × dt                     │ │
│  └─ throttle_current += clamp(delta)        │ │
│                                              │ │
├─ 7️⃣ CONVERSIÓN A VOLTAJE ──────────────────┐ │
│  └─ voltage = throttle × MAX_VOLTAGE_SAFE   │ │
│     (2.0V máximo)                           │ │
│                                              │ │
├─ 8️⃣ RAMPA DE VOLTAJE (0.5 V/s) ──────────┐ │
│  ├─ delta = target_voltage - current_V     │ │
│  ├─ max_step = 0.5 × dt                     │ │
│  └─ current_voltage += clamp(delta)         │ │
│                                              │ │
├─ 9️⃣ LÍMITE ANALÓGICA [0, 2.0V] ──────────┐ │
│  └─ if voltage > 2.0 → voltage = 2.0        │ │
│                                              │ │
├─ 🔟 LOGGING PROFESIONAL ───────────────────┐ │
│  └─ printf estado + voltaje + fallos        │ │
│                                              │ │
└─ OUTPUT: current_voltage (0-2.0V) ────────────┘
    ↓
    Escritura en DAC (PEX-DA16)
```

---

## 3️⃣ ESTRUCTURA DE DATOS

```
┌─────────────────────────────────────────────────────────┐
│                    SafeMotorTest                        │
│                     (134 bytes)                         │
├─────────────────────────────────────────────────────────┤
│ STATE:                                                  │
│  ├─ enum MotorState state                             │
│  ├─ enum SafeFailureReason last_failure               │
│  └─ int fault_count, cycle_count                       │
│                                                        │
│ CONTROL VARIABLES:                                      │
│  ├─ double current_voltage (0-2.0V)                    │
│  ├─ double target_voltage (rampa destino)             │
│  ├─ double throttle_current (rampa actual)            │
│  ├─ double throttle_target (destino)                  │
│  └─ double stable_time_s (contador armado)            │
│                                                        │
│ TIMERS:                                                 │
│  └─ double time_since_last_throttle (watchdog)         │
│                                                        │
│ CONFIGURATION (ajustables):                             │
│  ├─ double max_safe_voltage (2.0 default)             │
│  ├─ double max_torque_nm (15.0 default)               │
│  ├─ double ramp_rate_v_per_s (0.5 default)            │
│  ├─ double ramp_rate_throttle_per_s (0.5)             │
│  ├─ double deadzone (0.05 default)                    │
│  ├─ double stable_required_s (2.0 default)            │
│  ├─ double brake_pressed_threshold (0.2)              │
│  ├─ double watchdog_timeout_s (0.2 default)           │
│  ├─ double throttle_sensor_min_v (0.2 default)        │
│  ├─ double throttle_sensor_max_v (4.8 default)        │
│  └─ double max_analog_output (2.0 default)            │
│                                                        │
│ MONITORING:                                             │
│  ├─ int log_cycle_interval (50 default)               │
│  └─ [internals para control]                          │
└─────────────────────────────────────────────────────────┘
```

---

## 4️⃣ TIMELINE DE UN CICLO TÍPICO @ 20Hz (50ms)

```
t = 0.00s: Usuario presiona FRENO
────────────────────────────────
[SAFE_TEST] state=IDLE → ARMED
  brake=0.5, throttle=0.0, volt=0.0

t = 0.05s → 2.00s: Esperando ventana ARMED (2s)
─────────────────────────────────────────────────
[SAFE_TEST] state=ARMED (stable_time=0.05s...1.95s)
  brake=0.5, stable_time=0.10s, stable_req=2.0s

t = 2.00s: Usuario suelta FRENO
──────────────────────────────
[SAFE_TEST] state=ARMED → RUNNING
  brake=0.0, stable_time=2.0s ✅

t = 2.05s → 4.00s: Usuario acelera gradualmente
──────────────────────────────────────────────
Input: throttle_raw = (t - 2.0) / 2.0  (rampa 0→1 en 2s)

t = 2.05s (throttle_in=0.025):
  ├─ 1️⃣ Validar: ✅
  ├─ 2️⃣ Watchdog: 0.05ms ✅
  ├─ 3️⃣ Freno: No presionado ✅
  ├─ 4️⃣ Deadzone: pequeño, mantener
  ├─ 5️⃣ FSM: RUNNING ✅
  ├─ 6️⃣ Rampa throttle: 0.0 → 0.025 (0.5 V/s × 0.05s = 0.025)
  ├─ 7️⃣ Conv V: 0.025 × 2.0 = 0.05V
  ├─ 8️⃣ Rampa V: 0.0 → 0.05V (rampa suave)
  ├─ 9️⃣ Límite: 0.05 < 2.0 ✅
  └─ 🔟 Log: [SAFE_TEST] state=RUNNING thr=0.03 volt=0.05

t = 4.00s (throttle_in=1.0):
  [SAFE_TEST] state=RUNNING thr=1.00 volt=2.00  ✅ MAX ALCANZADO

t = 4.05s: Usuario presiona FRENO
──────────────────────────────────
[SAFE_TEST] state=RUNNING → SAFE_STOP
  ├─ Failsafe: ACTIVO
  ├─ Parada: < 50ms
  └─ throttle_target = 0.0

t = 4.10s → 4.30s: Rampa DOWN (desaceleración suave)
────────────────────────────────────────────────────
[SAFE_TEST] state=SAFE_STOP
  voltage: 2.0V → 1.9V → 1.8V ... → 0.0V
  (con rampa 0.5 V/s, tarda ~4 segundos desaceleración)

t = 4.30s: Usuario suelta FRENO completamente
───────────────────────────────────────────────
[SAFE_TEST] state=SAFE_STOP → IDLE
  brake < 0.1 ✅ (transición segura)
```

---

## 5️⃣ MATRIZ DE TRANSICIONES FSM

```
┌─────┬────────┬────────┬────────┬──────────┬───────────┐
│FROM │  IDLE  │ ARMED  │RUNNING │SAFE_STOP │EMERGENCY  │
├─────┼────────┼────────┼────────┼──────────┼───────────┤
│IDLE │   -    │  ✓👇   │   -    │    -     │     -     │
│     │        │FRENO   │        │          │           │
│     │        │2.0s    │        │          │           │
├─────┼────────┼────────┼────────┼──────────┼───────────┤
│ARM  │  ✓👇   │   -    │  ✓👇   │    -     │     -     │
│     │NO FRENO│        │2.0s    │          │           │
│     │        │        │ESTABLE │          │           │
├─────┼────────┼────────┼────────┼──────────┼───────────┤
│RUN  │    -   │   -    │   -    │  ✓👇     │  ✓👇      │
│     │        │        │        │FRENO     │FALLO      │
├─────┼────────┼────────┼────────┼──────────┼───────────┤
│SAFE │  ✓👇   │   -    │   -    │    -     │     -     │
│STOP │NO FRENO│        │        │          │           │
├─────┼────────┼────────┼────────┼──────────┼───────────┤
│EMERG│   -    │   -    │   -    │    -     │  RESET    │
│ NO  │        │        │        │          │MANUAL     │
└─────┴────────┴────────┴────────┴──────────┴───────────┘

Nota: ✓ = transición válida
      👇 = condición requerida
```

---

## 6️⃣ RANGOS DE VOLTAJE/THROTTLE

```
SENSOR ENTRADA (Throttle Pedal):
┌─────────────────────────────────────────────────┐
│  0.0V ──── 0.2V ──── 2.5V ──── 4.8V ──── 5.0V  │
│  └─ MIN   │ VÁLIDO │ MID  │ VÁLIDO │  RECHAZO─┘
│  RECHAZO  │ RANGE  │ RANGE│ RANGE  │
│           │ 0.2-4.8│      │ 0.2-4.8│
└─────────────────────────────────────────────────┘

THROTTLE NORMALIZADO (0-1):
┌─────────────────────────────────────────────────┐
│  0.0 ─── 0.05 ─── 0.5 ─── 1.0 ─── 1.x         │
│         │ DEAD  │ MID │ MAX │ OUT OF RANGE    │
│  REPOSO │ ZONE  │ RANGE      │ (rechazado)     │
│         │       │            │                 │
└─────────────────────────────────────────────────┘

VOLTAJE SALIDA (DAC):
┌─────────────────────────────────────────────────┐
│  0.0V ────── 1.0V ────── 2.0V ────── (5.0V)   │
│   OFF    │   MID OUTPUT    │   MAX SAFE
│         │ RANGE            │   LIMIT
│         │                  │   [CLAMPED]
│         │                  │
└─────────────────────────────────────────────────┘

TORQUE EQUIVALENTE (a través de conversión):
┌─────────────────────────────────────────────────┐
│  0.0 Nm ────── 7.5Nm ─────── 15.0Nm ──────┘
│  (OFF)   │  HALF POWER   │   MAX SAFE    │
│          │               │   LIMIT       │
│          │               │  [CLAMPED]    │
│          │               │               │
└─────────────────────────────────────────────────┘
```

---

## 7️⃣ DIAGRAMA DE FALLOS (SafeFailureReason)

```
┌─────────────────────────────────────────────────┐
│         SafeFailureReason (7 tipos)             │
├─────────────────────────────────────────────────┤
│                                                 │
│ NONE = 0                (Sin fallo)             │
│ ADC_FAULT              (Sensor roto)            │
│ CAN_FAULT              (Red caída)              │
│ SENSOR_OUT_OF_RANGE    (Rango inválido)        │
│ TIMEOUT                (Sin datos > 200ms)     │
│ BRAKE_PRESSED          (Freno activado)        │
│ UNKNOWN                (Fallo indeterminado)   │
│                                                 │
├─────────────────────────────────────────────────┤
│ MANEJO:                                         │
│  ├─ Contar en fault_count++                    │
│  ├─ Guardar en last_failure                    │
│  ├─ Loguear con printf                          │
│  ├─ Transición a SAFE_STOP                     │
│  └─ (en críticos) → EMERGENCY_STOP             │
└─────────────────────────────────────────────────┘
```

---

## 8️⃣ TIMING REAL-TIME (SCHED_FIFO)

```
┌─────────────────────────────────────────────────────────┐
│                  Thread Timeline @ 20Hz                 │
│                   (Period = 50ms)                       │
├─────────────────────────────────────────────────────────┤
│                                                         │
│ Cycle 0:   |───── 50ms ───────|                        │
│            ├─ Lectura entrada (1ms)                    │
│            ├─ 10 capas (15ms)                          │
│            ├─ Escritura DAC (1ms)                      │
│            └─ Slack: ~33ms (plenty)                    │
│                                                         │
│ Cycle 1:   |───── 50ms ───────|                        │
│            └─ (repeat)                                 │
│                                                         │
│ Jitter permitido: < 10ms (para FIFO 20Hz)             │
│ Deadline:        50ms                                  │
│ Worst case:      ~20ms (10 capas + I/O)               │
│ Margin:          30ms ✅                               │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

---

## 9️⃣ EJEMPLO DE RESPUESTA A FAILSAFE

```
t = 0.00s:   θ = 0.50 (acelerando)
             b = 0.00 (freno NO presionado)
             state = RUNNING ✅

t = 0.05s:   [Normal cycle]

...

t = 1.20s:   usuario PRESIONA FRENO
             θ = 0.50 (no cambió aún, pero será sobrescrito)
             b = 0.95 (freno presionado)
             
             FAILSAFE ACTIVA:
             ├─ if brake_raw >= 0.2 → TRUE
             ├─ Cambio: state = SAFE_STOP
             ├─ Cambio: throttle_target = 0.0
             ├─ Cambio: throttle_raw = 0.0
             └─ Tiempo de respuesta: < 1ms ✅

t = 1.25s:   Rampa DOWN comienza
             voltage: 1.00V → 0.99V → 0.98V ...
             (disminuye 0.5 V/s = -0.025V cada 50ms)

t = 3.25s:   voltage ≈ 0.0V
             Motor completamente parado

RESUMEN:
├─ Detección: < 1ms
├─ Rampa parada: ~2 segundos (suave, sin shock)
└─ Riesgo eliminado ✅
```

---

## 🔟 GRÁFICA DE RESPUESTA TÍPICA

```
ESCALÓN DE ACELERACIÓN (con rampa):

Throttle Input:    │
(Paso Brusco)      1.0 ┌────────────────────
                   0.9 │    ◆
                   0.8 │    ◆◆
                   0.7 │    ◆◆◆
   Sin Rampa ★     0.6 │    ◆◆◆◆
                   0.5 │  ★◆◆◆◆◆  ← Rampa suave
                   0.4 │  ★  ◆◆◆◆◆◆◆
                   0.3 │  ★  ◆◆◆◆◆◆◆◆◆
                   0.2 │  ★      ◆◆◆◆◆◆◆◆◆
                   0.1 │  ★          ◆◆◆◆◆◆◆
                   0.0 └───────────────────────
                        0    1    2    3    4 (segundos)

WITHOUT RAMP (★): Aceleración brusca en 50ms
                  → Tirones, riesgo físico, posible volcamiento

WITH RAMP (◆):   Aceleración suave en 2 segundos
                  → Movimiento controlado, seguro, cómodo
```

---

## 🎯 MAPA DE DECISIONES (Decision Tree)

```
                        [START UPDATE]
                             │
                             ▼
                    ┌─────────────────┐
                    │ ADC OK & CAN OK?│
                    └─────┬─────┬─────┘
                        ✓ │     │ ✗
                          │     └────→ SAFE_STOP
                          ▼
                    ┌──────────────┐
                    │ Rango válido?│
                    └─────┬────┬──┘
                        ✓ │    │ ✗
                          │    └────→ SAFE_STOP
                          ▼
                    ┌──────────────────┐
                    │ Watchdog < 200ms?│
                    └─────┬────────┬──┘
                        ✓ │        │ ✗
                          │        └────→ throttle = 0
                          ▼
                    ┌──────────────┐
                    │ Brake >= 0.2?│
                    └─────┬────┬──┘
                        ✓ │    │ ✗
                        ──┘    │
              SAFE_STOP ◄──────┘
                  │
                  ▼
              ┌─────────────┐
              │ FSM Update  │
              │ (5 estados) │
              └────┬────────┘
                   ▼
            ┌──────────────┐
            │ Rampas (2)   │
            │ Throttle     │
            │ + Voltage    │
            └────┬─────────┘
                 ▼
          ┌──────────────┐
          │ Limit Check  │
          │ voltage ≤2.0V
          └────┬────────┘
               ▼
         [OUTPUT & LOG]
```

---

**Documentación técnica completa de Safe Test Mode v1.0** ✅
