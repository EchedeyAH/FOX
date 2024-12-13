/***************************************/
// Proyecto: FOX
// Nombre fichero: comunicacion.c
// Descripción: Funciones para la comunicación entre procesos y el manejo de colas de mensajes.
// Autor: Echedey Aguilar Hernández
// email: eaguilar@us.es
// Fecha: 2024-11-26
// ***************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include <stdint.h>
#include "include/constantes_fox.h"
#include "include/estructuras_fox.h"
#include "include/declaraciones_fox.h"
void crea_colas_mensajes() {
    // Crear y configurar colas de mensajes
    mqd_t cola_can2;
    struct mq_attr attr;

    attr.mq_flags = 0;
    attr.mq_maxmsg = 10; // Número máximo de mensajes
    attr.mq_msgsize = sizeof(est_bat_t); // Tamaño del mensaje
    attr.mq_curmsgs = 0;

    cola_can2 = mq_open("NOMBRE_COLA_CAN2", O_CREAT | O_RDWR, 0644, &attr);
    if (cola_can2 == (mqd_t)-1) {
        perror("Error al crear cola CAN2");
    }
}

void recibe_datos_cola_can2(mqd_t cola) {
    // Lógica para recibir datos de la cola
    est_bat_t dato_cola;
    if (mq_receive(cola, (char *)&dato_cola, sizeof(dato_cola), NULL) == -1) {
        perror("Error al recibir datos de la cola CAN2");
    }
    // Procesar los datos recibidos...
}

void envia_datos_cola_imu(mqd_t cola) {
    // Lógica para enviar datos a la cola
    dato_cola_imu_t dato;
    // Preparar datos a enviar...
    if (mq_send(cola, (char *)&dato, sizeof(dato), 0) == -1) {
        perror("Error al enviar datos a la cola IMU");
    }
}