# Makefile

Este proyecto incluye un Makefile que facilita la compilación, prueba y gestión del código fuente. A continuación, se describen las variables, archivos y reglas que componen el Makefile.
### Variables

    CC: El compilador utilizado para la construcción del proyecto (en este caso, gcc).
    CFLAGS: Opciones de compilación que incluyen advertencias (-Wall, -Wextra), directorios de inclusión y la opción -g para incluir información de depuración.
    SRC_DIR, BUILD_DIR, TEST_DIR: Directorios que organizan el código fuente, los archivos compilados y las pruebas, respectivamente.

### Archivos Fuente y Objeto

    SRC_FILES: Esta variable se genera automáticamente a partir de los archivos .c en los directorios correspondientes.
    OBJ_FILES: Convierte la lista de archivos fuente en una lista de archivos objeto, que son necesarios para la creación del ejecutable.

### Reglas

    El Makefile define varias reglas para gestionar el proceso de construcción:

    - all: Regla principal que compila el ejecutable del proyecto.
    $(EXECUTABLE): Regla que enlaza todos los archivos objeto para crear el ejecutable final.
    $(BUILD_DIR)/%.o: Regla que compila archivos fuente en archivos objeto, asegurando que el directorio de construcción exista antes de la compilación.
    - test: Regla para compilar y ejecutar pruebas unitarias.
    - clean: Regla que elimina los archivos generados durante la compilación, manteniendo el espacio de trabajo limpio.
    - install: Regla opcional que permite instalar el ejecutable en un directorio del sistema.
    - run: Regla que permite ejecutar el programa compilado directamente desde el Makefile, facilitando pruebas rápidas.

### .PHONY

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
Mata los procesos hijos y cierra la cola de mensajes al final del programa.0

## Propuesta inicial estrutura de ficheros para el archivo ecu_fox.c
  ```    
 /ecu_fox/
├── include/
│   ├── constantes_fox.h
│   ├── estructuras_fox.h
│   ├── declaraciones_fox.h
│   └── funciones.h
└── src/
    ├── main.c
    ├── hilos.c
    ├── comunicacion.c
    ├── impresion.c
    └── inicializacion.c
```

### include/
    
    - constantes_fox.h
    - estructuras_fox.h
    - declaraciones_fox.h
    - funciones.h

### src/
    - main.c
    - hilos.c
    - comunicacion.c
    - impresion.c
    - inicializacion.c

_________________________________________________________________________________________
# SUPERVISOR

_________________________________________________________________________________________
# VEHICULO

_________________________________________________________________________________________
# IMU

_________________________________________________________________________________________
# BATERÍA

_________________________________________________________________________________________
# CONTROLADOR-MOTOR

_________________________________________________________________________________________
# POTENCIA

_________________________________________________________________________________________
# ERRORES

_________________________________________________________________________________________
# TARJETA DE ADQUISICIÓN DE DATOS

_________________________________________________________________________________________
# PILA HIDRÓGENO

_________________________________________________________________________________________
#########################################################################################
_________________________________________________________________________________________
# MIGRACION QNX A LINUX, CAMBIOS EN SOFTWARE
## + can2_fox.c
### Cabeceras específicas de QNX:
    <sys/neutrino.h>, <sys/mman.h>, <hw/inout.h>: Estas cabeceras no están disponibles en Linux y deberán ser reemplazadas con alternativas de Linux o reimplementaciones.
    Mensajería (MsgReceivePulse_r, ChannelCreate, ConnectAttach): Estas llamadas son específicas de QNX y se deben sustituir con mecanismos equivalentes en Linux, como pipes, sockets, o colas de mensajes POSIX.
### Driver CAN:
    El acceso a dispositivos CAN en Linux se gestiona con SocketCAN, disponible en el kernel de Linux, en lugar de las funciones propietarias de QNX (ConnectDriver, CanRead, CanRestart, etc.).
### Colas de mensajes:
    Las colas de mensajes POSIX (mq_open, mq_send) son compatibles tanto en QNX como en Linux, por lo que esta parte requiere pocos cambios.

### Comunicación con el hardware CAN:
    El código utiliza funciones como ConnectDriver, CanRead, CanRestart, etc., que son probablemente específicas de una librería QNX.
    Deberás sustituir estas llamadas por una librería CAN compatible con tu entorno (por ejemplo, SocketCAN en Linux).
### Colas de mensajes y pulsos:
    ChannelCreate, ConnectAttach, MsgReceivePulse_r, y SIGEV_PULSE_INIT son específicas de QNX. En otro sistema, puedes usar otras alternativas:
    Para Linux, considera usar mqueue para colas de mensajes y señales (signal.h) para manejar notificaciones.
### Temporización:
    Las funciones como nanosleep y timer_timeout son comunes en POSIX, pero algunos detalles de su uso pueden variar.
### Error Handling (errno):
    Aunque errno es común en POSIX, asegúrate de que la lógica de manejo de errores sea compatible con tu entorno.
### Interacción con hardware CAN:
    Asegúrate de adaptar el manejo de mensajes CAN (msg_can_in, CanRead, CanGetStatus, etc.) para trabajar con la nueva librería CAN en tu sistema operativo.
### Otros detalles menores:
    Algunos encabezados (sys/neutrino.h, sys/mman.h, etc.) no estarán disponibles en otros sistemas y deberán ser reemplazados por equivalentes.


_________________________________________________________________________________________


_________________________________________________________________________________________