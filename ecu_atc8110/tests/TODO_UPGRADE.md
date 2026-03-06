# ECU Test Framework Upgrade Plan - COMPLETADO ✅

## Objetivo
Subir el framework a nivel profesional para uso en banco de pruebas y ECU real.

## Tareas Principales

### A) Interfaces C++ (Diseño) ✅
- [x] ICanBackend (Sim/SocketCAN) - `backends/ican_backend.hpp`
- [x] IClock / TimeProvider - `framework/clock_provider.hpp`
- [x] TestContext enriquecido - `framework/test_context.hpp`

### B) Backends de Ejecución ✅
- [x] Backend SIM (CanSimulator existente) - `backends/sim_backend.hpp`
- [x] Backend REAL (SocketCAN) - `backends/socketcan_backend.hpp`
- [x] CLI: --backend, --if0, --if1 - `framework/cli_parser.hpp`

### C) Diferenciación Tests ✅
- [x] Tests de lógica (deterministas) - `TestCategory::LOGIC`
- [x] Tests de timing (jitter, deadline misses) - `SchedulerTimingSuite`
- [x] Métricas: p50/p95/p99/max jitter, deadline_miss_count - `framework/timing_metrics.hpp`

### D) Mejoras HMI Suite ✅
- [x] Validar esquema (campos obligatorios) - HMI2_01_SchemaValidation
- [x] Validar snapshot inicial completo - HMI2_03_InitialSnapshot
- [x] Validar latencia EVENT desde raise_error (<100ms) - HMI2_04_EventLatency
- [x] Validar reconexión - HMI2_05_Reconnection
- [x] Validar orden de timestamps (monotónico) - HMI2_06_TimestampOrder

### E) Políticas de Seguridad ✅
- [x] --policy=dev: recovery automático de SAFE_STOP - `DevSecurityPolicy`
- [x] --policy=prod: SAFE_STOP requiere reset explícito/ACK - `ProdSecurityPolicy`

### F) Reporte y Exit Codes ✅
- [x] Exit code 0 PASS, 1 FAIL, 2 SKIPPED/ENV - `ecu_tests_main_v2.cpp`
- [x] Enhanced test_report.json con metadata - `generate_enhanced_json_report()`

### G) Actualización README ✅
- [x] Cómo correr en sim vs real
- [x] Requisitos de permisos (SCHED_FIFO)
- [x] Ejemplos de ejecución

---

## Archivos Creados

### Backends
- `tests/backends/ican_backend.hpp` - Interfaz ICanBackend
- `tests/backends/sim_backend.hpp` - Backend SIM
- `tests/backends/socketcan_backend.hpp` - Backend REAL
- `tests/backends/backend_factory.hpp` - Factory de backends

### Framework
- `tests/framework/clock_provider.hpp` - IClock/TimeProvider
- `tests/framework/test_context.hpp` - TestContext enriquecido
- `tests/framework/timing_metrics.hpp` - Métricas de timing
- `tests/framework/cli_parser.hpp` - Parser CLI mejorado

### Tests
- `tests/test_suites_v2.hpp` - Suites mejoradas con policy-aware tests
- `tests/ecu_tests_main_v2.cpp` - Main runner v2

### Documentación
- `tests/README.md` - Documentación completa v2.0

### Build
- `tests/CMakeLists.txt` - Actualizado para v2

---

## Uso

```bash
# Compilar
cd ecu_atc8110/build/tests
cmake ../../tests
make

# Ejecutar en SIM
./ecu_tests --backend=sim

# Ejecutar en ECU real
./ecu_tests --backend=socketcan --if0=emuccan0

# Con política PROD
