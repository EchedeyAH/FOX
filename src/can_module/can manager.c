/***************************************/
// Proyecto: FOX
// Nombre fichero: can_manager.c
// Descripción: Funciones para inicializar el bus CAN, recibir y enviar mensajes utilizando SocketCAN
// Autor: Echedey Aguilar Hernández
// email: eaguilar@us.es
// Fecha: 2024-12-11
// ***************************************/
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

#define CAN_INTERFACE "can0"  // El nombre de la interfaz CAN (ajustar según configuración)

int can_socket;

int can_init() {
    struct sockaddr_can addr;
    struct ifreq ifr;

    // Crear un socket CAN
    can_socket = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (can_socket < 0) {
        perror("Error al crear el socket CAN");
        return -1;
    }

    // Obtener la interfaz de red para el CAN
    strcpy(ifr.ifr_name, CAN_INTERFACE);
    ioctl(can_socket, SIOCGIFINDEX, &ifr);
    
    // Configurar la dirección del socket CAN
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    // Enlazar el socket a la interfaz
    if (bind(can_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Error al enlazar el socket CAN");
        close(can_socket);
        return -1;
    }

    printf("Bus CAN inicializado correctamente\n");
    return 0;
}

int can_send_message(uint32_t id, uint8_t *data, uint8_t len) {
    struct can_frame frame;
    frame.can_id = id;
    frame.can_dlc = len;
    memcpy(frame.data, data, len);

    // Enviar el mensaje al bus CAN
    if (write(can_socket, &frame, sizeof(struct can_frame)) != sizeof(struct can_frame)) {
        perror("Error al enviar mensaje CAN");
        return -1;
    }

    return 0;
}

int can_receive_message(struct can_frame *frame) {
    // Recibir un mensaje desde el bus CAN
    int nbytes = read(can_socket, frame, sizeof(struct can_frame));
    if (nbytes < 0) {
        perror("Error al recibir mensaje CAN");
        return -1;
    }

    return nbytes;
}

void can_close() {
    close(can_socket);
    printf("Bus CAN cerrado\n");
}
