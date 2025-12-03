# Documentación ECU ATX-1610

Esta carpeta contiene la documentación técnica del proyecto ECU ATX-1610.

## Documentos Disponibles

### 📋 [task.md](task.md)
Lista de tareas para la implementación del sistema de comunicación CAN. Incluye el progreso de cada componente.

### 📐 [implementation_plan.md](implementation_plan.md)
Plan de implementación detallado para la migración del sistema CAN desde QNX a Linux SocketCAN. Incluye:
- Análisis del sistema legacy
- Cambios propuestos por componente
- Plan de verificación
- Notas técnicas sobre diferencias QNX vs Linux

### 🎯 [walkthrough.md](walkthrough.md)
Guía completa de la implementación realizada. Documenta:
- Todos los cambios realizados
- Arquitectura del sistema
- Instrucciones de compilación y despliegue
- Procedimientos de prueba
- Compatibilidad con hardware

## Estructura del Proyecto

```
ecu_atc8110/
├── docs/                      # Documentación (este directorio)
├── common/                    # Tipos, interfaces y utilidades comunes
├── comunicacion_can/          # Sistema de comunicación CAN
│   ├── can_protocol.hpp       # Definiciones del protocolo CAN
│   ├── can_bms_handler.*      # Handler para BMS
│   ├── can_manager.hpp        # Gestor principal CAN
│   └── socketcan_interface.*  # Driver SocketCAN
├── adquisicion_datos/         # Lectura de sensores (PEX-1202L)
├── control_vehiculo/          # Lógica de control
├── logica_sistema/            # Coordinación del sistema
├── interfaces/                # CLI y diagnóstico
├── scripts/                   # Scripts de configuración
│   └── setup_can.sh          # Configuración de interfaces CAN
└── tests/                     # Pruebas unitarias e integración
```

## Hardware Soportado

- **ECU**: ATC-8110 (basado en ATX-1610)
- **Tarjeta CAN**: EMUC-B2S3
- **BMS**: Sistema de gestión de baterías (24 celdas)
- **Motores**: 4 controladores con protocolo CCP
- **GPS**: ublox SM-76G
- **ADC**: PEX-1202L
- **DAC**: PEX-DA16

## Sistema Operativo

- **OS**: Ubuntu 18.04 LTS
- **Protocolo**: SocketCAN (Linux)
- **Compilador**: GCC con soporte C++17

## Enlaces Rápidos

- [README principal del proyecto](../README.md)
- [Análisis del código legacy](../../analisis_ecu_fox.md)
- [Script de configuración CAN](../scripts/setup_can.sh)

## Inicio Rápido

### Compilación

```bash
cd ecu_atc8110
mkdir -p build && cd build
cmake ..
make
```

### Configuración CAN

```bash
# Modo real (hardware EMUC-B2S3)
sudo ./scripts/setup_can.sh --real

# Modo virtual (testing sin hardware)
sudo ./scripts/setup_can.sh --virtual
```

### Ejecución

```bash
sudo ./build/ecu_atc8110
```

## Contribución

Para contribuir al proyecto, consulta la documentación en `docs/` y sigue las convenciones establecidas en el código existente.
