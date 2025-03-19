#ifndef FUNCIONES_H
#define FUNCIONES_H

#include "constantes_fox.h"
#include "estructuras_fox.h"

// -----------------------------------------------------------------------------
// Funciones auxiliares y utilitarias para ECU_FOX
// -----------------------------------------------------------------------------
//
// Estas funciones proporcionan mecanismos para el manejo de errores y el registro de
// mensajes, reemplazando en Linux los mecanismos específicos de QNX utilizados en el
// proyecto original.
// -----------------------------------------------------------------------------

/**
 * @brief Muestra un mensaje de error y termina el programa.
 *
 * Esta función imprime el mensaje de error en stderr y finaliza la ejecución del programa.
 *
 * @param msg Mensaje de error a mostrar.
 */
void error_exit(const char *msg);

/**
 * @brief Registra o imprime un mensaje de log.
 *
 * Permite registrar eventos importantes o mensajes de diagnóstico.
 *
 * @param msg Mensaje a registrar.
 */
void log_message(const char *msg);

#endif // FUNCIONES_H
