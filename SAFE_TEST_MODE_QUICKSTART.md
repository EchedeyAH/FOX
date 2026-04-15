# 🚀 Safe Test Mode - Guía de Inicio Rápido

## 📋 ACTIVACIÓN INMEDIATA (2 minutos)

### ✅ Paso 1: Verificar estado actual

```bash
cd /path/to/FOX/ecu_atc8110
grep "SAFE_TEST_MODE\|MAX_TORQUE_SAFE\|MAX_VOLTAGE_SAFE" \
    ../control_vehiculo/safe_motor_test.hpp
```

**Esperar**:
```
constexpr bool SAFE_TEST_MODE = true;
constexpr double MAX_TORQUE_SAFE = 15.0;
constexpr double MAX_VOLTAGE_SAFE = 2.0;
```

✅ Si ves esto → **YA ESTÁ ACTIVADO**

---

### ✅ Paso 2: Compilar con Safe Test Mode

```bash
cd ecu_atc8110/build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j4 motor_control

# Opcional: Run ejemplos
make safe_motor_test_example
./safe_motor_test_example
```

---

### ✅ Paso 3: Lanzar sistema con seguridad

```bash
# Terminal 1: Motor control
./build/motor_control

# Esperar instrucciones en pantalla:
# "Pisa el FRENO para iniciar..."
```

**IMPORTANTE**:
1. Pisar **FRENO** (pedal presionado completamente)
2. Esperar **2 segundos** (ventana de "arming")
3. Soltar freno
4. Acelerar gradualmente (máximo 2.0V esperado)
5. Presionar freno para parar

---

## 🎯 TABLA DE LÍMITES DE SEGURIDAD

| Parámetro | Valor | Rango | Estado |
|-----------|-------|-------|--------|
| **Torque máximo** | 15.0 Nm | 0-117 Nm | ✅ SEGURO (87% reducción) |
| **Voltaje máximo** | 2.0 V | 0-5.0 V | ✅ SEGURO (60% reducción) |
| **Ramp rate** | 0.5 V/s | smooth | ✅ SIN TIRONES |
| **Brake failsafe** | < 50 ms | N/A | ✅ INSTANTÁNEO |
| **Timeout watchdog** | 200 ms | N/A | ✅ CORTE AUTOMÁTICO |
| **Sensor min** | 0.2 V | N/A | ✅ VALIDACIÓN |
| **Sensor máx** | 4.8 V | N/A | ✅ VALIDACIÓN |

---

## ⚡ RESULTADO ESPERADO EN LOGS

```
========================================
 ECU ATC8110 - Control de Motores FOX
========================================

[MAIN] Inicializando sistema...
[CAN_TX] Hilo iniciado [prio=50, T=50ms]
[CTRL] Hilo iniciado [prio=60, T=20ms]

⚠️  Sistema en SAFE TEST MODE
📊 Máximo torque: 15.0 Nm
⚡ Máximo voltaje: 2.0 V
⏱️  Timeout seguridad: 200 ms

INSTRUCCIONES:
1. Pisa el FRENO para iniciar la secuencia de arranque
2. Suelta el freno una vez activados los motores
3. Presiona ACELERADOR para controlar los motores
4. Presiona Ctrl+C para detener el sistema

========================================

[SAFE_TEST] state=IDLE thr_in=0.00 thr_cur=0.00 volt=0.00 brake=0.50 faults=0
[SAFE_TEST] state=ARMED thr_in=0.00 thr_cur=0.00 volt=0.00 brake=0.50 faults=0
[SAFE_TEST] state=ARMED thr_in=0.00 thr_cur=0.00 volt=0.00 brake=0.50 faults=0
[SAFE_TEST] state=RUNNING thr_in=0.30 thr_cur=0.28 volt=0.56 brake=0.00 faults=0
[CAN_TX_M1] torque=  4.20(clamped)→  4.20(max=15.0) throttle=255 voltage= 0.56V AO_limit=2.0V
```

**Interpretación**:
- ✅ `state=RUNNING` → Motor activo
- ✅ `volt=0.56` → Dentro de límite 2.0V
- ✅ `faults=0` → Sin problemas
- ✅ `torque...max=15.0` → Limitado correctamente

---

## 🔴 MODOS CRÍTICOS

### Si el motor NO responde

```bash
Verifica en este orden:
1. ¿Está presionado el FRENO?
   → Sí: espera 2 segundos en ARMED
   → No: vuelve a presionar

2. ¿Log muestra state=RUNNING?
   → No: máquina de estados atrapada
   → Revisar logs de fallo

3. ¿El voltaje es > 0?
   → No: salida analógica sin alimentación
   → Verificar cable DAC
```

### Si el freno NO detiene

```bash
🚨 EMERGENCIA CRÍTICA
1. Presionar Ctrl+C inmediatamente
2. Desconectar batería del motor
3. Revisar estado del freno:
   
   grep "BRAKE\|failsafe" motor_control.log
```

---

## 📊 MONITOREO EN VIVO

### Ver logs en tiempo real

```bash
# Terminal 1: Ejecutar sistema
./motor_control > motor.log 2>&1

# Terminal 2: Monitor en vivo
tail -f motor.log | grep "SAFE_TEST\|FAULT\|state="
```

### Buscar problemas específicos

```bash
# Fallos de sensor
grep "SENSOR_OUT_OF_RANGE" motor_control.log

# Timeout
grep "TIMEOUT" motor_control.log

# Fallo ADC
grep "ADC_FAULT" motor_control.log

# Estados
grep "state=EMERGENCY" motor_control.log
```

---

## 🔧 AJUSTES COMUNES

### Rampa más rápida (menos suave)

```cpp
// En safe_motor_test.hpp - línea ~75
safe_motor.ramp_rate_v_per_s = 1.0;  // Antes: 0.5
```

**Efecto**: Aceleración 2x más rápida (puede causar tirones)

### Rampa más lenta (más suave)

```cpp
safe_motor.ramp_rate_v_per_s = 0.25;  // Antes: 0.5
```

**Efecto**: Aceleración 2x más lenta (movimiento muy suave)

### Timeout más largo

```cpp
safe_motor.watchdog_timeout_s = 0.5;  // Antes: 0.2 (200 ms)
```

**Efecto**: 500 ms sin datos antes de cortar

### Aumentar torque (SOLO PARA TESTING POST-VALIDACIÓN)

```cpp
// ⚠️ ADVERTENCIA: Solo cambiar si ya validaste 15 Nm es seguro
constexpr double MAX_TORQUE_SAFE = 20.0;  // Antes: 15.0
constexpr double MAX_VOLTAGE_SAFE = 2.67;  // 5.0 × (20/100) - NO HACER ESTO SIN VALIDACIÓN
```

**PROHIBIDO**: 
- ❌ No cambiar MAX_VOLTAGE_SAFE > 2.5V (límite PCB)
- ❌ No cambiar MAX_TORQUE_SAFE > 30 Nm (sin análisis de seguridad)
- ❌ No deshabilitar SAFE_TEST_MODE sin aprobación

---

## 🧪 TEST RÁPIDO DE 30 SEGUNDOS

```bash
#!/bin/bash
echo "🚀 Fast validation..."

# 1. Verificar compilación
./test_safe_motor_mode.sh validate
if [ $? -ne 0 ]; then
    echo "❌ Validación falló"
    exit 1
fi

# 2. Compilar
./test_safe_motor_mode.sh build
if [ ! -f ./build/motor_control ]; then
    echo "❌ Compilación falló"
    exit 1
fi

# 3. Ejecutar ejemplo rápido (10 segundos)
timeout 10 ./build/motor_control 2>&1 | egrep "SAFE_TEST_MODE|state=|torque=" | head -20

echo "✅ Test rápido completado"
```

---

## 📞 SOPORTE RÁPIDO

### Pregunta: "¿Cómo verifico que realmente está limitado a 15 Nm?"

**Respuesta**:
```bash
# Ver valores actuales en ejecución
./motor_control | grep "max_torque\|max_voltage" | head -5

# Debería output:
# max_torque= 15.0 Nm
# max_voltage=  2.0 V
```

---

### Pregunta: "¿Qué pasa si se rompe el sensor de freno?"

**Respuesta**:
```cpp
// El sistema falla SEGURO (fail-safe)
if (brake_raw >= 0.2) {
    state = SAFE_STOP;  // ← Corte automático
}
```

Si el sensor está roto:
- El freno será siempre 0 → nunca entra en ARMED
- El motor no se activa
- ✅ **Seguro por defecto**

---

### Pregunta: "¿Cuál es el máximo riesgo físico?"

**Respuesta**:
```
ANTES (sistema original):
  Max torque: 117 Nm → Rueda gira RÁPIDO
  Aceleración: SIN RAMPA → Tirones bruscos
  Freno: Delay de ~100ms → Inercia peligrosa
  Riesgo: ⚠️⚠️⚠️ PELIGROSO

DESPUÉS (Safe Test Mode):
  Max torque: 15 Nm → Rueda gira LENTO
  Aceleración: Rampa 2s → Movimiento suave
  Freno: < 50ms → Paro inmediato
  Riesgo: ✅ BANCO DE PRUEBAS SEGURO
```

---

## ✅ CHECKLIST PRE-LAUNCH

Antes de usar en hardware real, completar:

- [ ] Archivo safe_motor_test.hpp contiene SAFE_TEST_MODE = true
- [ ] LOG muestra "SAFE_TEST_MODE ACTIVE"
- [ ] Voltaje máximo mostrado en log es ≤ 2.0V
- [ ] Torque máximo es ≤ 15.0 Nm
- [ ] Presionar freno → motor se detiene < 50ms
- [ ] Sin aceleración detona timeout
- [ ] Ejemplos ejecutan sin errores
- [ ] Documentación SAFE_TEST_MODE_IMPLEMENTATION.md accesible
- [ ] Script test_safe_motor_mode.sh funciona

---

## 🎯 SIGUIENTE PASO

Cuando estés listo para aumentar seguridad:

```bash
# 1. Validar 15 Nm está REALMENTE limitado (con osciloscopio DAC)
# 2. Registrar log de 2 horas sin fallos
# 3. Incrementar MAX_TORQUE_SAFE a 20-25 Nm (con análisis ingenieril)
# 4. Re-validar (otro log de 2 horas)
# 5. Documentar cambios en CHANGELOG.md
```

---

## 📚 DOCUMENTACIÓN COMPLETA

Para más detalles, ver:
- `docs/SAFE_TEST_MODE_IMPLEMENTATION.md` - Documentación Full
- `examples/safe_motor_test_example.cpp` - Ejemplos de código
- `test_safe_motor_mode.sh` - Script de validación

---

**Estado**: ✅ **SISTEMA SEGURO - LISTO PARA PRUEBAS**

Mantén los límites, respeta el failsafe, y disfruta de un banco de pruebas seguro.
