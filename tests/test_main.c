#include <stdio.h>
#include "ecu_fox/declaraciones_fox.h"

int main(void) {
    printf("Ejecutando pruebas unitarias para ECU_FOX...\n");
    
    if (inicializarSistema() != ERROR_OK) {
        printf("Error en la inicialización del sistema.\n");
        return 1;
    }
    printf("Inicialización exitosa.\n");
    
    finalizarSistema();
    printf("Finalización exitosa.\n");
    return 0;
}
