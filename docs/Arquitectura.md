
## Descripción de Componentes

### include/ecu_fox/
Contiene los archivos de cabecera que definen:

- **constantes_fox.h:** Valores y macros globales para todo el sistema.
- **estructuras_fox.h:** Declaración de estructuras de datos compartidas entre módulos.
- **declaraciones_fox.h:** Prototipos de funciones comunes, permitiendo una interfaz pública clara.
- **funciones.h:** Funciones auxiliares y utilitarias para facilitar la reutilización del código.

### src/
Contiene la implementación del sistema dividida en módulos:

- **main.c:**  
  Punto de entrada del programa. Se encarga de inicializar el sistema, crear y gestionar los hilos de ejecución y finalizar el sistema de forma adecuada.

- **hilos/:**  
  Módulo dedicado a la gestión de hilos. Aquí se implementan funciones que crean, sincronizan y gestionan los hilos usando pthreads (equivalentes a las APIs QNX).

- **comunicacion/:**  
  Módulo que implementa la capa de comunicación. Por ejemplo, aquí se adaptarán las llamadas para el bus CAN utilizando SocketCAN en Linux, en lugar de las llamadas específicas de QNX.

- **impresion/:**  
  Funciones para la impresión, logging o depuración. Este módulo facilita el registro de información útil para el monitoreo y la solución de problemas.

- **inicializacion/:**  
  Módulo encargado de la configuración inicial del sistema. Aquí se configuran dispositivos, se abren interfaces (como la interfaz CAN) y se inicializan recursos. Es en este módulo donde se adaptan las llamadas QNX a sus equivalentes en Linux (por ejemplo, usando SocketCAN para CAN).

- **drivers/:**  
  (Opcional) Código específico para la abstracción de hardware y drivers. Por ejemplo, un módulo que encapsule el funcionamiento del bus CAN, permitiendo aislar el resto del sistema de detalles específicos del hardware.

- **utils/:**  
  (Opcional) Funciones utilitarias, como un sistema de logging avanzado o herramientas de análisis, que pueden ser utilizadas a lo largo del proyecto.

### tests/
Este directorio está destinado a pruebas unitarias e integración, lo que permite validar el comportamiento de cada módulo de forma aislada y asegurar la calidad del sistema a medida que se incorporan nuevas funcionalidades.

### docs/
Contiene la documentación del proyecto, guías de usuario, diagramas y este README.md, que explica la arquitectura y la visión del sistema.

## Build System y Configuración

- **CMake:**  
  Se utiliza CMake para gestionar la compilación del proyecto en Ubuntu, lo que facilita la integración de nuevos módulos y la gestión de dependencias de manera centralizada.

- **Herramientas de Compilación:**  
  El uso de gcc/g++ junto con CMake (o Makefiles) permite configurar el entorno de compilación en Ubuntu 18.04, asegurando compatibilidad con las herramientas modernas y facilitando la migración desde QNX.

## Escalabilidad y Mantenimiento

La arquitectura propuesta favorece:

- **Modularidad:** Cada funcionalidad se encuentra aislada en su propio subdirectorio, lo que facilita la adición, eliminación o modificación de módulos sin afectar al sistema global.
- **Facilidad para las Pruebas:** Con un directorio dedicado a tests, es sencillo implementar pruebas unitarias e integración para validar que cada módulo funciona correctamente.
- **Flexibilidad y Evolución:** La estructura permite la incorporación de nuevos drivers, módulos de comunicación o funcionalidades adicionales de manera ordenada, sin reestructurar todo el código.
- **Mantenimiento Sencillo:** La separación clara entre interfaces (archivos de cabecera en include/ecu_fox/) y la implementación (src/) facilita el mantenimiento y la colaboración entre distintos desarrolladores.

## Migración de QNX a Ubuntu

El proceso de migración se centra en:

- **Adaptar APIs Específicas:** Reemplazar las llamadas a funciones y APIs de QNX (por ejemplo, para la gestión de hilos, temporizadores o comunicación) por sus equivalentes en Linux utilizando POSIX y librerías como SocketCAN.
- **Configuración de Hardware:** Ajustar la inicialización de dispositivos en el módulo de inicialización para que se adapten a Ubuntu, por ejemplo, configurando la interfaz CAN con SocketCAN.
- **Recompilación y Pruebas:** Utilizar herramientas modernas de compilación y pruebas en Ubuntu para garantizar que el sistema funcione correctamente tras la migración.

## Conclusión

Esta arquitectura escalable y modular no solo facilita la migración del sistema ECU_FOX de QNX a Ubuntu 18.04, sino que también sienta las bases para futuras mejoras y ampliaciones. Con una estructura bien organizada, es más sencillo mantener y evolucionar el sistema, garantizando robustez y facilidad de integración de nuevas funcionalidades.
