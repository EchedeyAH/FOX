
>[!WARNING]
>PROYECTO EN DESARROLLO.

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
- RT Linuxca
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
/FOX
├── CMakeLists.txt         // Archivo de configuración del sistema de build (o Makefile)
├── include/
│   ├── ecu_fox/
│   │   ├── constantes_fox.h      // Constantes globales del proyecto
│   │   ├── estructuras_fox.h     // Declaración de estructuras de datos
│   │   ├── declaraciones_fox.h   // Prototipos de funciones comunes
│   │   └── funciones.h           // Funciones auxiliares o utilitarias
│   └── third_party/              // (Opcional) Headers de librerías de terceros
├── src/
│   ├── main.c                    // Punto de entrada del programa
│   ├── hilos/                    // Módulo de gestión de hilos
│   │   └── hilos.c
│   ├── comunicacion/             // Módulo de comunicación (por ejemplo, SocketCAN)
│   │   └── comunicacion.c
│   ├── impresion/                // Módulo de impresión, logging o depuración
│   │   └── impresion.c
│   ├── inicializacion/           // Módulo de inicialización y configuración del sistema
│   │   └── inicializacion.c
│   ├── drivers/                  // (Opcional) Código específico de drivers o abstracción hardware
│   │   └── can_driver.c          // Ejemplo: driver para el bus CAN
│   └── utils/                    // (Opcional) Funciones utilitarias y de apoyo (por ejemplo, logging avanzado)
│       └── logger.c
├── tests/                        // Directorio para pruebas unitarias y de integración
│   ├── test_main.c
│   └── CMakeLists.txt            // Configuración de build para las pruebas
└── docs/                         // Documentación del proyecto
    └── README.md                // Documentación general y guía de uso

```
