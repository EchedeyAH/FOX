/***************************************/ 
// Proyecto: FOX 
// Nombre fichero: can_driver.c 
// Descripción: Implementación de funciones para la gestión de bajo nivel de la comunicación CAN 
// Autor: Echedey Aguilar Hernández 
// email: eaguilar@us.es 
// Fecha: 2024-12-11 
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

#define CAN_INTERFACE "can0"  // Adjust according to your configuration

static int can_socket;

int can_driver_init(const char *interface_name) {
    struct sockaddr_can addr;
    struct ifreq ifr;

    // Create a CAN socket
    can_socket = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (can_socket < 0) {
        perror("Error creating CAN socket");
        return -1;
    }

    // Get the network interface for CAN
    strcpy(ifr.ifr_name, interface_name);
    ioctl(can_socket, SIOCGIFINDEX, &ifr);
    
    // Configure the address of the CAN socket
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    // Bind the socket to the interface
    if (bind(can_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Error binding CAN socket");
        close(can_socket);
        return -1;
    }

    printf("CAN bus initialized successfully\n");
    return 0;
}

int can_driver_send_message(uint32_t id, const uint8_t *data, uint8_t len) {
    struct can_frame frame;
    frame.can_id = id;
    frame.can_dlc = len;
    memcpy(frame.data, data, len);

    // Send the message to the CAN bus
    if (write(can_socket, &frame, sizeof(struct can_frame)) != sizeof(struct can_frame)) {
        perror("Error sending CAN message");
        return -1;
    }

    return 0;
}

int can_driver_receive_message(struct can_frame *frame) {
    // Receive a message from the CAN bus
    int nbytes = read(can_socket, frame, sizeof(struct can_frame));
    if (nbytes < 0) {
        perror("Error receiving CAN message");
        return -1;
    }

    return nbytes;
}

void can_driver_close() {
    close(can_socket);
    printf("CAN bus closed\n");
}
