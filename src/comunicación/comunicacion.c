#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <linux/can.h>
#include "ecu_fox/constantes_fox.h"
#include "ecu_fox/declaraciones_fox.h"

// Se asume que 'can_socket' se inicializa en el módulo de inicialización.
extern int can_socket;

void *comunicacion_thread(void *arg) {
    struct can_frame frame;
    while (1) {
        // Lectura de un frame CAN desde el socket (SocketCAN)
        int nbytes = read(can_socket, &frame, sizeof(struct can_frame));
        if (nbytes > 0) {
            printf("Recibido frame CAN con ID: %d\n", frame.can_id);
        } else if (nbytes < 0) {
            perror("Error en la lectura del socket CAN");
        }
        sleep(1);
    }
    return NULL;
}
