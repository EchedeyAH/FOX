#include <stdio.h>
#include <unistd.h>
#include "ecu_fox/constantes_fox.h"
#include "ecu_fox/declaraciones_fox.h"

void *impresion_thread(void *arg) {
    while (1) {
        // Ejemplo de impresión o logging periódico
        printf("Hilo de impresión/logging activo...\n");
        sleep(2);
    }
    return NULL;
}
