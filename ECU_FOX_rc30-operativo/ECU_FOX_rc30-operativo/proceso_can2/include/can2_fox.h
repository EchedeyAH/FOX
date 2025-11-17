/**************************************************************************
***									***	
***	Fichero: can2_fox.h						***
***	Fecha: 07/03/2013						***
***	Autor: Elena Gonzalez						***
***	Descripción: Cabecera de proceso de adquisicion de datos por 	***
***	CAN para el sistema de control de las baterias (BMS).		***
***									***
**************************************************************************/

#ifndef _CAN2_FOX_H_  // Si no se ha definido aún el encabezado _CAN2_FOX_H_, se define
#define _CAN2_FOX_H_

	/* CONSTANTES GENERALES */
	#ifndef TRUE 
	#define TRUE 1  // Define el valor 1 para representar TRUE
	#endif
	
	#ifndef FALSE 
	#define FALSE 0  // Define el valor 0 para representar FALSE
	#endif

	#ifndef BYTE
	#define BYTE uint8_t  // Define el tipo BYTE como uint8_t (un byte sin signo)
	#endif
	
	#ifndef BOOL
	#define BOOL BYTE  // Define BOOL como un alias de BYTE para representaciones booleanas
	#endif
	
	// Definiciones de temporizadores y timeouts
	#define TEMPO_CAN	500000000L  // Tiempo relacionado con la comunicación CAN
	#define TIEMPO_EXP_CAN_X 20  // Tiempo de espera de respuesta CAN
	#define TIMEOUT_COLA 10  // Tiempo de espera para la cola de mensajes CAN
	#define TEMPO_TIMEOUT_CAN 30  // Si no se recibe mensaje CAN en 30 segundos, se da error de comunicación

	/* CONSTANTES DE LA BATERIA */
	#define NUM_CEL_BAT 24  // Número de celdas en la batería (24 celdas)

	/* IDENTIFICADORES CAN DE ENTRADA (BMS) */
	#define ID_CAN_BMS 0x04D  // Identificador del mensaje CAN para la BMS (Battery Management System)

	/* DEFINICIONES PARA LA RECEPCION CAN - TIPOS DE MENSAJE */
	#define VOLTAJE_T 118  // Identificador para el mensaje de voltaje
	#define TEMPERATURA_T 116  // Identificador para el mensaje de temperatura
	#define ESTADO_T 37  // Identificador para el mensaje de estado
	#define ALARMA_T 115  // Identificador para el mensaje de alarma
	#define NUM_MAX_DIG_VALUE 5  // Número máximo de valores digitales que se pueden manejar
	#define NUM_MAX_DIG_INDEX 2  // Número máximo de índices digitales que se pueden manejar
	
	/* DEFINICIONES PARA LA RECEPCION CAN - TIPOS ALARMA */
	/* Niveles de alarma */
	#define NO_ALARMA 0  // No hay alarma
	#define WARNING 1  // Alarma leve
	#define ALARMA 2  // Alarma grave
	#define ALARMA_CRITICA 3  // Alarma crítica
	
	/* Tipos de alarma */
	#define PACK_OK 1  // El pack de baterías está en estado normal
	#define CELL_TEMP_HIGH 2  // Alarma por temperatura alta de celda
	#define PACK_V_HIGH 3  // Alarma por voltaje alto del pack
	#define PACK_V_LOW 4  // Alarma por voltaje bajo del pack
	#define PACK_I_HIGH 5  // Alarma por corriente alta del pack
	#define CHASIS_CONECT 6  // Alarma por conexión del chasis
	#define CELL_COM_ERROR 7  // Error de comunicación con una celda
	#define SYSTEM_ERROR 8  // Error general del sistema

	/* DEFINICIONES PARA LAS COLAS */
	#define NOMBRE_COLA "/COLA_CAN2"  // Nombre de la cola para la comunicación CAN
	#define TAMANHO_COLA_CAN2 200  // Tamaño de la cola para la comunicación CAN (200 elementos)

	/* ESTRUCTURA DE DATOS ENVIADA AL HILO PRINCIPAL */
	// Esta estructura contiene los datos que serán enviados al hilo principal para procesarlos
	typedef struct {
		// Errores de comunicación
		BOOL error_com_tcan;  // 0 si no hay error de comunicación con la tarjeta CAN, 1 si hay error
		BOOL error_com_bms;  // 0 si no hay error de comunicación con la BMS a través de CAN, 1 si hay error
		// Estado general de las baterías
		uint8_t num_cel_scan;  // Número de celdas de batería detectadas
		uint8_t temp_media;  // Temperatura media de la celda (en grados Celsius)
		uint16_t v_medio;  // Voltaje medio de las celdas (en milivoltios)
		uint32_t v_pack;  // Voltaje total del pack de baterías (en voltios)
		int16_t i_pack;  // Corriente del pack de baterías (en amperios)
		uint8_t soc;  // Estado de carga del pack de baterías (0-100%)
		uint16_t timestamp;  // Timestamp que marca el momento de la adquisición de los datos
		uint8_t temp_max;  // Temperatura máxima de la celda
		uint8_t cel_temp_max;  // Índice de la celda con la temperatura máxima
		uint16_t v_max;  // Voltaje máximo de las celdas
		uint8_t cel_v_max;  // Índice de la celda con el voltaje máximo
		uint16_t v_min;  // Voltaje mínimo de las celdas
		uint8_t cel_v_min;  // Índice de la celda con el voltaje mínimo
		uint8_t nivel_alarma;  // Nivel de la alarma (NO_ALARMA, WARNING, ALARMA, ALARMA_CRITICA)
		uint8_t alarma;  // Tipo de alarma activa (PACK_OK, CELL_TEMP_HIGH, etc.)
		// Estado detallado de las baterías
		uint16_t mv_cel[NUM_CEL_BAT];  // Voltaje de cada celda en milivoltios
		uint8_t temp_cel[NUM_CEL_BAT];  // Temperatura de cada celda en grados Celsius
	} dato_cola_t;
	
	/* ESTRUCTURA DE DATOS RECIBIDA DEL PROGRAMA PRINCIPAL */
	// Esta estructura es utilizada para recibir los datos del programa principal, especialmente el estado de finalización del proceso
	typedef struct {
		uint8_t fin_programa;  // 0 si el programa principal sigue en ejecución, 1 si se ha finalizado
	} dato_ecu_t;
	
	
	/* Prototipos de funciones */
	// Función que interpreta los mensajes CAN de entrada
	// La función recibe un objeto de tipo can_object (definido en otro lugar) para procesar los mensajes CAN
	void interpreta_can_entrada(struct can_object *);

#endif  // Fin de la definición del encabezado _CAN2_FOX_H_
