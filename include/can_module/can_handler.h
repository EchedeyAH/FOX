/***************************************/
// Proyecto: FOX
// Nombre fichero: can_handler.h
// Descripción: Este archivo manejará la lógica para procesar los mensajes recibidos.
// Autor: Echedey Aguilar Hernández
// email: eaguilar@us.es
// Fecha: 2024-12-11
// ***************************************/
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "can_manager.h"  // Incluir la cabecera de can_manager

void process_can_message(struct can_frame *frame) {
    // Aquí podemos implementar diferentes acciones dependiendo del ID del mensaje
    printf("Recibido mensaje CAN: ID=0x%X, DLC=%d, Datos=", frame->can_id, frame->can_dlc);
    
    for (int i = 0; i < frame->can_dlc; i++) {
        printf("0x%02X ", frame->data[i]);
    }
    printf("\n");

    // Ejemplo: Procesar mensaje con ID 0x123
    if (frame->can_id == 0x123) {
        // Realizar alguna acción específica con los datos recibidos
        printf("Procesando datos para el mensaje con ID 0x123\n");
    }
}
