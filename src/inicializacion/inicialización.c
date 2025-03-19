#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include "ecu_fox/constantes_fox.h"
#include "ecu_fox/declaraciones_fox.h"

int can_socket = -1;

int inicializarSistema(void) {
    struct ifreq ifr;
    struct sockaddr_can addr;

    // Creación del socket CAN utilizando SocketCAN
    can_socket = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (can_socket < 0) {
        perror("Error al crear el socket CAN");
        return ERROR_FAIL;
    }
    
    // Configuración de la interfaz CAN (por defecto "can0")
    strncpy(ifr.ifr_name, CAN_INTERFACE, IFNAMSIZ - 1);
    if (ioctl(can_socket, SIOCGIFINDEX, &ifr) < 0) {
        perror("Error al obtener el índice de la interfaz CAN");
        close(can_socket);
        return ERROR_FAIL;
    }
    
    // Enlazamos el socket a la interfaz CAN
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    if (bind(can_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Error al enlazar el socket CAN");
        close(can_socket);
        return ERROR_FAIL;
    }
    
    printf("Socket CAN inicializado correctamente en la interfaz %s\n", CAN_INTERFACE);
    return ERROR_OK;
}

void finalizarSistema(void) {
    if (can_socket != -1) {
        close(can_socket);
        can_socket = -1;
    }
    printf("Sistema finalizado correctamente.\n");
}
