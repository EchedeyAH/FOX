/***************************************/
// Proyecto: FOX
// Nombre fichero: estructuras_fox.h
// Descripción: Definiciones de estructuras
// Autor: Echedey Aguilar Hernández
// email: eaguilar@us.es
// Fecha: 2024-11-26
// ***************************************/

#ifndef ESTRUCTURAS_FOX_H
#define ESTRUCTURAS_FOX_H

#include <stdint.h>

typedef struct {
    float voltaje;
    float corriente;
    // Otras propiedades de la batería...
} est_bat_t;

typedef struct {
    uint32_t iTimeStamp;
    float pfAccelScal[3]; // Aceleración
    float pfGyroScal[3];  // Giroscopio
    double pdLLHPos[4];   // Posición
    // Otros campos...
} datosImu_t;

typedef struct {
    // Definiciones de datos de IMU
    datosImu_t datosImu;
} dato_cola_imu_t;

#endif // ESTRUCTURAS_FOX_H