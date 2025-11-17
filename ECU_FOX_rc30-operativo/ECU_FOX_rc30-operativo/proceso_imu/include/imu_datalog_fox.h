/**************************************************************************
***									***	
***	Fichero: imu_datalog_fox.h						***
***	Fecha: 26/11/2013						***
***	Autor: Elena Gonzalez						***
***	Descripción: Cabecera de proceso de datalog de datos recibidos 	***
***	por cola acerca de la imu.		***
***									***
**************************************************************************/

#ifndef _IMU_DATALOG_FOX_H_
#define _IMU_DATALOG_FOX_H_

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

	#define TIEMPO_EXP_COLA_S 		30	 // Si no se recibe un msg en la cola en 30 seg da error de com. y termina el proceso
	#define NOMBRE_COLA 			"/COLA_IMU"
	#define NOMBRE_RAIZ 			"/datos/datos_imu/datalog_imu"
	#define TAMANHO_COLA_IMU 		500
	#define NUM_MAX_DATOS_FICHERO	20000
	#define TIEMPO_BUCLE_NS 		50000000L	// Tiempo base para espera del bucle principal de la ECU (50 ms).
	#define ER_CONT_COLA 			50

	/* ESTRUCTURAS */
	
	typedef struct {
		float pfAccelScal[3]; 	//[x;y;z] (G=0.980665 m/s^2). Centrado localmente.
		float pfGyroScal[3];  	//[x;y;z] (rad/s). Centrado localmente.
		float pfEuler[3];		//[roll;pitch;yaw] (rad). Centrado en la Tierra.
		uint32_t iTimeStamp;  	//en ticks de 16 us (/62500 para s).
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
	}datosImu_t;
	
	typedef struct {
		datosImu_t datosImu;
		uint16_t errores_criticos;
		uint32_t errores_graves;
		uint32_t errores_leves;
	}dato_cola_t;

	/* DECLARACIONES DE FUNCIONES */
	int crea_nuevo_fichero (void);
	int escribe_cabeceras(void);
	int escribe_dato (dato_cola_t *);

#endif // _IMU_DATALOG_FOX_H_
