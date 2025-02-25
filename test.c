#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>

#ifdef __linux__
    #include <linux/if.h>
#endif

#define CAN_INTERFACE "emuccan1"  // Cambia esto si usas otra interfaz

int main() {
    int sock;
    struct sockaddr_can addr;
    struct ifreq ifr;
    struct can_frame frame;

    // Crear socket CAN
    sock = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (sock < 0) {
        perror("Error al crear el socket CAN");
        return EXIT_FAILURE;
    }

    // Configurar la interfaz CAN
    strcpy(ifr.ifr_name, CAN_INTERFACE);
    if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
        perror("Error al obtener el índice de la interfaz CAN");
        close(sock);
        return EXIT_FAILURE;
    }

    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    // Enlazar el socket a la interfaz CAN
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Error al enlazar el socket CAN");
        close(sock);
        return EXIT_FAILURE;
    }

    printf("Esperando mensajes CAN en %s...\n", CAN_INTERFACE);

    // Leer mensajes CAN
    while (1) {
        int nbytes = read(sock, &frame, sizeof(struct can_frame));
        if (nbytes < 0) {
            perror("Error al leer mensaje CAN");
            break;
        }

        printf("ID: 0x%X DLC: %d Data: ", frame.can_id, frame.can_dlc);
        for (int i = 0; i < frame.can_dlc; i++) {
            printf("%02X ", frame.data[i]);
        }
        printf("\n");
    }

    close(sock);
    return EXIT_SUCCESS;
}