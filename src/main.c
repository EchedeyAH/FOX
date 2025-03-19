#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "ecu_fox/constantes_fox.h"
#include "ecu_fox/declaraciones_fox.h"
#include "ecu_fox/funciones.h"

int main(int argc, char *argv[]) {
    // Inicialización del sistema
    if (inicializarSistema() != ERROR_OK) {
        fprintf(stderr, "Error inicializando el sistema.\n");
        exit(EXIT_FAILURE);
    }
    
    pthread_t hilo_com, hilo_hilos, hilo_imp;
    
    // Crear hilo de comunicación
    if(pthread_create(&hilo_com, NULL, comunicacion_thread, NULL) != 0) {
        error_exit("Error creando el hilo de comunicación");
    }
    
    // Crear hilo de gestión de hilos
    if(pthread_create(&hilo_hilos, NULL, hilos_thread, NULL) != 0) {
        error_exit("Error creando el hilo de hilos");
    }
    
    // Crear hilo de impresión/logging
    if(pthread_create(&hilo_imp, NULL, impresion_thread, NULL) != 0) {
        error_exit("Error creando el hilo de impresión");
    }
    
    // Espera a la finalización de los hilos
    pthread_join(hilo_com, NULL);
    pthread_join(hilo_hilos, NULL);
    pthread_join(hilo_imp, NULL);
    
    // Finalización del sistema
    finalizarSistema();
    
    return EXIT_SUCCESS;
}
