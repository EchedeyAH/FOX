/***************************************/ 
// Proyecto: FOX 
// Nombre fichero: can_driver.h 
// Descripción: Funciones para la gestión de bajo nivel de la comunicación CAN 
// Autor: Echedey Aguilar Hernández 
// email: eaguilar@us.es 
// Fecha: 2024-12-11 
// ***************************************/ 

#ifndef CAN_DRIVER_H 
#define CAN_DRIVER_H 

#include <linux/can.h>

int can_driver_init(const char *interface_name);
int can_driver_send_message(uint32_t id, const uint8_t *data, uint8_t len);
int can_driver_receive_message(struct can_frame *frame);
void can_driver_close();

#endif // CAN_DRIVER_H
