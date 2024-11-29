/***************************************/
// Proyecto: FOX
// Nombre fichero: gestionPot_fox.c
// Descripción: Descripción del código
// Autor: Echedey Aguilar Hernández
// email: eaguilar@us.es
// Fecha: 2024-11-27
// ***************************************/
#include "include/declaraciones_fox.h"
#include "include/funciones_fox.h"

void *gestion_potencia_main(void *pi) {
    BOOL fin_local = 0;
    struct timespec espera = {0, TIEMPO_BUCLE_NS};

    // Indica que el hilo de gestión de potencia está listo
    pthread_mutex_lock(&mut_hilos_listos);
    hilos_listos |= HILO_GPOT_LISTO;
    pthread_mutex_unlock(&mut_hilos_listos);
    printf("Hilo GPOT listo\n");

    pthread_mutex_lock(&mut_inicio);
    pthread_cond_wait(&inicio, &mut_inicio);
    pthread_mutex_unlock(&mut_inicio);

    while (!fin_local) {
        // Espera el tiempo de muestreo establecido
        nanosleep(&espera, NULL);
        
        pthread_mutex_lock(&mut_hilos_listos);
        if ((hilos_listos & HILO_GPOT_LISTO) == 0x0000) {
            fin_local = 1;
        }
        pthread_mutex_unlock(&mut_hilos_listos);
    }

    pthread_exit(NULL);
    return NULL;
}