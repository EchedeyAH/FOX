# PLAN DE PRUEBAS DE INTEGRACIÓN
## Sistema ECU ATC8110 - Supervisión y Seguridad

---

## 1. INTRODUCCIÓN

### 1.1 Propósito
Este documento define el plan de pruebas de integración para el sistema de supervisión y seguridad del ECU ATC8110.

### 1.2 Alcance
Pruebas de integración de los módulos:
- SystemModeManager
- SystemSupervisor  
- MotorTimeoutDetector
- VoltageProtection
- TelemetryPublisher
- CanSimulator

### 1.3 Criterios de Éxito
- Todas las pruebas PASS/FAIL definidas
- Cobertura de casos de borde
- Reproducibilidad de resultados

---

## 2. CASOS DE PRUEBA

### 2.1 Motor Timeout → SAFE_STOP

| ID | Descripción | Condición Inicial | Acción | Resultado Esperado | Criterio PASS |
|----|-------------|------------------|--------|-------------------|---------------|
| MT-01 | Timeout motor M1 | Sistema OK, sin mensajes CAN M1 | Esperar >2s | Modo → SAFE_STOP | mode == SAFE_STOP |
| MT-02 | Timeout todos los motores | Sistema OK | Esperar >2s sin mensajes | Modo → SAFE_STOP | mode == SAFE_STOP |
| MT-03 | Recovery después de timeout | SAFE_STOP por timeout | Enviar mensajes CAN | Modo → OK | mode == OK después de recovery |
| MT-04 | Timeout parcial (1 de 4) | Sistema OK, solo M1 timeout | Esperar >2s | Warning M1, modo OK | M1 timeout, otros OK |

**Procedimiento MT-01:**
```
1. Iniciar sistema en modo OK
2. Deshabilitar generación de mensajes M1 en simulador
3. Esperar 2100ms
4. Verificar: mode == SAFE_STOP
5. Verificar: torque_zero == true
6. PASS/FAIL
```

---

### 2.2 Voltaje Bajo → LIMP_MODE

| ID | Descripción | Condición Inicial | Acción | Resultado Esperado | Criterio PASS |
|----|-------------|------------------|--------|-------------------|---------------|
| VB-01 | Voltaje bajo crítico | Sistema OK | Set Vbat = 45V | Modo → SAFE_STOP | mode == SAFE_STOP |
| VB-02 | Voltaje bajo warning | Sistema OK | Set Vbat = 55V | Modo → LIMP_MODE | mode == LIMP_MODE, factor = 0.3 |
| VB-03 | Recovery voltaje | LIMP por voltaje | Set Vbat = 70V | Modo → OK | mode == OK, factor = 1.0 |
| VB-04 | Histéresis | LIMP, Vbat=55V | Set Vbat=58V | Mantener LIMP | mode == LIMP_MODE (no recovery) |

**Procedimiento VB-01:**
```
1. Iniciar sistema en modo OK
2. Configurar simulador: voltaje = 45000mV
3. Esperar 500ms (debounce)
4. Verificar: mode == SAFE_STOP
5. Verificar: power_factor == 0.0
6. PASS/FAIL
```

---

### 2.3 SOC Crítico → SAFE_STOP

| ID | Descripción | Condición Inicial | Acción | Resultado Esperado | Criterio PASS |
|----|-------------|------------------|--------|-------------------|---------------|
| SC-01 | SOC crítico | Sistema OK | Set SOC = 5% | Modo → SAFE_STOP | mode == SAFE_STOP |
| SC-02 | SOC bajo warning | Sistema OK | Set SOC = 15% | Modo → LIMP_MODE | mode == LIMP_MODE, factor = 0.3 |
| SC-03 | Recovery SOC | LIMP por SOC | Set SOC = 25% | Modo → OK | mode == OK después de hystéresis |

---

### 2.4 Temperatura Alta → LIMP_MODE

| ID | Descripción | Condición Inicial | Acción | Resultado Esperado | Criterio PASS |
|----|-------------|------------------|--------|-------------------|---------------|
| TH-01 | Temp motor crítica | Sistema OK | Set temp M1 = 105°C | Modo → SAFE_STOP | mode == SAFE_STOP |
| TH-02 | Temp motor warning | Sistema OK | Set temp M1 = 85°C | Modo → LIMP_MODE | mode == LIMP_MODE, factor = 0.5 |
| TH-03 | Temp batería crítica | Sistema OK | Set temp bat = 75°C | Modo → SAFE_STOP | mode == SAFE_STOP |
| TH-04 | Temp batería warning | Sistema OK | Set temp bat = 65°C | Modo → LIMP_MODE | mode == LIMP_MODE |
| TH-05 | Recovery temperatura | LIMP por temp | Set temp = 25°C | Modo → OK | mode == OK |

---

### 2.5 Scheduler Jitter Alto → Warning

| ID | Descripción | Condición Inicial | Acción | Resultado Esperado | Criterio PASS |
|----|-------------|------------------|--------|-------------------|---------------|
| SJ-01 | Jitter warning | Sistema OK | Inyectar jitter >1ms | Warning en log | Log: "Jitter alto" |
| SJ-02 | Jitter crítico | Sistema OK | Inyectar jitter >2ms | Múltiples overruns | overrun_count > 3 |
| SJ-03 | Scheduler recovery | LIMP por overrun | Reducir jitter | Modo → OK | mode == OK |

---

### 2.6 Integración Completa

| ID | Descripción | Condición Inicial | Acción | Resultado Esperado | Criterio PASS |
|----|-------------|------------------|--------|-------------------|---------------|
| INT-01 | Caso complejo múltiple | OK | Multiple faults | Modo según prioridad | Prioridad correcta |
| INT-02 | Transición OK→LIMP→SAFE | OK | Voltaje bajo → luego crítico | OK→LIMP→SAFE_STOP | Secuencia correcta |
| INT-03 | Telemetry publish | OK | Ninguna (esperar 200ms) | Datos en socket | JSON válido recibido |

---

## 3. MATRIZ DE TRAZABILIDAD

| Requisito | MT-01 | MT-02 | VB-01 | VB-02 | SC-01 | TH-01 | SJ-01 | INT-01 |
|-----------|-------|-------|-------|-------|-------|-------|-------|--------|
| Timeout 2s | X | X | | | | | | |
| Protección Vbat | | | X | X | | | | |
| Protección SOC | | | | | X | | | |
| Protección Temp | | | | | | X | | |
| Scheduler | | | | | | | X | |
| Integración | | | | | | | | X |

---

## 4. PROCEDIMIENTOS DE TEST

### 4.1 Setup de Prueba

```
cpp
// Setup común para todas las pruebas
void test_setup() {
    // Inicializar sistema
    mode_manager.reset();
    supervisor.reset();
    timeout_detector.reset();
    voltage_protection.enable(true);
    
    // Configurar simulador CAN
    simulator.set_mode(SimulationMode::NORMAL);
    simulator.start();
    
    // Esperar estabilización
    std::this_thread::sleep_for(100ms);
}
```

### 4.2 Cleanup

```
cpp
void test_cleanup() {
    simulator.stop();
    mode_manager.reset();
}
```

---

## 5. HERRAMIENTAS DE PRUEBA

### 5.1 CanSimulator
```
cpp
// Simular timeout motor
simulator.set_mode(SimulationMode::MOTOR_TIMEOUT);

// Simular voltaje bajo
simulator.config_.vbat_mv = 45000;
simulator.set_mode(SimulationMode::LOW_VOLTAGE);

// Simular temperatura alta
simulator.config_.motor_temps_c = {105.0, 30.0, 30.0, 30.0};
simulator.set_mode(SimulationMode::HIGH_TEMP);
```

### 5.2 Verificación
```
cpp
// Verificar modo
ASSERT_EQ(SystemMode::SAFE_STOP, mode_manager.get_mode());

// Verificar factor de potencia
ASSERT_DOUBLE_EQ(0.0, mode_manager.get_power_factor());

// Verificar torque zero
ASSERT_TRUE(mode_manager.is_torque_zero());
```

---

## 6. RESULTADOS ESPERADOS

### 6.1 Summary Table

| Suite | Total | PASS | FAIL | Blocked |
|-------|-------|------|------|---------|
| Motor Timeout | 4 | 4 | 0 | 0 |
| Voltaje Batería | 4 | 4 | 0 | 0 |
| SOC | 3 | 3 | 0 | 0 |
| Temperatura | 5 | 5 | 0 | 0 |
| Scheduler | 3 | 3 | 0 | 0 |
| Integración | 3 | 3 | 0 | 0 |
| **TOTAL** | **22** | **22** | **0** | **0** |

---

## 7. CRITERIOS DE ACEPTACIÓN

### 7.1 Para Liberación
- 100% pruebas PASS
- Coverage > 80%
- Sin defectos críticos abiertos
- Documentación actualizada

### 7.2 Defect Severity
- **CRITICAL**: Sistema no funciona, bloquea liberación
- **HIGH**: Funcionalidad degradada, debe resolverse
- **MEDIUM**: Workaround disponible
- **LOW**: Mejora, puede resolver en siguiente sprint

---

## 8. EJECUCIÓN

### 8.1 Orden Sugerido
1. Pruebas unitarias de cada módulo
2. Pruebas de integración básicas (MT, VB, SC, TH)
3. Pruebas de integración complejas (INT)
4. Pruebas de regresión

### 8.2 Entorno
- Target: Linux con PREEMPT_RT
- Compilador: GCC 9+
- Dependencias: CMake, pthread, socketcan

---

**Fecha**: 2025  
**Versión**: 1.0  
**Autor**: Lead Engineer ECU ATC8110
