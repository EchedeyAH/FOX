/***************************************/
// Proyecto: FOX
// Nombre fichero: can_config.h
// Descripción: Archivo para las configuraciones del bus CAN
// Autor: Echedey Aguilar Hernández
// email: eaguilar@us.es
// Fecha: 2024-11-27
// ***************************************/

#ifndef CAN_CONFIG_H
#define CAN_CONFIG_H

void init_can_bus(void);
void set_can_bitrate(int bitrate);

#endif // CAN_CONFIG_H