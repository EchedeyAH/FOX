/**************************************************************************
***									***	
***	Fichero: can2_fox.h						***
***	Fecha: 07/03/2013						***
***	Autor: Elena Gonzalez						***
***	Descripción: Cabecera de proceso de adquisicion de datos por 	***
***	CAN para el sistema de control de las baterias (BMS).		***
***									***
**************************************************************************/

#ifndef _CAN2_FOX_H_
#define _CAN2_FOX_H_

	/* CONSTANTES GENERALES */
	#ifndef TRUE 
	#define TRUE 1 
	#endif
	
	#ifndef FALSE 
	#define FALSE 0 
	#endif

	#ifndef BYTE
	#define BYTE uint8_t
	#endif
	
	#ifndef BOOL
	#define BOOL BYTE
	#endif
	
	#define TEMPO_CAN	500000000L
	#define TIEMPO_EXP_CAN_X 20
	#define TIMEOUT_COLA 10
	#define TEMPO_TIMEOUT_CAN 30	// Si no se recibe un msg can de la BMS en 30 seg da error de com. y termina el proceso

	/* CONSTANTES DE LA BATERIA */
	#define NUM_CEL_BAT 24

	/* IDENTIFICADORES CAN DE ENTRADA (BMS) */
	#define ID_CAN_BMS 0x04D

	/* DEFINICIONES PARA LA RECEPCION CAN - TIPOS DE MENSAJE */
	#define VOLTAJE_T 118	
	#define TEMPERATURA_T 116
	#define ESTADO_T 37
	#define ALARMA_T 115
	#define NUM_MAX_DIG_VALUE 5
	#define NUM_MAX_DIG_INDEX 2
	/* DEFINICIONES PARA LA RECEPCION CAN - TIPOS ALARMA */
	/* Niveles de alarma */
	#define NO_ALARMA 0
	#define WARNING 1
	#define ALARMA 2
	#define ALARMA_CRITICA 3
	/* Tipos de alarma */
	#define PACK_OK 1
	#define CELL_TEMP_HIGH 2
	#define PACK_V_HIGH 3
	#define PACK_V_LOW 4
	#define PACK_I_HIGH 5
	#define CHASIS_CONECT 6
	#define CELL_COM_ERROR 7
	#define SYSTEM_ERROR 8

	/* DEFINICIONES PARA LAS COLAS */
	#define NOMBRE_COLA "/COLA_CAN2"
	#define TAMANHO_COLA_CAN2 200


	/* ESTRUCTURA DE DATOS ENVIADA AL HILO PRINCIPAL */
	typedef struct {
		// Errores de comunicacion
		BOOL error_com_tcan;		// Vale 0 si no hay error de comunicacion con la tarjeta CAN, 1 si hay error.
		BOOL error_com_bms;		// Vale 0 si no hay error de comunicacion con la BMS via CAN, 1 si hay error.
		// Estado general de las baterias
		uint8_t num_cel_scan;		// Numero de celdas de la bateria detectadas
		uint8_t temp_media;		// Temperatura media de celda
		uint16_t v_medio;			// Voltaje medio de celda
		uint32_t v_pack;			// Voltaje del pack de baterias
		int16_t i_pack;				// Corriente del pack de baterias
		uint8_t soc;			// Estado de carga de baterias
		uint16_t timestamp;		// Timestamp de la BMS
		uint8_t temp_max;			// Temperatura maxima de celda
		uint8_t cel_temp_max;		// Celda con temperatura maxima
		uint16_t v_max;			// Voltaje maximo de celda
		uint8_t cel_v_max;		// Celda con voltaje maximo
		uint16_t v_min;			// Voltaje minimo de celda
		uint8_t cel_v_min;		// Celda con voltaje minimo
		uint8_t nivel_alarma;		// Indica si la alarma es nula, leve, grave o critica
		uint8_t alarma;			// Indica la alarma vigente
		// Estado detallado de las baterias
		uint16_t mv_cel[NUM_CEL_BAT];	// Voltaje en mV en cada celda
		uint8_t temp_cel[NUM_CEL_BAT];	// Temperatura en ºC en cada celda
	}dato_cola_t;
	
	/* ESTRUCTURA DE DATOS RECIBIDA DEL PROGRAMA PRINCIPAL */
	typedef struct {
		uint8_t fin_programa;	// El hilo principal envia un 0 en caso normal, un 1 en caso de fin de programa */
	}dato_ecu_t;
	
	
	/* Prototipos de funciones */
	void interpreta_can_entrada(struct can_object *);
	
#endif
