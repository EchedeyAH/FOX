#ifndef CONSTANTES_FOX_H
#define CONSTANTES_FOX_H

// -----------------------------------------------------------------------------
// Constantes globales para el proyecto ECU_FOX
// -----------------------------------------------------------------------------
//
// Estas definiciones se utilizan en todo el sistema para establecer parámetros
// generales, como tamaños de buffers, nombres de interfaces, tiempos de espera y
// códigos de error.
//
// -----------------------------------------------------------------------------

// Tamaño máximo del buffer para comunicaciones (por ejemplo, en mensajes CAN)
#define MAX_BUFFER_SIZE 1024

// Nombre de la interfaz CAN a utilizar (SocketCAN en Linux)
#define CAN_INTERFACE "can0"

// Códigos de error generales
#define ERROR_OK 0
#define ERROR_FAIL -1

// Tiempo de espera en segundos para ciclos o temporizadores
#define TIEMPO_ESPERA 1

// Ejemplo de prioridad de hilos (si se utiliza en el proyecto original)
#define PRIORIDAD_HILOS 50

#endif // CONSTANTES_FOX_H
