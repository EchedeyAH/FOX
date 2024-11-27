/***************************************/
// Proyecto: FOX
// Nombre fichero: main.c
// Descripción: Función main y flujo principal del programa
// Autor: Echedey Aguilar Hernández
// email: eaguilar@us.es
// Fecha: 2024-11-27
// ***************************************/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <mqueue.h>
#include <signal.h>
#include <time.h>
#include "include/declaraciones_fox.h"
#include "include/funciones.h"

int main(int argc, char **argv) {
    // Inicialización y configuración
    inicializa_variables();
    bloquea_senales();
    selecciona_procesos(argc, argv);
    crea_colas_mensajes();
    crea_procesos();
    crea_hilos();

    // Bucle principal
    bucle_principal();

    // Cierre y limpieza
    cerrar_hilos_y_procesos();
    return 0;
}