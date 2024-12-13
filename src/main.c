/***************************************/
// Proyecto: FOX
// Nombre fichero: main.c
// Descripción: Función main y flujo principal del programa
// Autor: Echedey Aguilar Hernández
// email: eaguilar@us.es
// Fecha: 2024-11-27
// ***************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "can_manager.h"
#include "can_handler.h"

int main() {
    // Inicializar el bus CAN
    if (can_init() < 0) {
        return -1;
    }

    struct can_frame frame;
    while (1) {
        // Recibir un mensaje del bus CAN
        if (can_receive_message(&frame) > 0) {
            // Procesar el mensaje recibido
            process_can_message(&frame);
        }
    }

    // Cerrar el bus CAN
    can_close();
    return 0;
}
