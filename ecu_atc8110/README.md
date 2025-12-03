# ECU ATX-1610 - Sistema de Control para Vehículo FOX

Sistema embebido de control en tiempo real para el vehículo experimental FOX, diseñado para la ECU ATC-8110 con Ubuntu 18.04 LTS.

## 🚀 Inicio Rápido

### Despliegue en la ECU

```bash
# 1. Transferir código a la ECU
scp -r ecu_atc8110 fox@193.147.165.236:/home/fox/

# 2. Conectar a la ECU
ssh fox@193.147.165.236

# 3. Compilar
cd /home/fox/ecu_atc8110
mkdir build && cd build
cmake .. && make

# 4. Configurar interfaces CAN
sudo ../scripts/setup_can.sh --real

# 5. Ejecutar
sudo ./ecu_atc8110
```

📖 **Guía completa**: [docs/DEPLOYMENT.md](docs/DEPLOYMENT.md)

---

## 📋 Características

- ✅ **Comunicación CAN** vía SocketCAN (EMUC-B2S3)
  - CAN0 @ 1 Mbps: Motores + Supervisor
  - CAN1 @ 500 Kbps: BMS (24 celdas)
- ✅ **Gestión de Batería** con monitoreo completo de celdas
- ✅ **Control de 4 Motores** con protocolo CCP
- ✅ **Adquisición de Datos** (PEX-1202L, PEX-DA16)
- ✅ **Máquina de Estados** robusta
- ✅ **Sistema de Alarmas** multinivel
- 🔄 **Control de Tracción** (en desarrollo)
- 🔄 **Suspensión Activa** (en desarrollo)

---

## 🏗️ Arquitectura

```
ecu_atc8110/
├── comunicacion_can/      # Sistema CAN (SocketCAN)
│   ├── can_protocol.hpp   # Definiciones del protocolo
│   ├── can_bms_handler.*  # Handler BMS (24 celdas)
│   ├── can_manager.hpp    # Gestor principal CAN
│   └── socketcan_interface.*  # Driver SocketCAN
├── adquisicion_datos/     # Sensores analógicos/digitales
├── control_vehiculo/      # Controladores (batería, tracción, suspensión)
├── logica_sistema/        # Máquina de estados y coordinación
├── interfaces/            # CLI y diagnóstico
├── scripts/               # Scripts de configuración
│   └── setup_can.sh      # Configuración automática de CAN
├── docs/                  # Documentación técnica
└── tests/                 # Pruebas unitarias
```

---

## 🔧 Hardware Soportado

| Componente | Modelo | Estado |
|------------|--------|--------|
| ECU | ATC-8110 | ✅ |
| CAN | EMUC-B2S3 | ✅ |
| BMS | 24 celdas | ✅ |
| Motores | 4x CCP | ✅ |
| GPS | ublox SM-76G | ⏳ |
| ADC | PEX-1202L | ⏳ |
| DAC | PEX-DA16 | ⏳ |

---

## 📚 Documentación

- **[DEPLOYMENT.md](docs/DEPLOYMENT.md)** - Guía completa de despliegue
- **[walkthrough.md](docs/walkthrough.md)** - Implementación detallada
- **[implementation_plan.md](docs/implementation_plan.md)** - Plan técnico
- **[task.md](docs/task.md)** - Lista de tareas del proyecto

---

## 🧪 Desarrollo y Testing

### Compilación Local (Testing)

```bash
# Modo virtual (sin hardware)
mkdir build && cd build
cmake ..
make

# Configurar interfaces virtuales
sudo ../scripts/setup_can.sh --virtual

# Ejecutar
./ecu_atc8110
```

### Monitoreo CAN

```bash
# Ver mensajes CAN en tiempo real
candump can0  # Motores + Supervisor
candump can1  # BMS

# Enviar mensaje de prueba
cansend can0 100#AABBCCDD
```

---

## 🔍 Protocolo CAN

### IDs CAN Principales

| ID | Dispositivo | Descripción |
|----|-------------|-------------|
| `0x100` | Supervisor | Heartbeat |
| `0x180` | BMS | Estado de batería |
| `0x201-0x204` | ECU→Motores | Comandos |
| `0x281-0x284` | Motores→ECU | Telemetría |

### Protocolo BMS

Mensajes con formato ASCII: `[index][param][value]`
- `'V'` - Voltaje de celda (mV)
- `'T'` - Temperatura de celda (°C)
- `'E'` - Estado del pack
- `'A'` - Alarmas

---

## ⚙️ Configuración

### Requisitos del Sistema

- **OS**: Ubuntu 18.04 LTS
- **Kernel**: Linux con SocketCAN
- **Compilador**: GCC 7.5+ con C++17
- **CMake**: 3.10+
- **Herramientas**: can-utils

### Variables de Entorno

```bash
# Opcional: Configurar nivel de logging
export ECU_LOG_LEVEL=DEBUG  # DEBUG, INFO, WARNING, ERROR
```

---

## 🐛 Solución de Problemas

### Interfaces CAN no aparecen

```bash
# Verificar hardware
lspci | grep -i can

# Cargar módulos
sudo modprobe can can_raw
```

### Error de permisos

```bash
# Ejecutar con sudo
sudo ./ecu_atc8110
```

### No se reciben mensajes

```bash
# Verificar estado de interfaces
ip link show can0
ip -s link show can0  # Ver estadísticas
```

📖 Más detalles en [docs/DEPLOYMENT.md](docs/DEPLOYMENT.md)

---

## 🤝 Contribución

Este proyecto sigue las mejores prácticas de desarrollo embebido:
- Código modular y testeable
- Documentación completa
- Manejo robusto de errores
- Logging detallado

---

## 📄 Licencia

Proyecto académico - Universidad [Nombre]

---

## 📞 Contacto

- **Repositorio**: [GitHub/GitLab URL]
- **Documentación**: `docs/`
- **Issues**: [URL de issues]

---

**Estado del Proyecto**: ✅ Listo para despliegue en hardware real

Última actualización: 2025-11-27
