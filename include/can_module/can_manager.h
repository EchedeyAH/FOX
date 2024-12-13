/***************************************/
// Proyecto: FOX
// Nombre fichero: can_manager.h
// Descripción: Funciones que se utilizarán en can_manager.c
// Autor: Echedey Aguilar Hernández
// email: eaguilar@us.es
// Fecha: 2024-12-11
// ***************************************/

#ifndef CAN_MANAGER_H
#define CAN_MANAGER_H

#include <linux/can.h>

int can_init();
int can_send_message(uint32_t id, uint8_t *data, uint8_t len);
int can_receive_message(struct can_frame *frame);
void can_close();

#endif
