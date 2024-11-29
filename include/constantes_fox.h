/***************************************/
// Proyecto: FOX
// Nombre fichero: constantes_fox.h
// Descripción: Definiciones de constantes
// Autor: Echedey Aguilar Hernández
// email: eaguilar@us.es
// Fecha: 2024-11-26
// ***************************************/

#ifndef CONSTANTES_FOX_H
#define CONSTANTES_FOX_H

// Definiciones de IDs de controladores
#define CONTROLADOR_MOTOR_1 0x01 // ID del controlador del motor 1
#define CONTROLADOR_MOTOR_2 0x02 // ID del controlador del motor 2

// Definiciones de tipos de mensajes CAN
#define MSG_TIPO_01 0x100 // Tipo de mensaje para la función 1
#define MSG_TIPO_02 0x200 // Tipo de mensaje para la función 2

// Límites de control para la gestión de motores
#define LIMITE_CONT_MOTOR_G 100 // Límite para control de motor grave
#define LIMITE_CONT_MOTOR_C 200 // Límite para control de motor crítico

// Otros límites o constantes que podrías necesitar
#define TIEMPO_BUCLE_NS 50000000 // Tiempo de bucle en nanosegundos (50 ms)
#define NUM_MAX_MOTORES 4 // Número máximo de motores

#endif // CONSTANTES_FOX_H