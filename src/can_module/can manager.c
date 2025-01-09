/***************************************/
// Proyecto: FOX
// Nombre fichero: can_manager.c
// Descripción: Gestión de alto nivel para la comunicación CAN
// Autor: Echedey Aguilar Hernández
// email: eaguilar@us.es
// Fecha: 2025-01-08
// ***************************************/

#include "can_driver.h"
#include <stdio.h>
#include <stdint.h>

#define MAX_CAN_DATA_LEN 8

typedef struct {
    uint32_t id;
    uint8_t data[MAX_CAN_DATA_LEN];
    uint8_t len;
} can_message_t;

int can_manager_init(const char *interface_name) {
    return can_driver_init(interface_name);
}

int can_manager_send_message(uint32_t id, const uint8_t *data, uint8_t len) {
    if (len > MAX_CAN_DATA_LEN) {
        fprintf(stderr, "Error: Longitud de datos (%d) excede el máximo permitido (%d).\n", len, MAX_CAN_DATA_LEN);
        return -1;
    }

    can_message_t message;
    message.id = id;
    message.len = len;
    memcpy(message.data, data, len);

    // Aquí se podrían aplicar reglas de negocio, filtrado, etc.

    return can_driver_send_message(message.id, message.data, message.len);
}

int can_manager_receive_message(can_message_t *message) {
    struct can_frame frame;
    int result = can_driver_receive_message(&frame);
    
    if (result >= 0) {
        message->id = frame.can_id;
        message->len = frame.can_dlc;
        memcpy(message->data, frame.data, frame.can_dlc);
        
        // Aquí se podrían aplicar reglas de negocio, filtrado, etc.
        
        printf("Mensaje CAN procesado - ID: 0x%X, Longitud: %d\n", message->id, message->len);
    }
    
    return result;
}

void can_manager_close() {
    can_driver_close();
}
