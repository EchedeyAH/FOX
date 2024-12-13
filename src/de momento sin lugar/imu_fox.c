/***************************************/
// Proyecto: FOX
// Nombre fichero: imu_fox.c
// Descripción: Descripción del código
// Autor: Echedey Aguilar Hernández
// email: eaguilar@us.es
// Fecha: 2024-11-27
// ***************************************/

#include "include/declaraciones_fox.h"
#include "include/funciones_fox.h"

void *imuMain(void *pi) {
    // Inicialización y configuración del hilo IMU
    // Lógica para abrir el puerto COM y comunicarse con la IMU
    printf("Hilo IMU iniciado\n");

    // Bucle para recibir y procesar datos de la IMU
    while (1) {
        // Lógica para recibir datos de la IMU
        // Procesar datos...
    }

    pthread_exit(NULL);
    return NULL;
}