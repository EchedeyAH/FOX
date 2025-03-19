#ifndef DECLARACIONES_FOX_H
#define DECLARACIONES_FOX_H

// -----------------------------------------------------------------------------
// Declaraciones de funciones comunes para ECU_FOX
// -----------------------------------------------------------------------------
//
// Se definen los prototipos de funciones esenciales para la inicialización, finalización,
// y manejo de hilos, manteniendo la interfaz del sistema original.
// -----------------------------------------------------------------------------

// Funciones de inicialización y finalización del sistema
int inicializarSistema(void);
void finalizarSistema(void);

// Prototipos de funciones para hilos (adaptados para usar pthreads en Linux)
void *comunicacion_thread(void *arg);
void *hilos_thread(void *arg);
void *impresion_thread(void *arg);

// Ejemplo de función de envío de un frame CAN (definida en el módulo drivers)
int enviar_frame_can(const can_frame_t *frame);

#endif // DECLARACIONES_FOX_H
