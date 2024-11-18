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
