### Makefile

Este proyecto incluye un Makefile que facilita la compilación, prueba y gestión del código fuente. A continuación, se describen las variables, archivos y reglas que componen el Makefile.
Variables

    CC: El compilador utilizado para la construcción del proyecto (en este caso, gcc).
    CFLAGS: Opciones de compilación que incluyen advertencias (-Wall, -Wextra), directorios de inclusión y la opción -g para incluir información de depuración.
    SRC_DIR, BUILD_DIR, TEST_DIR: Directorios que organizan el código fuente, los archivos compilados y las pruebas, respectivamente.

#### Archivos Fuente y Objeto

    SRC_FILES: Esta variable se genera automáticamente a partir de los archivos .c en los directorios correspondientes.
    OBJ_FILES: Convierte la lista de archivos fuente en una lista de archivos objeto, que son necesarios para la creación del ejecutable.

#### Reglas

    El Makefile define varias reglas para gestionar el proceso de construcción:

    - all: Regla principal que compila el ejecutable del proyecto.
    $(EXECUTABLE): Regla que enlaza todos los archivos objeto para crear el ejecutable final.
    $(BUILD_DIR)/%.o: Regla que compila archivos fuente en archivos objeto, asegurando que el directorio de construcción exista antes de la compilación.
    - test: Regla para compilar y ejecutar pruebas unitarias.
    - clean: Regla que elimina los archivos generados durante la compilación, manteniendo el espacio de trabajo limpio.
    - install: Regla opcional que permite instalar el ejecutable en un directorio del sistema.
    - run: Regla que permite ejecutar el programa compilado directamente desde el Makefile, facilitando pruebas rápidas.

#### .PHONY

Las reglas all, clean, install, test y run están declaradas como .PHONY, lo que indica que no corresponden a archivos reales. Esto evita conflictos y asegura que estas reglas se ejecuten cada vez que se invocan.

_________________________________________________________________________________________

# ECU
 
 Programa principal que se encarga de lanzar todos los hilos:
 
 * Hilo de errores: Comprueba y gestiona los errores del sistema.
 * Hilo de IMU: Lee datos de la unidad de medición inercial.
 * Hilo de adquisición de datos: Lee datos de los sensores, como el acelerador, el freno, el volante y la suspensión.
 * Hilo de adquisición de datos de salidas analógicas: Gestiona las salidas analógicas de la ECU.
 * Hilo de recepción de mensajes CAN: Recibe mensajes CAN del Supervisor y los motores.
 * Hilo de comunicación con el Supervisor: Gestiona la comunicación con el Supervisor.
 * Hilo de comunicación con los motores: Gestiona la comunicación con cada uno de los motores.
 * Hilo de gestión de potencia: Se encarga de la gestión de energía de la ECU.
 * Hilo de control de tracción: Implementa el control de tracción del vehículo.
 * Hilo de control de estabilidad: Implementa el control de estabilidad del vehículo.


Inicializa las variables globales y las estructuras.
Bloquea las señales SIGRTMIN, SIGRTMIN+1 y SIGRTMIN+2.
Crea los procesos hijos para el driver CAN, el proceso CAN2 (BMS) y el proceso IMU datalogging.
Crea los hilos de la ECU.
Espera a que todos los hilos estén listos.
Envía un mensaje de inicio a los demás hilos.
Comprueba si hay errores antes de iniciar el bucle principal.
Ejecuta el bucle principal, que recibe datos de la cola de mensajes del proceso CAN2 (BMS) y envía datos a la cola de mensajes del proceso IMU datalogging.
Imprime datos en la pantalla según la configuración.
Espera un tiempo determinado antes de volver a ejecutar el bucle.
Mata los procesos hijos y cierra la cola de mensajes al final del programa.
