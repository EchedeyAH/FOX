/***************************************/
// Proyecto: FOX
// Nombre fichero: can_messages.h
// Descripción: Archivo para la definición de funciones relacionadas con el manejo de mensajes CAN
// Autor: Echedey Aguilar Hernández
// email: eaguilar@us.es
// Fecha: 2024-11-27
// ***************************************/

#ifndef CAN_MESSAGES_H
#define CAN_MESSAGES_H

void send_can_message(int message_type, void* data);
void receive_can_message(void);

#endif // CAN_MESSAGES_H