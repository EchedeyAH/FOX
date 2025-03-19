#include <stdio.h>
#include <unistd.h>
#include "ecu_fox/constantes_fox.h"
#include "ecu_fox/declaraciones_fox.h"

void *hilos_thread(void *arg) {
    while (1) {
        // Simulación de trabajo en la gestión de hilos
        printf("Hilo de gestión de hilos funcionando...\n");
        sleep(1);
    }
    return NULL;
}
