# Proyecto FOX - Sistema en Tiempo Real

## Descripción General
Este proyecto implementa un sistema en tiempo real (RT) desarrollado para la plataforma FOX, con módulos de comunicación CAN y USB, planificación de tareas en tiempo real, y una interfaz para la gestión de sensores y actuadores. Utiliza un entorno RT Linux, lo que permite la ejecución eficiente de tareas críticas.

## Estructura del Proyecto
- **src/**: Código fuente del proyecto.
- **include/**: Archivos de cabecera compartidos entre módulos.
- **drivers/**: Controladores de hardware para CAN y USB.
- **config/**: Archivos de configuración del sistema.
- **rt_tasks/**: Tareas críticas en tiempo real.
- **scripts/**: Scripts de compilación, despliegue y monitoreo.
- **tests/**: Pruebas unitarias y de integración.
- **docs/**: Documentación del proyecto.

## Requisitos del Sistema
- RT Linux
- Herramientas de compilación: `gcc`, `make`
- Dependencias específicas: **librerías CAN, USB**

## Información sobre los ficheros
Consulta la información detallada de los ficheros en [InfoFiles.md](InfoFiles.md)


## Instalación y Configuración
Consulta [INSTALL.md](INSTALL.md) para más detalles sobre el proceso de instalación.

## Uso
Para ejecutar el proyecto, consulta la guía [USAGE.md](USAGE.md) y sigue las instrucciones para interactuar con el sistema en tiempo real.


# Estructura del Proyecto

```plaintext
- Estructura ideal - 
project_root/
│
├── src/                                # Código fuente principal
│   ├── main.c                          # Punto de entrada del programa
│   ├── can_module/                     # Módulo para procesos CAN
│   │   ├── can_manager.c               # Gestión de mensajes CAN
│   │   ├── can_handler.c               # Manejo de eventos CAN
│   │   ├── can_driver.c                # Interacción directa con el hardware
│   │   └── Makefile                    # Construcción específica del módulo CAN
│   │
│   ├── imu_module/                     # Módulo para IMU
│   │   ├── imu_manager.c               # Gestión de la IMU
│   │   ├── imu_driver.c                # Driver para la IMU
│   │   └── Makefile                    # Construcción específica del módulo IMU
│   │
│   ├── scheduler_module/               # Módulo para el planificador
│   │   ├── rt_scheduler.c              # Planificador de tareas en tiempo real
│   │   └── Makefile                    # Construcción específica del módulo
│   │
│   └── Makefile                        # Archivo de construcción global
│
├── include/                            # Archivos de cabecera compartidos
│   ├── can_manager.h                   # Cabecera del gestor CAN
│   ├── imu_manager.h                   # Cabecera del gestor IMU
│   ├── rt_scheduler.h                  # Cabecera del planificador
│   ├── ecu_config.h                    # Configuración general
│   └── ...                             # Más cabeceras
│
├── drivers/                            # Controladores de hardware
│   ├── can/                            # Controladores CAN
│   │   ├── can_driver.c
│   │   ├── can_dual.c                  # Compatibilidad con hardware dual
│   │   └── Makefile
│   │
│   ├── imu/                            # Controladores IMU
│   │   ├── imu_driver.c
│   │   └── Makefile
│
├── tests/                              # Pruebas unitarias e integración
│   ├── can_module/                     # Pruebas para el módulo CAN
│   │   ├── test_can_manager.c
│   │   ├── test_can_driver.c
│   │   └── Makefile
│   │
│   ├── imu_module/                     # Pruebas para el módulo IMU
│   │   ├── test_imu_manager.c
│   │   └── Makefile
│
├── config/                             # Configuración externa (JSON, YAML)
│   ├── can_config.json                 # Configuración del bus CAN
│   ├── imu_config.json                 # Configuración de la IMU
│   └── ecu_settings.yaml               # Configuración global del ECU
│
├── scripts/                            # Scripts de automatización
│   ├── build.sh                        # Script para compilar todo el proyecto
│   ├── deploy.sh                       # Despliegue en el vehículo
│   ├── clean.sh                        # Limpieza de binarios
│   └── monitor_can.sh                  # Monitorización del bus CAN
│
├── build/                              # Binarios generados
│   ├── can_module.o
│   ├── imu_module.o
│   ├── rt_scheduler.o
│   ├── project_executable              # Binario final del proyecto
│   └── logs/                           # Registros de compilación
│
├── docs/                               # Documentación del proyecto
│   ├── README.md                       # Guía general
│   ├── ARCHITECTURE.md                 # Explicación de la arquitectura
│   ├── CAN_PROTOCOL_SPEC.md            # Especificaciones del protocolo CAN
│   └── RT_TASK_DESIGN.md               # Diseño de tareas en tiempo real
│
└── Makefile                            # Makefile principal para toda la compilación
```
__________________________________________________________________________________________________________________________________________






