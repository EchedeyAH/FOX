#include <stdio.h>
#include <unistd.h>
#include <linux/can.h>
#include "ecu_fox/constantes_fox.h"
#include "ecu_fox/estructuras_fox.h"
#include "ecu_fox/declaraciones_fox.h"

// Ejemplo de función para enviar un frame CAN
int can_driver_send_frame(const struct can_frame *frame) {
    extern int can_socket;
    int nbytes = write(can_socket, frame, sizeof(struct can_frame));
    if (nbytes != sizeof(struct can_frame)) {
        perror("Error al enviar frame CAN");
        return ERROR_FAIL;
    }
    return ERROR_OK;
}
