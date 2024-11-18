/**********************************************************************************
***										***	
***	Fichero: estructuras_fox.h						***
***	Fecha: 22/02/2013							***
***	Autor: Elena Gonzalez							***
***	Descripción: Cabecera de definicion de estructuras globales		***
***										***
**********************************************************************************/

#ifndef _ESTRUCTURAS_FOX_H_
#define _ESTRUCTURAS_FOX_H_

	/* SUPERVISOR */
	typedef struct {
		BOOL heartbeat;		// Toma valores TRUE-FALSE en funcion de si se ha recibido el heartbeat o no
		BOOL enviado_off;	// Toma valores TRUE-FALSE en funcion de si se ha enviado la peticion de apagado al supervisor
		BOOL peticion_off;	// Toma valores TRUE-FALSE en funcion de si se ha de apagar el supervisor
		BOOL on;			// Toma valores TRUE-FALSE en funcion de si se ha encendido ya el supervisor (recibe heartbeat por primera vez)
		BOOL motor_on[NUM_MOTORES];	//Permite elegir desde el supervisor sobre que motores se actuara
	} est_superv_t;
	
	/* VEHICULO */
	typedef struct {
		/* Variables de usuario */
		float acelerador;		// Valor del acelerador (de 0 a 5V)	
		float freno;			// Valor del freno (de 0 a 5V)
		float volante;			// Valor de giro del volante (de 0 a 5V)
		/* Variables de sensores de suspension */
		float susp_ti;			// Valor de la suspension de M1
		float susp_td;			// Valor de la suspension de M3
		float susp_di;			// Valor de la suspension de M2
		float susp_dd;			// Valor de la suspension de M4
		float i_eje_d;			// Valor de corriente del eje delantero (A)
		float i_eje_t;			// Valor de corriente del eje trasero	(A)
		BOOL marcha_atras;		// Indica si esta activada la marcha atras
		BOOL act_acel; 			// Indica si esta activado el switch del pedal acelerador	
	} est_veh_t;
	
	/* IMU */
	typedef struct {
		float pfAccelScal[3]; //[x;y;z] (G=0.980665 m/s^2). Centrado localmente.
		float pfGyroScal[3];  //[x;y;z] (rad/s). Centrado localmente.
		float pfEuler[3];	//[roll;pitch;yaw] (rad). Centrado en la Tierra.
		uint32_t iTimeStamp;  //en ticks de 16 us (/62500 para s).
		//pdLLHPos[0]: Latitud (º dec.).
		//pdLLHPos[1]: Longitud (º dec.).
		//pdLLHPos[2]: Altura sobre la elipsoide (WGS 84) (m).
		//pdLLHPos[3]: Altura sobre el nivel del mar (MSL) (m).
		double pdLLHPos[4];
		//pfNEDVeloc[0]: Norte (m/s)
		//pfNEDVeloc[1]: Este (m/s)
		//pfNEDVeloc[2]: Abajo (m/s)
		//pfNEDVeloc[3]: Modulo (m/s)
		//pfNEDVeloc[4]: Modulo proyeccion en tierra (m/s)
		//pfNEDVeloc[5]: Angulo polar (º dec.)
		float pfNEDVeloc[6];
	} datosImu_t;
	
	typedef struct {
		BYTE bDescTipo;
		BYTE bNumCampos;
		BYTE **ppbDatos;
	} mensajeImu_t;
	
	/* BATERIAS */
	typedef struct {
		// Errores de comunicacion
		BOOL error_com_tcan;		// Vale 0 si no hay error de comunicacion con la tarjeta CAN, 1 si hay error.
		BOOL error_com_bms;		// Vale 0 si no hay error de comunicacion con la BMS via CAN, 1 si hay error.
		/* Estado general de las baterias */
		uint8_t num_cel_scan;		// Numero de celdas de la bateria detectadas (0 - 24)
		uint8_t temp_media;		// Temperatura media de celda (0 - 80)
		uint16_t v_medio;			// Voltaje medio de celda en mV (0 - 4500)
		uint32_t v_pack;			// Voltaje del pack de baterias en mv (0 - 91200)
		int16_t i_pack;				// Corriente del pack de baterias (-800 - 800)
		uint8_t soc;			// Estado de carga de baterias (0 - 100)
		uint16_t timestamp;		// Timestamp de la BMS (0 - 3090...)
		uint8_t temp_max;			// Temperatura maxima de celda (0 - 80)
		uint8_t cel_temp_max;		// Celda con temperatura maxima (0 - 24)
		uint16_t v_max;			// Voltaje maximo de celda en mv (0 - 4500)
		uint8_t cel_v_max;		// Celda con voltaje maximo (0 - 24)
		uint16_t v_min;			// Voltaje minimo de celda (0 - 4500)
		uint8_t cel_v_min;		// Celda con voltaje minimo (0 - 24)
		uint8_t nivel_alarma;		// Indica si la alarma es nula, leve, grave o critica (0 - 3)
		uint8_t alarma;			// Indica la alarma vigente (0 - 8)
		/* Estado detallado de las baterias */
		uint16_t mv_cel[NUM_CEL_BAT];	// Voltaje en mV en cada celda (0 - 4500)
		uint8_t temp_cel[NUM_CEL_BAT];	// Temperatura en ºC en cada celda (0 - 80)
	} est_bat_t;
	
	/* CONTROLADOR-MOTOR */
	typedef struct {
		char modelo[9];
		short version;
		
		BYTE zm_inf_acel;		// Zona muerta inferior configurada para el acelerador (en %)
		BYTE zm_sup_acel;		// Zona muerta superior configurada para el acelerador (en %)
		BYTE zm_inf_fren;		// Zona muerta inferior configurada para el freno (en %)
		BYTE zm_sup_fren;		// Zona muerta superior configurada para el freno (en %)
		
		float freno;			// Senial de freno leida por el controlador del motor (0 a 5 V)
		float acel;				// Senial de acelerador leida por el controlador del motor (0 a 5 V)
		float v_pot;			// Senial de tension de potencia leida por el controlador (hasta aprox 72 V)
		float v_aux;			// Senial de tension de 5v auxiliar leida por el controlador (hasta 5.25 V)
		float v_bat;			// Senial de tension de baterias leida por el controlador (hasta aprox 72 V)
		
		BYTE i_a;				// Corriente fase A (unidades desconocidas)
		BYTE i_b;				// Corriente fase B
		BYTE i_c;				// Corriente fase C
		float v_a;				// Voltaje fase A (hasta 72 V aprox.)
		float v_b;				// Voltaje fase B
		float v_c;				// Voltaje fase C
		
		BYTE pwm;				//Duty cycle (de 0 a 100)
		BOOL en_motor_rot;		//Enabling motor rotation
		BYTE temp_motor;		//Temp. del motor (celsius). Si es 255 no hay sensor.
		BYTE temp_int_ctrl;
		BYTE temp_sup_ctrl;
		BYTE temp_inf_ctrl;
		
		uint16_t rpm;			//Velocidad del motor en RPM
		BYTE i_porcent;
		
		BOOL acel_switch;
		BOOL freno_switch;
		BOOL reverse_switch;
	} est_motor_t;
	
	/* POTENCIAS */
	typedef struct {
		float acelerador[NUM_MOTORES];	// Valor 0-5v que se dara como entrada de acelerador de cada motor
		float freno[NUM_MOTORES];		// Valor 0-5v que se dara como entrada de freno de cada motor
//		BOOL act_acel[NUM_MOTORES];		//Activacion digital del interruptor de los aceleradores
		BOOL act_freno[NUM_MOTORES]; 	//Activacion digital del interruptor de los frenos
		int16_t pot_deman;				// Potencia demandada a partir del acelerador que acciona el usuario
		int16_t pot_bat_act;			// Potencia que cedera/absorbera la bateria para satisfacer la demanda en el instante actual
		int16_t pot_bat_sig;			// Potencia que cedera/absorbera la bateria, calculada para el instante siguiente
		int16_t pot_motores_act;		// Potencia asignada en total para los 4 motores del instante actual
		int16_t pot_motores_sig;		// Potencia asignara en total para los 4 motores, calculada para el instante siguiente
		int16_t pot_m1;			// Potencia asignada al motor 1		
		int16_t pot_m2;			// Potencia asignada al motor 2
		int16_t pot_m3;			// Potencia asignada al motor 3
		int16_t pot_m4;			// Potencia asignada al motor 4
		int16_t pot_fc;			// Potencia que cedera/absorbera la pila de combustible para satisfacer la demanda
		int16_t pot_sc;			// Potencia que cedera/absorbera el supercondensador para satisfacer la demanda
	} est_pot_t;
	
	
	/* PILA DE COMBUSTIBLE */
	
	/* SUPERCONDENSADORES */
	
	
	/* ESTRUCTURA ERRORES */
	typedef struct {
		uint8_t error_leve;			/* Flag errores leves */
		uint8_t error_grave;			/* Flag errores graves */
		uint8_t error_critico;			/* Flag errores criticos */
		uint16_t er_leve_1;	
		uint16_t er_leve_2;
		uint16_t er_grave_1;
		uint16_t er_grave_2;
		uint16_t er_critico_1;
		uint16_t er_emergencia;
		uint16_t er_watchdog_bms;
		uint16_t er_watchdog_m1;
		uint16_t er_watchdog_m2;
		uint16_t er_watchdog_m3;
		uint16_t er_watchdog_m4;
	} est_error_t;
	
	/* Estructura de datos para enviar al proceso que guarda los datos de la imu y los errores */
	typedef struct {
		datosImu_t datosImu;
		uint16_t errores_criticos;
		uint16_t errores_graves;
		uint16_t errores_leves;
	} dato_cola_imu_t;
	
	
	/* OPCIONES DE LA TARJETA DE ADQUISICION DE DATOS */
	typedef struct {
		/* Configuracion HW */
		uint64_t direccion; 		//Direccion fisica de la tarjeta (0x300 por defecto)
		uint8_t irq; 			// Numero de IRQ.
		/* Configuracion entrada analogica */
		char rango_ad[16]; 		//Rango de las entradas analogicas. 0-11 (ver pcm-3718hg_ho.h)
		float rango_ad_f[16];		//Rango en su valor real (indica polaridad en su signo).
		uint8_t canal_inicio_ad; 	// Primer canal lectura de entradas analogicas
		uint8_t canal_fin_ad; 		// Último canal lectura de entradas analogicas
		/* Configuracion E/S digital */
		uint8_t dio_0; 			// 0 si el bit 0 del byte 0 de las e/s digitales es entrada, y 1 si es salida
		uint8_t dio_1; 			// idem byte 1
		/* Configuracion pacer */
		int16_t cnt1; 		// Programacion contador 1
		int16_t cnt2; 		// Programacion contador 2
		/* Trigger */
		uint8_t trigger; 		// 1-->SW trigger, 3-->Pacer Trigger 
		struct timespec periodo; 	//Periodo temporizadores adquisicion datos.
	} config_pcm3718;

#endif	// _ESTRUCTURAS_FOX_H_
