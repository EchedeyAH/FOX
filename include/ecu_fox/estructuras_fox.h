#ifndef ESTRUCTURAS_FOX_H
#define ESTRUCTURAS_FOX_H

#include <linux/can.h>

// -----------------------------------------------------------------------------
// Estructuras de datos para ECU_FOX
// -----------------------------------------------------------------------------
//
// Aquí se definen las estructuras que se utilizarán para representar frames CAN,
// información de hilos y otros parámetros de configuración o estado global, tal como
// se usaban en el proyecto original.
// -----------------------------------------------------------------------------

// Estructura para representar un frame CAN (simplificada)
typedef struct {
    int id;                         // Identificador del frame CAN
    unsigned char data[8];          // Datos del frame (8 bytes, como es estándar en CAN)
    int dlc;                        // Data Length Code: número de bytes válidos en data
} can_frame_t;

// Estructura para almacenar información de un hilo (para monitoreo o debugging)
typedef struct {
    int thread_id;                  // Identificador lógico del hilo
    char nombre[32];                // Nombre descriptivo del hilo
    int estado;                     // Estado del hilo (por ejemplo: 0 = inactivo, 1 = activo, -1 = error)
} thread_info_t;

// Estructura para parámetros de configuración global
typedef struct {
    int parametro1;                 // Ejemplo: parámetro numérico (ajustar según sea necesario)
    float parametro2;               // Ejemplo: otro parámetro de configuración
    // Se pueden agregar más campos según la documentación y requerimientos del proyecto
} config_t;

#endif // ESTRUCTURAS_FOX_H
