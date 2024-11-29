/***************************************/
// Proyecto: FOX
// Nombre fichero: declaraciones_fox.h
// Descripción: Definiciones de declaraciones
// Autor: Echedey Aguilar Hernández
// email: eaguilar@us.es
// Fecha: 2024-11-26
// ***************************************/

#ifndef DECLARACIONES_FOX_H
#define DECLARACIONES_FOX_H

#include "estructuras_fox.h"

// Declaraciones de funciones
int envia_can(int tipo_msg, int id_can);
int interpreta_can_entrada(int tipo_msg, struct can_object *p_can);
void *canMotor(void *cm);

#endif // DECLARACIONES_FOX_H