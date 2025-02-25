/***************************************/ 
// Proyecto: FOX 
// Nombre fichero: can_driver.c 
// Descripción: Implementación de funciones para la gestión de bajo nivel de la comunicación CAN 
// Autor: Echedey Aguilar Hernández 
// email: eaguilar@us.es 
// Fecha: 2024-12-11 
// ***************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include "can_manager.h"

#define CAN_INTERFACE "can0"

// Configurar SocketCAN
int setup_can_socket() {
    int sock;
    struct sockaddr_can addr;
    struct ifreq ifr;

    // Crear socket CAN
    sock = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (sock < 0) {
        perror("Error al crear socket CAN");
        return -1;
    }

    // Configurar la interfaz CAN
    strcpy(ifr.ifr_name, CAN_INTERFACE);
    if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
        perror("Error en ioctl");
        close(sock);
        return -1;
    }

    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    // Enlazar socket a la interfaz CAN
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Error al enlazar socket CAN");
        close(sock);
        return -1;
    }

    return sock;
}

// Enviar un mensaje CAN
int send_can_message(int sock, struct can_frame *frame) {
    if (write(sock, frame, sizeof(struct can_frame)) != sizeof(struct can_frame)) {
        perror("Error al enviar mensaje CAN");
        return -1;
    }
    return 0;
}

// Recibir un mensaje CAN
int receive_can_message(int sock, struct can_frame *frame) {
    int nbytes = read(sock, frame, sizeof(struct can_frame));
    if (nbytes < 0) {
        perror("Error al recibir mensaje CAN");
        return -1;
    }
    return nbytes;
}
