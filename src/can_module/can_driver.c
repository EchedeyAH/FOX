/***************************************/
// Proyecto: FOX
// Nombre fichero: can_driver.c
// Descripción: Driver de bajo nivel para la comunicación CAN
// Autor: Echedey Aguilar Hernández
// email: eaguilar@us.es
// Fecha: 2025-01-08
// ***************************************/

#include "can_driver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/socket.h>
#include <net/if.h>
#include <errno.h>

static int can_socket = -1;

int can_driver_init(const char *interface_name) {
    if (!interface_name) {
        fprintf(stderr, "Error: Nombre de interfaz CAN nulo.\n");
        return -1;
    }

    struct sockaddr_can addr;
    struct ifreq ifr;

    can_socket = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (can_socket < 0) {
        perror("Error al crear el socket CAN");
        return -1;
    }

    strncpy(ifr.ifr_name, interface_name, IFNAMSIZ - 1);
    if (ioctl(can_socket, SIOCGIFINDEX, &ifr) < 0) {
        perror("Error al obtener el índice de la interfaz CAN");
        close(can_socket);
        return -1;
    }

    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(can_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Error al enlazar el socket CAN");
        close(can_socket);
        return -1;
    }

    printf("Bus CAN inicializado correctamente en la interfaz %s\n", interface_name);
    return 0;
}

int can_driver_send_message(uint32_t id, const uint8_t *data, uint8_t len) {
    if (can_socket < 0) {
        fprintf(stderr, "Error: El bus CAN no está inicializado.\n");
        return -1;
    }

    struct can_frame frame;
    frame.can_id = id;
    frame.can_dlc = len;
    memcpy(frame.data, data, len);

    if (write(can_socket, &frame, sizeof(struct can_frame)) != sizeof(struct can_frame)) {
        perror("Error al enviar mensaje CAN");
        return -1;
    }

    return 0;
}

int can_driver_receive_message(struct can_frame *frame) {
    if (can_socket < 0) {
        fprintf(stderr, "Error: El bus CAN no está inicializado.\n");
        return -1;
    }

    int nbytes = read(can_socket, frame, sizeof(struct can_frame));
    if (nbytes < 0) {
        perror("Error al recibir mensaje CAN");
        return -1;
    }

    return nbytes;
}

void can_driver_close() {
    if (can_socket >= 0) {
        close(can_socket);
        can_socket = -1;
    }
}
