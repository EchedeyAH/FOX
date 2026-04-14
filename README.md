# ECU ATX-1610 – Arquitectura de Control para Vehículo Experimental FOX

Este repositorio contiene el desarrollo completo del sistema embebido para la ECU ATX-1610, diseñada como núcleo de control para el vehículo experimental FOX. El sistema está orientado a cumplir con requisitos de robustez, latencia mínima, modularidad, mantenimiento sencillo y alta escalabilidad.

## 📚 Propósito del Proyecto

Desarrollar una arquitectura software robusta, modular y optimizada que integre sensores, actuadores y protocolos de comunicación (CAN), para permitir el control en tiempo real de funciones críticas del vehículo como:

- Suspensión activa
- Gestión de baterías
- Tracción y estabilidad
- Monitorización general y diagnóstico

El proyecto tiene una finalidad académica y profesional dentro del marco de un trabajo de ingeniería avanzada, respetando las mejores prácticas de diseño de sistemas embebidos.

---

## 📁 Estructura del Proyecto

```
ecu_atc8110/
├── common/             # Tipos, logs, interfaces base (Sensor, Bus, Controlador)
├── comunicacion_can/   # Comunicación CAN vía SocketCAN y EMUC-B2S3
├── adquisicion_datos/  # Lectura desde tarjetas PEX-1202L y PEX-DA16
├── control_vehiculo/   # Lógica de control de suspensión, batería, tracción, etc.
├── logica_sistema/     # Coordinación del sistema, máquina de estados, main()
├── interfaces/         # CLI, diagnóstico remoto, actualizaciones OTA
├── config/             # Archivos de configuración por entorno
├── scripts/            # Scripts de inicialización, configuración y despliegue
├── tests/              # Pruebas unitarias e integración
├── docs/               # Documentación técnica y esquemas
└── CMakeLists.txt      # Sistema de compilación
```

---

## ⚙️ Tecnologías

- **Lenguaje:** C++17
- **SO:** Ubuntu 18.04 LTS (x86_64)
- **Protocolo de Comunicación:** SocketCAN
- **Hardware:**
  - CPU ATX-1610 (basado en ATC-8110)
  - Tarjeta EMUC-B2S3 (CAN)
  - Tarjeta PEX-1202L (Entradas analógicas)
  - Tarjeta PEX-DA16 (Salidas analógicas/digitales)

---

## 🔁 Flujo de Desarrollo

El desarrollo sigue un enfoque incremental y modular, dividido por fases:

1. **Fundaciones del sistema:** tipos comunes, logging, interfaces.
2. **Comunicación CAN:** recepción y envío vía EMUC-B2S3 + SocketCAN.
3. **Adquisición de datos:** sensores a través de PEX-1202L y PEX-DA16.
4. **Control del vehículo:** algoritmos de tracción, batería, suspensión.
5. **Lógica general del sistema:** máquina de estados y coordinación.
6. **CLI y diagnóstico:** interfaz de mantenimiento y actualización.
7. **Pruebas completas:** validación funcional y de rendimiento.

---

## 🧪 Tests

Cada módulo cuenta con su suite de pruebas:

- `tests/unit/` – pruebas unitarias con mocks.
- `tests/integration/` – integración con drivers y buses reales o simulados.

Se emplean técnicas de *mocking*, *loopback* CAN y simuladores de señales analógicas/digitales para validar sin depender 100% del hardware desde el inicio.

---

## 📌 Notas Importantes

- El proyecto está diseñado para ser escalable y permitir nuevas versiones de hardware.
- Se cumple con las normas académicas de trazabilidad, modularidad y documentación.
- Cada fase del proyecto está documentada y justificada en los informes técnicos anexos.

---

## 📞 Contacto / Créditos

**Autor:** Echedey Aguilar Hernández
eaguilar1@us.es


