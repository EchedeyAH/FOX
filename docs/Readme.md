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

## Instalación y Configuración
Consulta [INSTALL.md](INSTALL.md) para más detalles sobre el proceso de instalación.

## Uso
Para ejecutar el proyecto, consulta la guía [USAGE.md](USAGE.md) y sigue las instrucciones para interactuar con el sistema en tiempo real.


# Estructura de ficheros

project_root/
│
├── src/                                # Código fuente principal
│   ├── main.c                          # Punto de entrada del programa
│   ├── can_module/                     # Módulo para procesos CAN
│   │   ├── can_manager.c               # Código para la gestión de CAN
│   │   └── can_handler.c               # Manejador de mensajes CAN
│   │
│   ├── usb_module/                     # Módulo USB (si es relevante para RT)
│   │   └── usb_manager.c               # Código para la gestión de USB
│   │
│   ├── scheduler_module/               # Módulo para el planificador de tareas RT
│   │   └── rt_scheduler.c              # Planificador de tareas RT
│   │
│   └── ...                             # Otros módulos de funcionalidad
│
├── include/                            # Archivos de cabecera compartidos
│   ├── can_manager.h                   # Cabecera del módulo CAN Manager
│   ├── can_handler.h                   # Cabecera del manejador CAN
│   ├── usb_manager.h                   # Cabecera del módulo USB Manager
│   ├── rt_scheduler.h                  # Cabecera para el planificador RT
│   ├── project_config.h                # Configuración general del proyecto
│   └── ...                             # Cabeceras de otros módulos
│
├── drivers/                            # Controladores de hardware
│   ├── can_driver.c                    # Controlador CAN con optimizaciones RT
│   ├── usb_driver.c                    # Controlador USB (si aplica)
│   └── include/                        # Cabeceras de los controladores
│       ├── can_driver.h                # Cabecera para el controlador CAN
│       └── usb_driver.h                # Cabecera para el controlador USB
│
├── config/                             # Configuraciones externas en JSON/YAML
│   ├── can_config.json                 # Configuración CAN (baudios, filtro, etc.)
│   ├── usb_config.json                 # Configuración USB (si es relevante)
│   ├── rt_task_priorities.json         # Configuración de prioridades de tareas RT
│   └── project_settings.yaml           # Configuración general del proyecto
│
├── rt_tasks/                           # Tareas críticas en tiempo real
│   ├── task_comm.c                     # Tareas de comunicación en tiempo real
│   ├── task_sensor.c                   # Tareas de adquisición de datos en tiempo real
│   ├── task_control.c                  # Tareas de control y actuadores
│   ├── task_monitor.c                  # Tareas de monitoreo del sistema
│   └── ...                             # Otros archivos específicos de tareas RT
│
├── scripts/                            # Scripts de automatización
│   ├── deploy.sh                       # Script para despliegue en el dispositivo
│   ├── build.sh                        # Script para compilar el proyecto
│   ├── clean.sh                        # Script para limpieza de archivos generados
│   ├── setup_env.sh                    # Configura el entorno para RT Linux
│   └── monitor_rt.sh                   # Monitorización de tareas en tiempo real
│
├── build/                              # Archivos binarios generados
│   ├── can_manager.o                   # Objeto compilado de CAN Manager
│   ├── usb_manager.o                   # Objeto compilado de USB Manager
│   ├── rt_scheduler.o                  # Objeto compilado del planificador
│   ├── task_comm.o                     # Objeto compilado de tarea de comunicación
│   ├── task_sensor.o                   # Objeto compilado de tarea de sensor
│   └── project_executable              # Ejecutable final del proyecto
│
├── tests/                              # Pruebas unitarias y de integración
│   ├── test_can_module.c               # Pruebas para el módulo CAN
│   ├── test_usb_module.c               # Pruebas para el módulo USB
│   ├── test_rt_scheduler.c             # Pruebas para el planificador RT
│   ├── test_task_comm.c                # Pruebas para la tarea de comunicación
│   ├── test_task_sensor.c              # Pruebas para la tarea de sensor
│   └── mock_can_driver.c               # Mocks para pruebas del driver CAN
│
└── docs/                               # Documentación del proyecto
    ├── README.md                       # Descripción general y guía de uso
    ├── INSTALL.md                      # Guía de instalación en sistema RT Linux
    ├── USAGE.md                        # Documentación de uso
    ├── ARCHITECTURE.md                 # Explicación de la arquitectura
    ├── can_protocol_spec.pdf           # Especificación del protocolo CAN
    └── rt_task_scheduling.md           # Documentación de tareas y programación RT