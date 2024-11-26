/**********************************************************************************
***										***	
***	Fichero: constantes_fox.h						***
***	Fecha: 22/02/2013							***
***	Autor: Elena Gonzalez							***
***	DescripciÃ³n: Cabecera de definicion de constantes			***
***										***
**********************************************************************************/

#ifndef _CONSTANTES_FOX_H_
#define _CONSTANTES_FOX_H_
	
	/**********************************************************
	***		HILO PRINCIPAL - ECU			***
	**********************************************************/
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
	
	#define NUM_MOTORES 4
	#define POS_M1 0
	#define POS_M2 1
	#define POS_M3 2
	#define POS_M4 3
	#define NOMBRE_COLA_CAN2 "/COLA_CAN2"
	#define NOMBRE_COLA_IMU "/COLA_IMU"
	#define TAMANHO_COLA_CAN2	200
	#define TAMANHO_COLA_IMU	500
	#define VERSION_ECU 300
	#define PRIO_IMU 20
	#define PRIO_CAN2 10
	#define SIG_CAN_SUP SIGRTMIN
	#define NUM_ERR_MAX_COLA 10

	/* TIEMPOS BASE Y SUS MULTIPLOS */
	#define TIEMPO_BUCLE_NS 		50000000L	// Tiempo base para espera del bucle principal de la ECU (50 ms).
	#define TIEMPO_INI_HILOS_NS 	500000000L	// Tiempo base para espera de creacion de hilos en funcion principal (500 ms).
	#define TIEMPO_INI_HILOS_X 		60			// Multiplo para TIEMPO_INI_HILOS_NS
	#define TIEMPO_CAN_SUP_NS 		250000000L	// Tiempo base para temporizador de salida de mensajes CAN al supervisor (250 ms).
	#define TEMPO_CAN				500000000L
	#define TIEMPO_DRV_CAN_S	2			//Tiempo espera al driver CAN (2 s)
	#define TIEMPO_FIN_S		3			//Tiempo de espera al fin de los procesos (3 s)
	#define TIEMPO_PROC_S		3		//Tiempo de espera a arranque de procesos
	#define TIEMPO_EXP_MOT_S 	2		// Tiempo en el que expira la espera de respuesta de CAN de los motores (2 s).
	#define TIEMPO_EXP_COLA_S 	30		// Tiempo de timeout para colas en segundos

	#define TIEMPO_EXP_CAN_X 	50		// Numero de iteraciones permitidas en el arranque del driver antes de dar error
	
	
	/* MENSAJES */
	#define MENSAJE_INICIO 		"Iniciando ECU..."
	#define MENSAJE_VERSION		"ECU Version "
	/* #define MENSAJE_STANDBY 	"Estado de FC: Standby Mode" */
	#define MENSAJE_ARRANQUE 	"Secuencia de Arranque"
	#define MENSAJE_PARADA 		"Secuencia de Parada"
	#define MENSAJE_EMERGENCIA 	"Parada de Emergencia"
	#define MENSAJE_FIN			"Apagando ECU..."
	
	/* UBICACION DE LOS FICHEROS */
	#define PATH_DRIVER_CAN 	"/ecu/driver_can/can_dual"
	#define PATH_PROC_IMU 		"/ecu/imu_datalog_fox"
	#define PATH_PROC_CAN2 		"/ecu/can2_fox"
	#define PATH_CONFIG_TAD 	"/etc/pcm3718hg_ho.cfg"

	// SELECCION DE LOS PROCESOS E HILOS
	#define	ON_TODOS			0xFFFF
	#define	OFF_TODOS			0x0000
	#define	ON_HILO_IMU       	(1<<0)
	#define	ON_HILO_CANMOTOR1  	(1<<1)
	#define	ON_HILO_CANMOTOR2  	(1<<2)
	#define	ON_HILO_CANMOTOR3  	(1<<3)
	#define	ON_HILO_CANMOTOR4  	(1<<4)
	#define	ON_HILO_CANRX      	(1<<5)
	#define	ON_HILO_CANSUPERV  	(1<<6)
	#define	ON_HILO_CTRLTRAC	(1<<7)
	#define	ON_HILO_CTRLEST		(1<<8)
	#define	ON_HILO_GESTPOT		(1<<9)
	#define	ON_HILO_ADQAO      	(1<<10)
	#define	ON_HILO_ADQ        	(1<<11)
	#define	ON_HILO_ERRORES    	(1<<12)
	#define	ON_PROC_IMUDATA		(1<<13)
	#define	ON_PROC_CANBMS     	(1<<14)
	#define	ON_IMPRIME	        (1<<15)
	
	/* CONDICIONES DE ARRANQUE DE HILOS - ESPERA */
	#define HILOS_OFF			0x0000
	#define HILOS_BASE_OK		0x0C21
	#define HILOS_MOTORES_OK	0x001E
	#define HILOS_OK 			0x1FFF
	#define HILO_IMU_LISTO 		(1<<0)
	#define HILO_MOT1_LISTO 	(1<<1)
	#define HILO_MOT2_LISTO 	(1<<2)
	#define HILO_MOT3_LISTO 	(1<<3)
	#define HILO_MOT4_LISTO 	(1<<4)
	#define HILO_CANRX_LISTO 	(1<<5)
	#define HILO_SUPERV_LISTO	(1<<6)
	#define HILO_TRAC_LISTO 	(1<<7)
	#define HILO_ESTAB_LISTO	(1<<8)
	#define HILO_GPOT_LISTO 	(1<<9)
	#define HILO_TADAO_LISTO	(1<<10)
	#define HILO_TAD_LISTO 		(1<<11)
	#define HILO_ERR_LISTO 		(1<<12)
	
	/* CONSTANTES BATERIA */
	#define NUM_CEL_BAT	24
	
	/* DEFINICION DE ESTADOS DE LA BATERIA */
	#define BATT_CARGA_BAJA		1	// Estado de carga de bateria baja.
	#define BATT_CARGA_MEDIA	2	// Estado de carga de bateria media.
	#define BATT_CARGA_ALTA		3	// Estado de carga de bateria alta.
	#define BATT_CARGA_COMPLETA	4	// Bateria llena.
	#define BATT_M_B			15	// Punto de transicion de estado MEDIA a BAJA.
	#define BATT_B_M			17	// Punto de transicion de estado BAJA a MEDIA.
	#define BATT_A_M			60	// Punto de transicion de estado ALTA a MEDIA.
	#define BATT_M_A			65	// Punto de transicion de estado MEDIA a ALTA.
	#define BATT_F_A			90	// Punto de transicion de estado FULL a ALTA.
	#define BATT_A_F			95	// Punto de transicion de estado ALTA a FULL.
	
	/* CONSTANTES POTENCIA */
	#define POTMINFC		1000
	#define POTMAXFC		56000
	#define POTMAXBATR		-18000
	#define POTMAXBAT		18000
	#define P_AUX			-1000
	#define DELTA_FC_P_MIN	-4000
	#define DELTA_FC_P_MAX	4000
	#define DELTA_BAT_P_MIN	-4000
	#define DELTA_BAT_P_MAX	4000
	
	/* DEFINICIONES PARA LA RECEPCION CAN DE BMS - TIPOS ALARMA */
	/* Niveles de alarma */
	#define NO_ALARMA 		0
	#define WARNING 		1
	#define ALARMA 			2
	#define ALARMA_CRITICA 	3
	/* Tipos de alarma */
	#define PACK_OK 		1
	#define CELL_TEMP_HIGH 	2
	#define PACK_V_HIGH 	3
	#define PACK_V_LOW 		4
	#define PACK_I_HIGH 	5
	#define CHASIS_CONECT 	6
	#define CELL_COM_ERROR 	7
	#define SYSTEM_ERROR 	8
	
	
	/**********************************************************
	***			HILO ERRORES			***
	**********************************************************/
	
	/* CONSTANTES */
	#define MOTOR_V_AUX_H_LEVE 6	// Margen sup del rango de tension aux(5v) proporcionada por los controladores de los motores [V]
	#define MOTOR_V_AUX_L_LEVE 4	// Margen inf del rango de tension aux(5v) proporcionada por los controladores de los motores [V]
	#define MOTOR_V_ALTA_GRAVE 95	// Umbral de error grave para tension alta de baterias detectada por controlador de motores [V]
	#define MOTOR_V_ALTA_LEVE 90	// Umbral de error leve para tension alta de baterias detectada por controlador de motores [V]
	#define MOTOR_V_BAJA_GRAVE 50	// Umbral de error grave para tension baja de baterias detectada por controlador de motores [V]
	#define MOTOR_V_BAJA_LEVE 60	// Umbral de error leve para tension baja de baterias detectada por controlador de motores [V]
	#define MOTOR_TEMP_ALTA_GRAVE 80	// Umbral de error grave para temperatura alta de controlador de motores [ÂºC]
	#define MOTOR_TEMP_ALTA_LEVE 60		// Umbral de error leve para temperatura alta de controlador de motores [ÂºC]
	#define MOTOR_DIFF_V_MAX_LEVE 2		// Umbral de error leve de diferencia de tension baterias - controlador de motores [V]
	#define LIMITE_ERL_HEARTBEAT 5
	#define LIMITE_ERG_HEARTBEAT 10
	#define LIMITE_CONT_MOTOR_G 5	// Contador para la espera de recepcion de un mensaje de motor antes de dar error grave
	#define LIMITE_CONT_MOTOR_C 10	// Contador para la espera de recepcion de un mensaje de motor antes de dar error critico
	
	
	/* CODIGOS DE ERRORES LEVES */
	#define ERL_ECU_MASK		0x0001
	#define ERL_BMS_MASK		0x000E
	#define ERL_SENS_MASK		0x0FF0
	#define ERL_M1_MASK_1		0xF000
	/***********************************/
	#define ERL_ECU_COM_SUPERV 	(1<<0)
	#define ERL_BMS_PVOLT_H		(1<<1)
	#define ERL_BMS_PVOLT_L		(1<<2)
	#define ERL_BMS_PI_H		(1<<3)
	#define ERL_SENS_SUSP_DD	(1<<4)
	#define ERL_SENS_SUSP_DI	(1<<5)
	#define ERL_SENS_SUSP_TD	(1<<6)
	#define ERL_SENS_SUSP_TI	(1<<7)
	#define ERL_SENS_VOLANT		(1<<8)
	#define ERL_SENS_FRENO		(1<<9)
	#define ERL_SENS_ACEL		(1<<10)
	#define ERL_SENS_IMU		(1<<11)
	#define ERL_M1_VOLT_H		(1<<12)
	#define ERL_M1_VOLT_L		(1<<13)
	#define ERL_M1_TEMP_H		(1<<14)
	#define ERL_M1_DIFF_PWR		(1<<15)
	/***********************************/
	#define ERL_M1_MASK_2		0x1000
	#define ERL_M2_MASK			0x200F
	#define ERL_M3_MASK			0x40F0
	#define ERL_M4_MASK			0x8F00
	#define ERL_M2_VOLT_H		(1<<0)
	#define ERL_M2_VOLT_L		(1<<1)
	#define ERL_M2_TEMP_H		(1<<2)
	#define ERL_M2_DIFF_PWR		(1<<3)
	#define ERL_M3_VOLT_H		(1<<4)
	#define ERL_M3_VOLT_L		(1<<5)
	#define ERL_M3_TEMP_H		(1<<6)
	#define ERL_M3_DIFF_PWR		(1<<7)
	#define ERL_M4_VOLT_H		(1<<8)
	#define ERL_M4_VOLT_L		(1<<9)
	#define ERL_M4_TEMP_H		(1<<10)
	#define ERL_M4_DIFF_PWR		(1<<11)
	#define ERL_M1_VAUX			(1<<12)
	#define ERL_M2_VAUX			(1<<13)
	#define ERL_M3_VAUX			(1<<14)
	#define ERL_M4_VAUX			(1<<15)
	/* CODIGOS DE ERRORES GRAVES */
	#define ERG_ECU_MASK		0x003F
	#define ERG_BMS_MASK		0x03C0
	#define ERG_M1_MASK			0x3C00
	#define ERG_M2_MASK_1		0xC000
	#define ERG_ECU_COM_SUPERV	(1<<0)
	#define ERG_ECU_COM_BMS		(1<<1)
	#define ERG_ECU_COM_M1		(1<<2)
	#define ERG_ECU_COM_M2		(1<<3)
	#define ERG_ECU_COM_M3		(1<<4)
	#define ERG_ECU_COM_M4		(1<<5)
	#define ERG_BMS_TEMP_H		(1<<6)
	#define ERG_BMS_PVOLT_H		(1<<7)
	#define ERG_BMS_PVOLT_L		(1<<8)
	#define ERG_BMS_PI_H		(1<<9)
	#define ERG_M1_VOLT_H		(1<<10)
	#define ERG_M1_VOLT_L		(1<<11)
	#define ERG_M1_TEMP_H		(1<<12)
	#define ERG_M1_DIFF_PWR		(1<<13)
	#define ERG_M2_VOLT_H		(1<<14)
	#define ERG_M2_VOLT_L		(1<<15)
	/***********************************/
	#define ERG_M2_MASK_2		0x0003
	#define ERG_M3_MASK			0x003C
	#define ERG_M4_MASK			0x03C0
	#define ERG_M2_TEMP_H		(1<<0)
	#define ERG_M2_DIFF_PWR		(1<<1)
	#define ERG_M3_VOLT_H		(1<<2)
	#define ERG_M3_VOLT_L		(1<<3)
	#define ERG_M3_TEMP_H		(1<<4)
	#define ERG_M3_DIFF_PWR		(1<<5)
	#define ERG_M4_VOLT_H		(1<<6)
	#define ERG_M4_VOLT_L		(1<<7)
	#define ERG_M4_TEMP_H		(1<<8)
	#define ERG_M4_DIFF_PWR		(1<<9)
	#define ERG_IMU_COM_COLA 	(1<<10)
	/* CODIGOS DE ERRORES CRITICOS */
	#define ERC_ECU_T_MASK		0x0007
	#define ERC_ECU_M_MASK		0x0078
	#define ERC_BMS_MASK		0x1F80
	#define ERC_HILOS_MASK		0x2000
	#define ERC_ECU_COM_TAD_AO	(1<<0)
	#define ERC_ECU_COM_TAD		(1<<1)
	#define ERC_ECU_COM_TCAN1	(1<<2)
	#define ERC_ECU_COM_M1		(1<<3)
	#define ERC_ECU_COM_M2		(1<<4)
	#define ERC_ECU_COM_M3		(1<<5)
	#define ERC_ECU_COM_M4		(1<<6)
	#define ERC_BMS_CHAS_CON	(1<<7)
	#define ERC_BMS_CEL_COM		(1<<8)
	#define ERC_BMS_SYS_ERR		(1<<9)
	#define ERC_ECU_COM_TCAN2	(1<<10)
	#define ERC_ECU_COM_BMS		(1<<11)
	#define ERC_CAN2_COM_COLA	(1<<12)
	#define ERC_ECU_HILOS		(1<<13)

	
	/**********************************************************
	***			HILO TAD			***
	**********************************************************/
	
	/* CONSTANTES DE CONVERSION */
	#define FACTOR_CONV_I	0.02	// Factor de conversion desde el voltaje leido a la corriente equivalente (sensor corriente)
	/* DEFINICIONES ENTRADAS/SALIDAS ANALOGICAS */
	#define NUM_ANALOG_IN	9	// Numero de canales de lectura analogica de la TAD
	#define NUM_ANALOG_OUT 	8	// Numero de canales de escritura analogica de la TAD
	/* ENTRADAS ANALOGICAS */
	/* Indica la posicion en el vector de entradas analogicas locales de cada uno de los datos recogidos */
	#define ANLG_IN_ACL 		0	// Lectura de acelerador 
	#define ANLG_IN_FRN 		1	// Lectura de freno
	#define ANLG_IN_VLT 		2	// Lectura de volante
	#define ANLG_IN_STI 		3	// Lectura de suspension trasera izquierda
	#define ANLG_IN_STD 		4	// Lectura de suspension trasera derecha
	#define ANLG_IN_SDI 		5	// Lectura de suspension delantera izquierda
	#define ANLG_IN_SDD 		6	// Lectura de suspension delantera derecha
	#define ANLG_IN_ID			7	// Lectura del sensor de corriente del eje delantero
	#define ANLG_IN_IT			8	// Lectura del sensor de corriente del eje trasero
	/* SALIDAS ANALOGICAS */
	#define ANLG_OUT_ACL_M1		0	// Escribe senial de acelerador para M1
	#define ANLG_OUT_ACL_M2		1	// Escribe senial de acelerador para M2
	#define ANLG_OUT_ACL_M3		2	// Escribe senial de acelerador para M3
	#define ANLG_OUT_ACL_M4		3	// Escribe senial de acelerador para M4
	#define ANLG_OUT_FRN_M1		4	// Escribe senial de freno para M1
	#define ANLG_OUT_FRN_M2		5	// Escribe senial de freno para M2
	#define ANLG_OUT_FRN_M3		6	// Escribe senial de freno para M3
	#define ANLG_OUT_FRN_M4		7	// Escribe senial de freno para M4
	
	/* DEFINICIONES ENTRADAS/SALIDAS DIGITALES */
	/* ENTRADAS DIGITALES */
	#define DIG_IN_REV		0
	#define DIG_IN_ACL		1
	/* SALIDAS DIGITALES */
//	#define DIG_OUT_ACL_M1 	1
//	#define DIG_OUT_ACL_M2 	2
//	#define DIG_OUT_ACL_M3 	3
//	#define DIG_OUT_ACL_M4 	4
	#define DIG_OUT_FRN_M1 	5
	#define DIG_OUT_FRN_M2 	6
	#define DIG_OUT_FRN_M3 	7
	#define DIG_OUT_FRN_M4 	8
	
	
	/**********************************************************
	***			HILO TAD AO			***
	**********************************************************/
	
	#define DIR_BASE_ADQ_AO 0x300
	#define IRQ_ADQ 5
	#define NUM_CANALES_CDA 8
	#define RANGO_CDA 5
	
	
	/**********************************************************
	***			HILO IMU			***
	**********************************************************/
	
	//Ruta del puerto serie en el sistema de ficheros
	#define PUERTOCOM "/dev/ser1"
	#define BAUDRATE B115200
	#define TAMBUFFRX 4096
	//Tiempo de espera a respuesta de la IMU en ns
	#define ESPERA 10000000L

	//Tipos de error (para mas detalle usar errno)
	#define ERROK			0x00
	#define ERRPCOMOPEN		0x01
	#define ERRPCOMCLOSE 	0x02
	#define ERRPCOMPURGA 	0x03
	#define ERRPCOMCONFIG 	0x04
	#define ERRPCOMREAD 	0x05
	#define ERRPCOMWRITE 	0x06
	#define ERRPCOMVACIO 	0x07
	#define ERRIMURX 		0x08
	#define ERRIMUTX 		0x09
	#define ERRIMUESTADO 	0x10
	#define ERRIMUMENSAJE	0x11

	//Cabecera de los mensajes
	#define CABU 			0x75
	#define CABE			0x65

	//Descriptores de tipo de mensaje
	//y sus descriptores de campo
	//(Ordenados como en el manual)
	//////////////////////////////
	#define CABDESCBASE		0x01
	/*----------------------------*/
	#define	DESCPING 		0x01
	#define DESCIDLE 		0x02
	#define	DESCINFO 		0x03
	#define	DESCDESET 		0x04
	#define	DESCBIT 		0x05
	#define DESCRESUME 		0x06
	#define DESCRESET 		0x7E
	#define DESCINFORES		0x81
	#define DESCACK			0xF1

	///////////////////////////////
	#define	CABDESCCOM		0x0C
	/*---------------------------*/
	#define DESCPOLLAHRS 	0x01
	#define DESCPOLLGPS 	0x02
	#define	DESCAHRSDATRATE	0x06
	#define	DESCGPSDATARATE 0x07
	#define	DESCAHRSMESFORM 0x08
	#define	DESCGPSMESFORM 	0x09
	#define	DESCENSTREAM 	0x11
	#define DESCSAVESET 	0x30
	#define DESCGPSMODE 	0x34
	#define DESCAHRSSIGNSET 0x35
	#define DESCAHRSTIME 	0x36
	#define DESCACCELBIAS 	0x37
	#define DESCGYROBIAS 	0x38
	#define DESCCAPGYROBIAS 0x39
	#define	DESCHARDIRON 	0x3A
	#define DESCSOFTIRON 	0x3B
	#define DESCREALIGNUP 	0x3C
	#define DESCREALIGNN 	0x3D
	#define DESCBAUDRATE 	0x40
	#define DESCSTREAMFORM 	0x60
	#define	DESCPOWERST 	0x61
	#define DESCSAVEADVSET 	0x62
	#define DESCSTATUS 		0x64

	/////////////////////////////
	#define CABDESCSYS		0x7F
	/*---------------------------*/
	#define DESCCOMMODE 	0x10

	/////////////////////////////
	#define CABDESCAHRS 	0x80
	/*---------------------------*/
	#define DESCRAWACCEL 	0x01
	#define DESCRAWGYRO 	0x02
	#define DESCRAMAG 		0x03
	#define DESCSCALACCEL 	0x04
	#define DESCSCALGYRO 	0x05
	#define DESCSCALMAG 	0x06
	#define DESCDELTATHETA 	0x07
	#define DESCDELTAVELO 	0x08
	#define DESCORIENT 		0x09
	#define DESCQUATER 		0x0A
	#define DESCORIENTUP 	0x0B
	#define DESCEULER 		0x0C
	#define DESCINTTIME 	0x0E
	#define DESCBEACONTIME 	0x0F
	#define DESCSTABMAG 	0x10
	#define DESCSTABACCEL 	0x11
	#define DESCGPSTIMEST 	0x12
	#define DESCWRAPDATA 	0x82

	/////////////////////////////
	#define CABDESCGPS 		0x81
	/*---------------------------*/
	#define DESCLLHPOS 		0x03
	#define DESCECEFPOS 	0x04
	#define DESCNEDVELO 	0x05
	#define DESCECEFVELO 	0x06
	#define DESCDOPDATA 	0x07
	#define DESCUTCTIME 	0x08
	#define DESCGPSTIME 	0x09
	#define DESCCLKINFO 	0x0A
	#define DESCGPSFIXINFO 	0x0B
	#define DESCSPACEINFO 	0x0C
	#define DESCHWSTAT 		0x0D
	#define DESCWRAPNMEA 	0x01
	#define DESCWRAPUBX 	0x02	
	
	/**********************************************************
	***			HILO CAN			***
	**********************************************************/
	
	/* CONSTANTES CONTROLADORES DE MOTORES - COM CAN */
	#define INFO_MODULE_NAME			0x40
	#define INFO_SOFTWARE_VER			0x53
	#define CAL_TPS_DEAD_ZONE_LOW		0x04
	#define CAL_TPS_DEAD_ZONE_HIGH		0x05
	#define CAL_BRAKE_DEAD_ZONE_LOW		0x26
	#define CAL_BRAKE_DEAD_ZONE_HIGH	0x27
	#define COM_READING					0x00
	#define CCP_INVALID_COMMAND			0xE3
	#define CCP_FLASH_READ				0xF2
	#define CCP_A2D_BATCH_READ1			0x1B
	#define CCP_A2D_BATCH_READ2			0x1A
	#define CCP_MONITOR1				0x33
	#define CCP_MONITOR2				0x37
	#define COM_SW_ACC					0x42
	#define COM_SW_BRK					0x43
	#define COM_SW_REV					0x44
	
	/* IDENTIFICADORES CAN (ENTRADA A LA ECU) */
	#define ID_CTRL_MOTOR_1_C_ECU		0x70
	#define ID_CTRL_MOTOR_3_C_ECU		0x71
	#define ID_CTRL_MOTOR_2_C_ECU		0x72
	#define ID_CTRL_MOTOR_4_C_ECU		0x73
	/* IDENTIFICADORES CAN (SALIDA DE LA ECU) */
	#define ID_CTRL_MOTOR_1_ECU_C 		0x6A
	#define ID_CTRL_MOTOR_3_ECU_C 		0x6B
	#define ID_CTRL_MOTOR_2_ECU_C 		0x6C
	#define ID_CTRL_MOTOR_4_ECU_C 		0x6D
	/* IDENTIFICADORES CAN - COMUNICACION ECU <-> SUPERVISOR */
	#define ID_WATCHDOG					0x10
	#define ID_SUPERV_OFF				0x11
	#define ID_SUPERV_HEARTBEAT			0x12
	#define ID_SUPERV_VER 				0x13
	#define ID_ECU_SUPERV_ERC			0x07
	#define ID_ECU_SUPERV_ERG			0x08
	#define ID_ECU_SUPERV_ERL			0x09
	#define ID_BMS_ESTADO 				0x50
	#define ID_BMS_ESTADO_V 			0x51
	#define ID_BMS_ESTADO_T				0x52
	#define ID_BMS_TEMP_CELDAS_1		0x53
	#define ID_BMS_TEMP_CELDAS_2		0x54
	#define ID_BMS_TEMP_CELDAS_3		0x55
	#define ID_BMS_V_CELDAS_1			0x56
	#define ID_BMS_V_CELDAS_2			0x57
	#define ID_BMS_V_CELDAS_3			0x58
	#define ID_BMS_V_CELDAS_4			0x59
	#define ID_BMS_V_CELDAS_5			0x5A
	#define ID_BMS_V_CELDAS_6			0x5B
	#define ID_VEHICULO_ACTUACION		0x80
	#define ID_VEHICULO_SUSPENSION		0x81
	#define ID_VEHICULO_CORRIENTE		0xC1
	#define ID_IMU_DATOS_1				0xB1
	#define ID_IMU_DATOS_2				0xB2
	#define ID_IMU_DATOS_3				0xB3
	#define ID_IMU_DATOS_4				0xB4
	#define ID_IMU_DATOS_5				0xB5
	#define ID_MOTOR1_ZONA_MUERTA 		0x84
	#define ID_MOTOR1_CONV_AD1			0x85
	#define ID_MOTOR1_CONV_AD2			0x86
	#define ID_MOTOR1_CONV_AD3			0x87
	#define ID_MOTOR2_ZONA_MUERTA 		0x8A
	#define ID_MOTOR2_CONV_AD1			0x8B
	#define ID_MOTOR2_CONV_AD2			0x8C
	#define ID_MOTOR2_CONV_AD3			0x8D
	#define ID_MOTOR3_ZONA_MUERTA		0x90
	#define ID_MOTOR3_CONV_AD1			0x91
	#define ID_MOTOR3_CONV_AD2			0x92
	#define ID_MOTOR3_CONV_AD3			0x93
	#define ID_MOTOR4_ZONA_MUERTA		0x96
	#define ID_MOTOR4_CONV_AD1			0x97
	#define ID_MOTOR4_CONV_AD2			0x98
	#define ID_MOTOR4_CONV_AD3			0X99
	#define ID_MOTOR1_MODELO_CTRL 		0xD1
	#define ID_MOTOR2_MODELO_CTRL 		0xD2
	#define ID_MOTOR3_MODELO_CTRL 		0xD3
	#define ID_MOTOR4_MODELO_CTRL 		0xD4
		
	/* TIPOS DE MENSAJES CAN ENVIADOS DESDE LA ECU A LOS CONTROLADORES */
	#define MSG_TIPO_01	1	// Tipo 1: Peticion de modelo de controlador (lectura 1 sola vez, impresion por pantalla)
	#define MSG_TIPO_02	2	// Tipo 2: Peticion de version de controlador (lectura 1 sola vez, impresion por pantalla)
	#define MSG_TIPO_03	3	// Tipo 3: Peticion de Zona Muerta Inferior del acelerador (lectura 1 sola vez, a estructura global)
	#define MSG_TIPO_04	4	// Tipo 4: Peticion de Zona Muerta Superior del acelerador (lectura 1 sola vez, a estructura global)
	#define MSG_TIPO_05	5	// Tipo 5: Peticion de Zona Muerta Inferior del freno (lectura 1 sola vez, a estructura global)
	#define MSG_TIPO_06	6	// Tipo 6: Peticion de Zona Muerta Superior del freno (lectura 1 sola vez, a estructura global)
	#define MSG_TIPO_07	7	// Tipo 7: Peticion de Conversion A/D 1 (lectura continua, a estructura global)
	#define MSG_TIPO_08	8	// Tipo 8: Peticion de Conversion A/D 2 (lectura continua, a estructura global)
	#define MSG_TIPO_09	9	// Tipo 9: Peticion de Datos 1 (lectura continua, a estructura global)
	#define MSG_TIPO_10	10	// Tipo 10: Peticion de Datos 2 (lectura continua, a estructura global)
	#define MSG_TIPO_11	11
	#define MSG_TIPO_12	12
	#define MSG_TIPO_13	13
	
	/* MASCARAS PARA BYTES DE MENSAJES CAN */
	#define MASK_BYTE_ALTO		0xFF00	// Para unir dos bytes y recuperar un dato completo en un mensaje.
	#define MASK_BYTE_BAJO		0x00FF	// Para unir dos bytes y recuperar un dato completo en un mensaje.
	
	/* TAMANHO BUFFER RECEPCION */
	#define TAM_BUFF_RX	100	// Numero de mensajes que contiene el buffer de recepcion del supervisor.

	//DEFINES POR: Gonzalo Hernández Rodríguez
	#define NP 8
	#define NU 5

#endif	// 	_CONSTANTES_FOX_H_
