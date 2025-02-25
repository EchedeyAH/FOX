/***************************************/ 
// Proyecto: FOX 
// Nombre fichero: can_manager.c 
// Descripción: Gestión de alto nivel para la comunicación CAN 
// Autor: Echedey Aguilar Hernández 
// email: eaguilar@us.es 
// Fecha: 2025-01-08 
/***************************************/

#include <stdio.h>
#include "can_manager.h"

// Inicializar el módulo CAN
void initialize_can() {
    int sock = setup_can_socket();
    if (sock < 0) {
        printf("Error al inicializar el socket CAN\n");
        return;
    }
    printf("Socket CAN inicializado correctamente.\n");
}

// Enviar mensaje de prueba
void test_can_send() {
    int sock = setup_can_socket();
    struct can_frame frame;

    frame.can_id = 0x123;
    frame.can_dlc = 2;
    frame.data[0] = 0xAB;
    frame.data[1] = 0xCD;

    send_can_message(sock, &frame);
    printf("Mensaje CAN enviado.\n");

    close(sock);
}
