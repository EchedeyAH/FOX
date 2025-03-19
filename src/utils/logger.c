#include <stdio.h>
#include <stdlib.h>
#include "ecu_fox/funciones.h"

// Implementación de funciones utilitarias para logging y manejo de errores

void error_exit(const char *msg) {
    fprintf(stderr, "ERROR: %s\n", msg);
    exit(EXIT_FAILURE);
}

void log_message(const char *msg) {
    printf("[LOG] %s\n", msg);
}
