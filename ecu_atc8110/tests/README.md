# ECU Test Framework v2.0 - README

## Descripción

Framework de pruebas automatizadas profesional para el sistema ECU ATC8110. Soporta ejecución en modo simulación (SIM) y con hardware real (SocketCAN).

## Características Principales

- **Backends de Ejecución**: SIM (CanSimulator) y REAL (SocketCAN)
- **Políticas de Seguridad**: DEV (desarrollo) y PROD (producción)
- **Métricas de Timing**: p50/p95/p99 jitter, deadline misses, overruns
- **Tests de Lógica vs Timing**: Diferenciación clara entre tests deterministas y de scheduler RT
- **Reportes Enriquecidos**: JSON y Markdown con metadata del sistema

## Estructura

```
tests/
├── backends/
│   ├── ican_backend.hpp       # Interfaz abstracta
│   ├── sim_backend.hpp        # Backend SIM
│   ├── socketcan_backend.hpp  # Backend REAL
│   └── backend_factory.hpp    # Factory
├── framework/
│   ├── cli_parser.hpp         # Parser CLI
│   ├── clock_provider.hpp     # Abstracción de tiempo
│   ├── test_context.hpp       # Contexto enriquecido
│   └── timing_metrics.hpp     # Métricas de timing
├── ecu_test_framework.hpp     # Framework core
├── test_suites.hpp           # Suites originales
├── test_suites_v2.hpp        # Suites mejoradas
├── ecu_tests_main.cpp        # Runner v1 (original)
├── ecu_tests_main_v2.cpp     # Runner v2
├── CMakeLists.txt            # Build config
└── README.md                 # Este archivo
```

## Compilación

```
bash
cd ecu_atc8110
mkdir -p build/tests
cd build/tests
cmake ../../tests
make
```

## Ejecución

### Modo Simulación (Desarrollo)

```
bash
# Todos los tests en modo simulación
./ecu_tests

# Especificar backend
./ecu_tests --backend=sim

# Con reportes
./ecu_tests --report
```

### Modo ECU Real (SocketCAN)

```
bash
# Con interfaz CAN por defecto
./ecu_tests --backend=socketcan

# Con interfaces específicas
./ecu_tests --backend=socketcan --if0=emuccan0 --if1=emuccan1

# Con reportes
./ecu_tests --backend=socketcan --report
```

### Políticas de Seguridad

```
bash
# Policy DEV (default): recovery automático
./ecu_tests --policy=dev

# Policy PROD: requiere ACK y condiciones estables
./ecu_tests --policy=prod
```

### Suites y Tests Específicos

```
bash
# Suite específica
./ecu_tests --suite=MotorTimeoutSuite

# Test específico
./ecu_tests --test=MT_01

# Suite de timing
./ecu_tests --suite=SchedulerTimingSuite --timing-iterations=5000
```

## Argumentos CLI

| Argumento | Descripción | Default |
|-----------|-------------|---------|
| `--backend` | Backend: `sim` o `socketcan` | `sim` |
| `--if0` | Interfaz CAN primaria | `emuccan0` |
| `--if1` | Interfaz CAN secundaria | `emuccan1` |
| `--policy` | Política: `dev` o `prod` | `dev` |
| `--suite` | Ejecutar suite específica | todas |
| `--test` | Ejecutar test específico | todos |
| `--report` | Generar reportes JSON/MD | no |
| `--verbose` | Salida detallada | no |
| `--timing-iterations` | Iteraciones para tests de timing | 1000 |
| `--timing-period` | Período base en microsegundos | 1000 |
| `--help` | Mostrar ayuda | - |

## Ejemplos Completos

### Desarrollo (Banco de Pruebas)

```
bash
# Tests rápidos en simulación
./ecu_tests --backend=sim --policy=dev

# Tests con métricas de timing
./ecu_tests --backend=sim --suite=SchedulerTimingSuite --timing-iterations=10000 --report
```

### Producción (ECU Real)

```
bash
# Tests en ECU real
./ecu_tests --backend=socketcan --if0=emuccan0 --policy=prod --report

# Tests completos con timing
./ecu_tests --backend=socketcan --suite=SchedulerTimingSuite --timing-iterations=5000 --report
```

## Suites Disponibles

| Suite | Tests | Descripción | Backend |
|-------|-------|-------------|---------|
| MotorTimeoutSuiteV2 | 5 | Timeout de motores (2s) | SIM/REAL |
| VoltageSuite | 4 | Protección de voltaje | SIM/REAL |
| SocSuite | 3 | Estado de carga batería | SIM/REAL |
| TemperatureSuite | 5 | Temperatura motores/batería | SIM/REAL |
| SchedulerTimingSuite | 4 | Jitter y deadline misses | SIM |
| IntegrationSuite | 3 | Pruebas complejas | SIM/REAL |
| CanCommunicationSuite | 3 | Comunicación CAN | SIM/REAL |
| HmiIntegrationSuiteV2 | 6 | Integración HMI mejorada | SIM/REAL |

## Tests de Timing (SchedulerTimingSuite)

La nueva suite de timing mide:

- **Jitter**: p50, p95, p99, máximo
- **Deadline Misses**: Conteo y tasa
- **Overruns**: Detección y métricas

Ejemplo de salida:

```
[SUITE] SchedulerTimingSuite
----------------------------------------
  [PASS] SJ2_01_JitterMeasurement
    Jitter: p50=50us p95=95us max=100us
  [PASS] SJ2_02_JitterPercentiles
    Jitter percentiles:
      p50: 50 us
      p95: 95 us
      p99: 99 us
      max: 100 us
      mean: 50.5 us
  [PASS] SJ2_03_DeadlineMiss
    Deadline misses: 50 (5.0%)
  [PASS] SJ2_04_OverrunDetection
    Overruns: 20 (2.0%)
```

## Políticas de Seguridad

### DEV (Desarrollo)

- Recovery automático de SAFE_STOP permitido
- No requiere ACK
- Timeout de recovery: 5 segundos
- Ideal para desarrollo y banco de pruebas

### PROD (Producción)

- Recovery automático NO permitido
- Requiere ACK explícito
- Requiere condiciones estables (mínimo 2 segundos)
- Timeout de recovery: 30 segundos
- Para tests en ECU real en producción

## Permisos y Requisitos

### Linux

```
bash
# Permisos necesarios (root o grupo can)
sudo setcap cap_sys_nice+ep ecu_tests
sudo setcap cap_net_raw+ep ecu_tests

# O ejecutar como root
sudo ./ecu_tests --backend=socketcan
```

### Interfaces CAN

```
bash
# Ver interfaces disponibles
ip link show

# Configurar interfaz (si no está configurada)
sudo ip link set emuccan0 up type can bitrate 500000
```

### Kernel RT

Para tests de timing precisos, se recomienda kernel con PREEMPT_RT:

```
bash
# Verificar si hay PREEMPT_RT
uname -r | grep -i preempt
```

## Reportes

### JSON (`test_report.json`)

Contiene:
- Metadata del framework (versión)
- Configuración de ejecución (backend, policy, interfaces)
- Información del sistema (kernel, CPU, PREEMPT_RT, etc.)
- Resumen de resultados
- Lista detallada de tests con duración

### Markdown (`test_report.md`)

Reporte legible con:
- Tabla de configuración
- Información del sistema
- Tabla de resultados
- Métricas de timing (si aplica)

## Exit Codes

| Code | Significado |
|------|-------------|
| 0 | PASS - Todos los tests pasaron |
| 1 | FAIL - Algún test falló |
| 2 | SKIPPED/ENV - Tests no ejecutados o entorno no disponible |

## Desarrollo

### Añadir Nuevo Test

1. Añadir a la suite correspondiente en `test_suites_v2.hpp`:

```
cpp
class MY_01_NewTest : public BaseTest {
public:
    void run() override {
        // Tu código de test
        assert_eq(expected, actual);
    }
};
```

2. Añadir a la lista de tests de la suite:

```
cpp
tests.push_back(std::make_unique<MY_01_NewTest>());
```

### Añadir Nueva Suite

1. Crear clase que herede de `TestSuite`
2. Implementar `get_tests()` retornando vector de `unique_ptr<BaseTest>`
3. Añadir al main:

```
cpp
framework.add_suite<MyNewSuite>();
```

### Definir Categoría de Test

```
cpp
// En el contexto del test
ctx.test_category = TestCategory::LOGIC;      // Determinista
ctx.test_category = TestCategory::TIMING;     // Mide jitter
ctx.required_backend = RequiredBackend::SIM_ONLY;  // Solo SIM
```

## Estado

✅ Framework implementado  
✅ 8 suites configuradas  
✅ 28+ tests definidos  
✅ Backends SIM y SocketCAN  
✅ Políticas DEV/PROD  
✅ Métricas de timing  
✅ Reportes JSON/MD enriquecidos  
✅ Documentación completa  

---

**Versión**: 2.0.0  
**Fecha**: 2026
