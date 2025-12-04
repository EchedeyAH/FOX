/*************************************************************************
***									***
***	Fichero: ctrl_trac_estab_fox.c					***
***	Fecha: 10/10/2013						***
***	Autor: Elena Gonzalez - Editado por: Jose Carlos Alonso Caeizal	***
***	Descripcien: Hilo de control de traccion y estabilidad.		    ***
***									***
**************************************************************************/

/* CABECERAS */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include <hw/inout.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/neutrino.h>
#include <stdint.h>
#include <sys/mman.h>
#include <mqueue.h>
#include <pthread.h>
/* Include local */
#include "./include/constantes_fox.h"
#include "./include/estructuras_fox.h"
#include "./include/declaraciones_fox.h"


// Definiciones a anhadir en el .h:
#define HILO_DIF_LISTO 1 
#define ANG_MIN_VOL -180.0
#define ANG_MAX_VOL 180.0
//#define MINIMO_U_PI 0.2
//#define MAXIMO_U_PI 0.8
#define NUMERO_MUESTRAS 6000
//#define TAMANHO_MUESTRA 18
#define NMUESTRAJC 80
#define GRAVEDAD 9.81

// Cabeceras a anhadir en el .h:
//void calculo_angulo_volante(float volante_local, float *volante_ang);
//void calculo_curvatura_ackermann(float volante_ang, float *curvatura_acker);
//void ctrl_pi(float error_yr, float *u_pi);


/**************************************************
***		 VARIABLES GLOBALES		***
**************************************************/
/* Variables globales */
extern uint16_t hilos_listos;

/* Estructuras globales */
extern est_veh_t vehiculo;
extern est_bat_t bateria;
extern est_motor_t motor1;
extern est_motor_t motor2;
extern est_motor_t motor3;
extern est_motor_t motor4;
extern est_error_t errores;
extern est_pot_t potencias;
extern est_superv_t supervisor;

extern datosImu_t tDatosImu; 

/* MUTEX */
extern pthread_mutex_t mut_vehiculo;
extern pthread_mutex_t mut_bateria;
extern pthread_mutex_t mut_motor1;
extern pthread_mutex_t mut_motor2;
extern pthread_mutex_t mut_motor3;
extern pthread_mutex_t mut_motor4;
extern pthread_mutex_t mut_errores;
extern pthread_mutex_t mut_supervisor;
extern pthread_mutex_t mut_potencias;
extern pthread_mutex_t mut_hilos_listos;
extern pthread_mutex_t mut_inicio;
extern pthread_cond_t inicio;

extern pthread_mutex_t mDatosImu;

/* flag para la variable de condicion*/
extern BOOL cond_inicio;

/*Prototipo de funciones propias*/
void  CambioSR_Veloc(float *Vgps, float *Ang,float *velocidad_COG);



/*LA POSICION DE LAS RUEDAS ES:
FL=2  ------  FR=4
	 |_|_	|
	 |		|
	 |		|
RL=1  ------  RR=3*/

void *ctrl_traccion_main (void *pi) 
{
	/**************************************************
	***		VARIABLES			***
	**************************************************/
	/*VARIABLES ASOCIADAS A SENSORES DEL COCHE*/
	float freno_local;			// Valor del freno accionado por el conductor
	float acelera_local;		// Valor del freno accionado por el conductor
	BOOL act_acel_local;
	BOOL motor_on_local[NUM_MOTORES];
	float trac_freno_local[NUM_MOTORES];	// Valor de freno para mandarle a cada motor
	float trac_acelera_local[NUM_MOTORES];	// Valor de acelerador para mandarle a cada motor
	BOOL act_freno_local[NUM_MOTORES];
		
	float volante_local;         // V en potenci�metro del volante (0-5V)
	float Steering_angle, Delta_WFL,  Delta_WFR; //Direccion del vehiculo, direccion ruedas delanteras

	float aceleracion[3];        // Valor del aceler�metro (g):  Aceleraci�n del vehiculo del COG en ejes locales
	float acelerometrox = {0,0};
	float acelerometroy = {0,0};
	float velocidad_Ang[3];      // Valor del gir�scopo: Velocidad angular (rad/s) del COG en ejes locales
	float ang_Euler[3];          // Valor del inclin�metro: [Roll Pitch Yaw] (rad)
	float velocidad_GPS[6];      // Velocidad lineal del COG medida por el gps(m/s)

	/*VARIABLES ASOCIADAS AL MODELO DINAMICO*/
	float velocidad_COG[4];      // Velocidad lineal del COG en ejes del coche(m/s) 
	float pV_vehiculo;          // Puntero para las velocidades en el sistema de ejes local
	float Beta;                  //�ngulo de deriva de la velocidad del coche (el que forma la velocidad con el sistema de ejes local (SRNI))
	
	/*Velocidad lineal de las ruedas[x,y]*/
	float veloc_WFL[2], veloc_WFR[2], veloc_WRL[2], veloc_WRR[2]; //EJES DE LA RUEDA
	float veloc_WFL_src[2], veloc_WFR_src[2];					  //EJES DEL COCHE (SistemaReferenciaCoche) (En las ruedas traseras ambos sistemas coinciden)
	/*Modulo de Velocidad*/
	float veloc_mod_WFL, veloc_mod_WFR, veloc_mod_WRL, veloc_mod_WRR;
	/*Angulo de deriva de la velocidad de las ruedas*/
	float ang_deriva_RL,ang_deriva_FL,ang_deriva_RR,ang_deriva_FR;
	/*Velocidad longitudinal acumulada*/
	float veloc_WFL_x[3]  ={0.0, 0.0, 0.0};
	float veloc_WFR_x[3]  ={0.0, 0.0, 0.0};
	float veloc_WRL_x[3]  ={0.0, 0.0, 0.0};
	float veloc_WRR_x[3]  ={0.0, 0.0, 0.0};

	/*Velocidad Angular acumulada*/
	float veloc_ang_WFL[3]  ={0.0, 0.0, 0.0};
	float veloc_ang_WFR[3]  ={0.0, 0.0, 0.0};
	float veloc_ang_WRL[3]  ={0.0, 0.0, 0.0};
	float veloc_ang_WRR[3]  ={0.0, 0.0, 0.0};
	float veloc_ang[4]      ={0.0, 0.0, 0.0 0.0}; 

	/*Aceleracion estimada longitudinal*/	
	float acel_est_FR[2]={0.0, 0.0};
	float acel_est_FL[2]={0.0, 0.0};
	float acel_est_RR[2]={0.0, 0.0};
	float acel_est_RL[2]={0.0, 0.0};
	/*Velocidad Angular acumulada*/
	float acel_ang_est_FR[2]={0.0, 0.0};
	float acel_ang_est_FL[2]={0.0, 0.0};
	float acel_ang_est_RR[2]={0.0, 0.0};
	float acel_ang_est_RL[2]={0.0, 0.0};

	float posicion_GPS[4];
	float susp_ti[3]={0.0, 0.0 0.0},susp_di[3]={0.0, 0.0 0.0},susp_td[3]={0.0, 0.0 0.0},susp_dd[3]={0.0, 0.0 0.0}; //Valor de los amortiguadores (�EN VOLTIOS?)
	//float susp_ti_der,susp_di_der,susp_td_der,susp_dd_der;														   //Derivada de la se�al de amortiguadores
	//float dist_calib_sti, dist_calib_sdi, dist_calib_std, dist_calib_sdd;										   //Valor de calibracion del amortiguador
	//float SUM_susp; //Suma de las medidas (Para estimacion de la normal en cada rueda)
	//float alloct_norm[4]; //Reparto de carga vertical entre las ruedas
	/*dist_calib_sti= 3.93;
	dist_calib_sdi= 3.7;
	dist_calib_std= 3.3;
	dist_calib_sdd= 3.7;*/

	float desliz_L_RL, desliz_L_FL, desliz_L_RR, desliz_L_FR, desliz_S_RL, desliz_S_FL, desliz_S_RR, desliz_S_FR;
	float desliz_res_FL, desliz_res_RL, desliz_res_FR, desliz_res_RR, nu_WRL,nu_WFL,nu_WRR,nu_WFR;
	float pseudodes[4]={0,0,0,0};

	//float A2 = -DIST_R_LEFT/(DIAMETRO_WR/2);
	//float A4 =  DIST_R_RIGHT/(DIAMETRO_WR/2);
	
	//VARIABLES DE MTTE
	/*Fuerza de Friccion en el neumatico,
	  Par�metro auxiliar alpha,
	  Par m�ximo aplicable a la rueda*/
	float Ffricc_FR, alpha_FR[2]={0.0, 0.0}, T_max_FR;
	float Ffricc_FL, alpha_FL[2]={0.0, 0.0}, T_max_FL;
	float Ffricc_RR, alpha_RR[2]={0.0, 0.0}, T_max_RR;
	float Ffricc_RL, alpha_RL[2]={0.0, 0.0}, T_max_RL;
	float T_max[4];  //Nivel de tension correspondiente al Par Maximo Aplicable
	
	//VARIABLES DEL CONTROL EN YAW
	float YR_real[3]    = {0.0, 0.0 0.0};           //Yaw rate del coche (rad/s)
	float YR_deseado[3] = {0.0, 0.0 0.0};           //Yaw rate deseado (rad/s)
	//float Derivada_YRdeseado, Derivada_YRreal;		//Derivada del YR deseado y real
	//float error_yr_derivada[3]={0.0, 0.0 0.0}; ;    //Error en derivadas de yawrate
	//float Momento_corrector;						//Momento que hay que aplicar al vehiculo para obtener el YR deseado
	float Momento_provocado;						//Momento provocado por los pares del pedal
	float dM[4];									//Diferencial del par a aplicar
	//float K_yawcontrol = 0.75;						//Controlador Proporcional	
	float yawrate_lim_ay, yawrate_lim_B ;
	float B_low  = (PI/180)*(-5);			//Limite de Beta inferior -5�
	float B_high = (PI/180)*(+5);		//Limite de Beta superior +5�
		
	float Ftotal,F_WFLsrc_Mz[2],F_WFL_Mz[2],F_WRL_Mz[2],F_WFRscr_Mz[2],F_WFR_Mz[2],F_WRR_Mz[2];
	float Par_calculado[4]={0,0,0,0};
	float Parmax[4]   ={0,0,0,0}; //par maximo aplicable
	float Par_avail[4]={0,0,0,0};

	float Par_inicial_met1[4]={0,0,0,0};
	float Par_inicial_met2[4]={0,0,0,0};
	float RRL RFL RRR RFR RadioGiro R_vlow R_vhigh; //Radio de Giro
	float Fz_RL, Fz_FL, Fz_RR, Fz_FR ;//
	float c1=1.1973, c2=25.168, c3=0.5373;//Asfalto
	//c1=0.8, c2=35,    c3=0.3;  //Asfalto mojado
	//c1=1.28,c2=23.99, c3=0.52; //Empedrado
	//c1=0.2, c2=94,    c3=0.07; //Nieve
	

	
	
	//float volante_local, acelerador_local, yaw_rate_local, velocidad_local; //Variables locales a partir de las globales
	//float volante_ang, curvatura_acker, yaw_rate_des, error_yr, u_pi, latitud, longitud;
	//float senhal_motores[NUM_MOTORES]; //Senhal a los motores [tras. izq., del. izq., del. izq., del. der.]
	//float dist[NUM_MOTORES]; // Distribucion [delantera interior, trasera interior, delantera exterior, trasera exterior]
	
	/*ALMACENAJE DE DATOS*/
	int i, j, k=0;
	FILE *fp;
	char nombre_archivo[100];
	struct timespec tiempo_actual;

	/*TEMPORIZACION*/
	sigset_t cjto_temp;
	timer_t tempo;
	struct sigaction accion;
	struct sigevent eventos_temp;
	struct itimerspec prg_temp;
	struct timespec periodo_temp;


//	struct timespec espera = {0, TIEMPO_BUCLE_NS};
	BOOL fin_local = 0;
	BOOL flag_archivo=1;
	
	float *bufferjc;
	//buffer=(float*)malloc(TAMANHO_MUESTRA*NUMERO_MUESTRAS*sizeof(float));
	bufferjc=(float*)malloc(NMUESTRAJC*NUMERO_MUESTRAS*sizeof(float));
	
//	if (buffer==NULL)
//		printf("El buffer es NULL\n");

	/* Indica que el hilo del Control esta listo */
	pthread_mutex_lock(&mut_hilos_listos);
	hilos_listos |= HILO_TRAC_LISTO;
	pthread_mutex_unlock(&mut_hilos_listos);
	printf("Hilo TRAC listo\n");
	clock_gettime(CLOCK_REALTIME,&tiempo_actual);
	pthread_mutex_lock(&mut_inicio);
	while(!cond_inicio){
		pthread_cond_wait(&inicio, &mut_inicio);
	}
	pthread_mutex_unlock(&mut_inicio);




	/**************************************************
	***	CONFIGURACION Y LANZAMIENTO DEL TEMPORIZADOR ***
	**************************************************/

	// Usamos SIG_CTRL_TRAC_SINC como se�al en el temporizador:
	sigemptyset(&cjto_temp);
	sigaddset(&cjto_temp, SIG_CTRL_TRAC_SINC);
	// Programacion del periodo
	periodo_temp.tv_sec=0;
	periodo_temp.tv_nsec=TIEMPO_BUCLE_NS;
	prg_temp.it_value=periodo_temp;
	prg_temp.it_interval=periodo_temp;
	activar_temporizador_esp_sinc(&tempo, SIG_CTRL_TRAC_SINC,  &accion, &eventos_temp, prg_temp);


	
	while(!fin_local)
	{

		/* Volcado de las variables globales a locales */
		pthread_mutex_lock(&mut_vehiculo);
		acelera_local  = vehiculo.acelerador;
		freno_local    = vehiculo.freno;
		act_acel_local = vehiculo.act_acel;
		volante_local  = vehiculo.volante;
		susp_ti[0]	   = vehiculo.susp_ti;
		susp_di[0]	   = vehiculo.susp_di;
		susp_td[0]	   = vehiculo.susp_td;
		susp_dd[0]	   = vehiculo.susp_dd;
		pthread_mutex_unlock(&mut_vehiculo);

		pthread_mutex_lock(&mDatosImu);
		memcpy(aceleracion,   tDatosImu.pfAccelScal,  sizeof(aceleracion));
		memcpy(velocidad_Ang, tDatosImu.pfGyroScal,   sizeof(velocidad_Ang));	/*Variaci�n �ngulos_euler [x,y,z](rad/s) medido por el gir�scopo*/
		memcpy(ang_Euler,     tDatosImu.pfEuler,      sizeof(ang_Euler));		/* Angulo de Euler(rad) */
		memcpy(velocidad_GPS, tDatosImu.pfNEDVeloc,   sizeof(velocidad_GPS)
			//pfNEDVeloc[0]: Norte (m/s)
			//pfNEDVeloc[1]: Este (m/s)
			//pfNEDVeloc[2]: Abajo (m/s)
			//pfNEDVeloc[3]: Modulo (m/s)
			//pfNEDVeloc[4]: Modulo proyeccion en tierra (m/s)
			//pfNEDVeloc[5]: Angulo polar (� dec.)
		memcpy(posicion_GPS, tDatosImu.pdLLHPos,   sizeof(posicion_GPS)
			//pdLLHPos[0]: Latitud (� dec.).
			//pdLLHPos[1]: Longitud (� dec.).
			//pdLLHPos[2]: Altura sobre la elipsoide (WGS 84) (m).
			//pdLLHPos[3]: Altura sobre el nivel del mar (MSL) (m).
		pthread_mutex_unlock(&mDatosImu);
		

		veloc_ang_WRL[2]=veloc_ang_WRL[1];
		veloc_ang_WRL[1]=veloc_ang_WRL[0];
		pthread_mutex_lock(&mut_motor1);
			veloc_ang_WRL[0]= motor1.rpm;				//Motor1=WRL
		pthread_mutex_unlock(&mut_motor1);
		veloc_ang_WRL[0]= RPM2RAD * veloc_ang_WRL[0];
		
		veloc_ang_WFL[2]=veloc_ang_WFL[1];
		veloc_ang_WFL[1]=veloc_ang_WFL[0];
		pthread_mutex_lock(&mut_motor2);
			veloc_ang_WFL[0]= motor2.rpm;				 //Motor2=WFL
		pthread_mutex_unlock(&mut_motor2);			
		veloc_ang_WFL[0]= RPM2RAD * veloc_ang_WFL[0];

		veloc_ang_WRR[2]=veloc_ang_WRR[1];
		veloc_ang_WRR[1]=veloc_ang_WRR[0];
		pthread_mutex_lock(&mut_motor3);
			veloc_ang_WRR[0]= motor3.rpm;				//Motor3=WRR
		pthread_mutex_unlock(&mut_motor3);
		veloc_ang_WRR[0]= RPM2RAD * veloc_ang_WRR[0];

		veloc_ang_WFR[2]=veloc_ang_WFR[1];
		veloc_ang_WFR[1]=veloc_ang_WFR[0];
		pthread_mutex_lock(&mut_motor4);
			veloc_ang_WFR[0]= motor4.rpm;				//Motor4=WFR
		pthread_mutex_unlock(&mut_motor4);
		veloc_ang_WFR=[0] RPM2RAD * veloc_ang_WFR[0];

for(i=0; i<3;i++){
	aceleracion[i]=	GRAVEDAD*aceleracion[i];
}


//SE�AL DE DIRECCION
Steering_angle = 0.9280*volante_local - 3.8254; //Entrada de volante en tension, salida de engulo en radianes
Delta_WFL      = -0.2086*pow(volante_local,4) +  3.6383*pow(volante_local,3) - 23.4499*pow(volante_local,2) +  67.1542*volante_local -  72.9841;
Delta_WFR      = -0.7504*pow(volante_local,4) + 12.5845*pow(volante_local,3) - 79.0022*pow(volante_local,2) + 220.9029*volante_local - 232.9891;

//VELOCIDAD: Cambio del Sistema NED a IMU
 //Entradas: Velocidad del GPS en ejes NED, angulos Euler en Ejes IMU.
 //Salidas : Velocidad del COG en ejes 
	CambioSR_Veloc(velocidad_GPS, ang_Euler, velocidad_COG); 
//Vale la misma funci�n de CambioSR_Veloc pero con angulos [180 0 180]
	velocidad_COG[0]=-velocidad_COG[0];  //Eje X
	velocidad_COG[2]=-velocidad_COG[2];  //Eje Z
	Beta = atan2(velocidad_COG[1],velocidad_COG[0]);
//ACELERACION: 
  //Vale la misma funci�n de CambioSR_Veloc pero con angulos [180 0 180]
	aceleracion[0]=-aceleracion[0]; //Eje X
	aceleracion[2]=-aceleracion[2]; //Eje Z
	acelerometrox[1]=acelerometrox[0];
	acelerometrox[0]=0.15*aceleracion[0] + (1-0.15)*acelerometrox[1];
	acelerometroy[1]=acelerometroy[0];
	acelerometroy[0]=0.15*aceleracion[1] + (1-0.15)*acelerometroy[1];;

	
//Calculo de velocidad en Rueda 1 - EJES COCHE=EJES RUEDA - Trasera Izquierda
veloc_WRL[0] = velocidad_COG[3]*cos(Beta) - velocidad_Ang[2]*DIST_R_LEFT;
veloc_WRL[1] = velocidad_COG[3]*sin(Beta) - velocidad_Ang[2]*DIST_R_LX;
//Actualizaci�n del vector de X para derivar
veloc_WRL_x[2] = veloc_WRL_x[1];
veloc_WRL_x[1] = veloc_WRL_x[0];
//Calculo del modulo de la rueda
veloc_mod_WRL = sqrt(  pow(veloc_WRL[0],2) + pow(veloc_WRL[1],2) );
//Angulo de deriva de la rueda
ang_deriva_RL = atan2(veloc_WRL[1],veloc_WRL[0]);


/*Calculo de velocidad en Rueda 2 - EJES COCHE - Frontal Izquierda*/
veloc_WFL_src[0] = velocidad_COG[3]*cos(Beta) - velocidad_Ang[2]*DIST_F_LEFT;
veloc_WFL_src[1] = velocidad_COG[3]*sin(Beta) + velocidad_Ang[2]*DIST_F_LX;	
/*Conversion a EJES RUEDA*/
veloc_WFL[0] =  cos(Delta_WFL)*veloc_WFL_src[0] +  sin(Delta_WFL)*veloc_WFL_src[1]; 
veloc_WFL[1] = -sin(Delta_WFL)*veloc_WFL_src[0] +  cos(Delta_WFL)*veloc_WFL_src[1];
/*Actualizacion del vector de componentes x (Para derivar)*/
veloc_WFL_x[2] = veloc_WFL_x[1];
veloc_WFL_x[1] = veloc_WFL_x[0];
/*Calculo del modulo de la rueda*/
veloc_mod_WFL = sqrt(  pow(veloc_WFL[0],2) + pow(veloc_WFL[1],2) );
/*Angulo de deriva de la rueda*/
ang_deriva_FL = atan2(veloc_WFL[1],veloc_WFL[0]);

/*Calculo de velocidad en Rueda 3 - EJES COCHE = EJES RUEDA - Trasera Derecha*/
veloc_WRR[0] = velocidad_COG[3]*cos(Beta) + velocidad_Ang[2]*DIST_R_RIGHT;
veloc_WRR[1] = velocidad_COG[3]*sin(Beta) - velocidad_Ang[2]*DIST_R_LX;
/*Actualizaci�n del vector de X para derivar*/
veloc_WRR_x[2] = veloc_WRR_x[1];
veloc_WRR_x[1] = veloc_WRR_x[0];
/*Calculo del modulo de la rueda*/
veloc_mod_WRR[0] = sqrt(  pow(veloc_WRR[0],2) + pow(veloc_WRR[1],2) );
/*Angulo de deriva de la rueda*/
ang_deriva_RR = atan2(veloc_WRR[1],veloc_WRR[0]);

/*Calculo de velocidad en Rueda 4 - EJES COCHE - Frontal Derecha*/
veloc_WFR_src[0] = velocidad_COG[3]*cos(Beta) + velocidad_Ang[2]*DIST_F_RIGHT;
veloc_WFR_src[1] = velocidad_COG[3]*sin(Beta) + velocidad_Ang[2]*DIST_F_LX;
/*Conversion a EJES RUEDA*/
veloc_WFR[0] =  cos(Delta_WFR)*veloc_WFR_src[0] +  sin(Delta_WFR)*veloc_WFR_src[1]; 
veloc_WFR[1] = -sin(Delta_WFR)*veloc_WFR_src[0] +  cos(Delta_WFR)*veloc_WFR_src[1];
/*Actualizaci�n del vector de X para derivar*/
veloc_WFR_x[2] = veloc_WFR_x[1];
veloc_WFR_x[1] = veloc_WFR_x[0];
/*Calculo del modulo de la rueda (A titulo informativo)*/
veloc_mod_WFR = sqrt(  pow(veloc_WFR[0],2) + pow(veloc_WFR[1],2) );
/*Angulo de deriva de la rueda*/
ang_deriva_FR = atan2(veloc_WFR[1],veloc_WFR[0]);
    

/*FILTRADO de la velocidad lineal, antes de derivar*/
veloc_WRL_x[0]=0.15*veloc_WRL[0]+(1-0.15)*veloc_WRL_x[1];
veloc_WFL_x[0]=0.15*veloc_WFL[0]+(1-0.15)*veloc_WFL_x[1];
veloc_WRR_x[0]=0.15*veloc_WRR[0]+(1-0.15)*veloc_WRR_x[1];
veloc_WFR_x[0]=0.15*veloc_WFR[0]+(1-0.15)*veloc_WFR_x[1];


/*CALCULAR LAS ACELERACIONES COMO DERIVADA DE VELOCIDAD Y FILTRADO*/
//Aceleracion lineal
	acel_est_FL[1] = acel_est_FL[0];
		acel_est_FL[0] = 0.15*(3*veloc_WFL_x[0]-4*veloc_WFL_x[1]+veloc_WFL_x[2])/(2*Tiempo_Muestreo) + (1-0.15)*acel_est_FL[1];	
	acel_est_FR[1] = acel_est_FR[0];
		acel_est_FR[0] = 0.15*(3*veloc_WFR_x[0]-4*veloc_WFR_x[1]+veloc_WFR_x[2])/(2*Tiempo_Muestreo) + (1-0.15)*acel_est_FR[1];
	acel_est_RL[1] = acel_est_RL[0];
		acel_est_RL[0] = 0.15*(3*veloc_WRL_x[0]-4*veloc_WRL_x[1]+veloc_WRL_x[2])/(2*Tiempo_Muestreo) + (1-0.15)*acel_est_RL[1];	
	acel_est_RR[1] = acel_est_RR[0];
		acel_est_RR[0] = 0.15*(3*veloc_WRR_x[0]-4*veloc_WRR_x[1]+veloc_WRR_x[2])/(2*Tiempo_Muestreo) + (1-0.15)*acel_est_RR[1];
//Aceleracion angular
	acel_ang_est_FL[1] = acel_ang_est_FL[0];
		acel_ang_est_FL[0] = 0.15*(3*veloc_WFL_x[0]-4*veloc_WFL_x[1]+veloc_WFL_x[2])/(2*Tiempo_Muestreo) + (1-0.15)*acel_ang_est_FL[1];	
	acel_ang_est_FR[1] = acel_ang_est_FR[0];
		acel_ang_est_FR[0] = 0.15*(3*veloc_WFR_x[0]-4*veloc_WFR_x[1]+veloc_WFR_x[2])/(2*Tiempo_Muestreo) + (1-0.15)*acel_ang_est_FR[1];
	acel_ang_est_RL[1] = acel_ang_est_RL[0];
		acel_ang_est_RL[0] = 0.15*(3*veloc_WRL_x[0]-4*veloc_WRL_x[1]+veloc_WRL_x[2])/(2*Tiempo_Muestreo) + (1-0.15)*acel_ang_est_RL[1];	
	acel_ang_est_RR[1] = acel_ang_est_RR[0];
		acel_ang_est_RR[0] = 0.15*(3*veloc_WRR_x[0]-4*veloc_WRR_x[1]+veloc_WRR_x[2])/(2*Tiempo_Muestreo) + (1-0.15)*acel_ang_est_RR[1];



//Calculo del Momento corrector de YawRate
	//Calculo del YR deseado
	YR_deseado[2] = YR_deseado[1];
	YR_deseado[1] = YR_deseado[0];
	//YR_deseado[0] = (velocidad_COG[0])/DIST_EJES_LX)*Steering_angle; //COMPROBAR CONDICIONES EN LAS QUE SE CUMPLE
	YR_deseado[0] = (velocidad_COG[0]*Steering_angle)/(DIST_EJES_LX +((MASS_FOX/DIST_EJES_LX)*((DIST_R_LX/CF)-(DIST_F_LX/CR))*(pow(velocidad_COG[0],2))));

	//YawRate del vehiculo
	YR_real[2] = YR_real[1];
	YR_real[1] = YR_real[0];
	YR_real[0] = velocidad_Ang[2];

	//Limites de yaw(Stuttgart)
	yawrate_lim_ay = sign(YR_real[0])*abs(acelerometroy[0]/velocidad_COG[0]);
    yawrate_lim_B  = sign(YR_real[0])*(abs(YR_real[0])-((abs(Beta)-B_low)/(B_high-B_low))*(abs(YR_real[0])-abs(yawrate_lim_ay)));


desliz_L_FL = ((DIAMETRO_WF/2)*veloc_ang_WFL[0]*cos(ang_deriva_FL)-veloc_mod_WFL)/max((DIAMETRO_WF/2)*veloc_ang_WFL[0]*cos(ang_deriva_FL),veloc_mod_WFL);
desliz_L_RL = ((DIAMETRO_WR/2)*veloc_ang_WRL[0]*cos(ang_deriva_RL)-veloc_mod_WRL)/max((DIAMETRO_WR/2)*veloc_ang_WRL[0]*cos(ang_deriva_RL),veloc_mod_WRL);
desliz_L_FR = ((DIAMETRO_WF/2)*veloc_ang_WFR[0]*cos(ang_deriva_FR)-veloc_mod_WFR)/max((DIAMETRO_WF/2)*veloc_ang_WFR[0]*cos(ang_deriva_FR),veloc_mod_WFR);
desliz_L_RR = ((DIAMETRO_WR/2)*veloc_ang_WRR[0]*cos(ang_deriva_RR)-veloc_mod_WRR)/max((DIAMETRO_WR/2)*veloc_ang_WRR[0]*cos(ang_deriva_RR),veloc_mod_WRR);

desliz_S_FL = ((DIAMETRO_WF/2)*veloc_ang_WFL[0]*sin(ang_deriva_FL))/max((DIAMETRO_WF/2)*veloc_ang_WFL[0]*cos(ang_deriva_FL),veloc_mod_WFL);
desliz_S_RL = ((DIAMETRO_WR/2)*veloc_ang_WRL[0]*sin(ang_deriva_RL))/max((DIAMETRO_WR/2)*veloc_ang_WRL[0]*cos(ang_deriva_RL),veloc_mod_WRL);
desliz_S_FR = ((DIAMETRO_WF/2)*veloc_ang_WFR[0]*sin(ang_deriva_FR))/max((DIAMETRO_WF/2)*veloc_ang_WFR[0]*cos(ang_deriva_FR),veloc_mod_WFR);
desliz_S_RR = ((DIAMETRO_WR/2)*veloc_ang_WRR[0]*sin(ang_deriva_RR))/max((DIAMETRO_WR/2)*veloc_ang_WRR[0]*cos(ang_deriva_RR),veloc_mod_WRR);

desliz_res_FL=norm([desliz_L_FL desliz_S_FL]);
desliz_res_RL=norm([desliz_L_RL desliz_S_RL]);
desliz_res_FR=norm([desliz_L_FR desliz_S_FR]);
desliz_res_RR=norm([desliz_L_RR desliz_S_RR]);

//Coeficiente de pseudodeslizamiento de cada rueda (Slip Rate)
pseudodes[0]= ((DIAMETRO_WR/2)*veloc_ang_WRL[0] - veloc_WRL_x[0])/max((DIAMETRO_WR/2)*veloc_ang_WRL[0],veloc_WRL_x[0]);
pseudodes[1]= ((DIAMETRO_WF/2)*veloc_ang_WFL[0] - veloc_WFL_x[0])/max((DIAMETRO_WF/2)*veloc_ang_WFL[0],veloc_WFL_x[0]);
pseudodes[2]= ((DIAMETRO_WR/2)*veloc_ang_WRR[0] - veloc_WRR_x[0])/max((DIAMETRO_WR/2)*veloc_ang_WRR[0],veloc_WRR_x[0]);
pseudodes[3]= ((DIAMETRO_WF/2)*veloc_ang_WFR[0] - veloc_WFR_x[0])/max((DIAMETRO_WF/2)*veloc_ang_WFR[0],veloc_WFR_x[0]);

nu_WFL = c1*(1-exp^(-c2*desliz_res_FL)) -c3*desliz_res_FL;
nu_WRL = c1*(1-exp^(-c2*desliz_res_RL)) -c3*desliz_res_RL;
nu_WFR = c1*(1-exp^(-c2*desliz_res_FR)) -c3*desliz_res_FR;
nu_WRR = c1*(1-exp^(-c2*desliz_res_RR)) -c3*desliz_res_RR;


//Calculo del error en derivada de yawrate
	//Derivada_YRdeseado =  (3*YR_deseado[0] - 4*YR_deseado[1] + YR_deseado[2])/(2*Tiempo_Muestreo);
	//Derivada_YRreal    =  (3*YR_real[2]    - 4*YR_real[1]    + YR_real[2])/(2*Tiempo_Muestreo);
	//error_yr_derivada[2] = error_yr_derivada[1];
	//error_yr_derivada[1] = error_yr_derivada[0];
	//error_yr_derivada[0] = Derivada_YRreal - Derivada_YRdeseado
	//Momento_corrector = K_yawcontrol*(J_FOX_ZZ*error_yr_derivada[0]);  //Error en momento (Nm)
	
	//if abs(Momento_corrector) > 700
	//	Momento_corrector=sign(Momento_corrector)*700;


//CALCULO DE Fz
Fz_FL = MASS_FOX*(DIST_R_LX*Grav/DIST_EJES_LX - hcog*acelerometrox[0]/DIST_EJES_LX)*(0.5-HCOG*acelerometroy[0]/(Grav*(DIST_F_LEFT+DIST_F_RIGHT)));
Fz_FR = MASS_FOX*(DIST_R_LX*Grav/DIST_EJES_LX - hcog*acelerometrox[0]/DIST_EJES_LX)*(0.5+HCOG*acelerometroy[0]/(Grav*(DIST_F_LEFT+DIST_F_RIGHT)));
Fz_RL = MASS_FOX*(DIST_F_LX*Grav/DIST_EJES_LX + hcog*acelerometrox[0]/DIST_EJES_LX)*(0.5-HCOG*acelerometroy[0]/(Grav*(DIST_R_LEFT+DIST_R_RIGHT)));
Fz_RR = MASS_FOX*(DIST_F_LX*Grav/DIST_EJES_LX + hcog*acelerometrox[0]/DIST_EJES_LX)*(0.5+HCOG*acelerometroy[0]/(Grav*(DIST_R_LEFT+DIST_R_RIGHT)));

//PRIMERA CAPA: ASIGNO PAR PILOT
veloc_ang ={veloc_ang_WRL[0], veloc_ang_WFL[0], veloc_ang_WRR[0], veloc_ang_WFR[0]};
for (i=0;i<NUM_MOTORES;i++){
	if veloc_ang[i] > 115{
		if veloc_ang[i] < 140
			Parmax[i]=0.0054584*pow(veloc_ang[i],3) + -2.1561*pow(veloc_ang[i],2) + 279.6976*veloc_ang[i] + -11874.595;
		else
			Parmax[i]=1;
		}
	else
		Parmax[i]=80;
}
Pardrive
for (i=0;i<NUM_MOTORES;i++){
	if Parmax[i] > trac_acelera_local[i][i]{
		Par_avail[i]=(Parmax[i]-trac_acelera_local[i]);
	}
	else
		Par_avail[i]=0;
}
	
Par_inicial_met1 = Relacion_ParPedal*acelera_local*[1,1,1,1]; //En Nm
Par_inicial_met2 = trac_acelera_local + (acelera_local/5)*Par_avail; //En Nm

//SEGUNDA CAPA:GIRO
R_vlow = abs(DIST_EJES_LX/Steering_angle);
R_vhigh= abs(velocidad_COG[3]/velocidad_Ang[2]);

if velocidad_COG[3] <(20/3.6){
	RadioGiro = R_vlow;
}
else{
	if velocidad_COG[3] < (80/3.6){
		RadioGiro=((80-velocidad_COG[3]*3.6)/(80-20))*R_vlow + (1-((80-3.6*velocidad_COG[3])/(80-20)))*R_vhigh;
	}
    else
        RadioGiro = R_vhigh;
}
    
RFL = sqrt(pow(RadioGiro-sign(Steering_angle)*DIST_F_LEFT,2)+pow(DIST_EJES_LX,2));
RFR = sqrt(pow(RadioGiro+sign(Steering_angle)*DIST_F_RIGHT,2)+pow(DIST_EJES_LX,2));
RRL = RadioGiro-sign(Steering_angle)*DIST_R_LEFT;
RRR = RadioGiro+sign(Steering_angle)*DIST_R_RIGHT;
//El par debe ser el acorde a la velocidad y el giro �?
dM= 1+3*(([RFL RRL RFR RRR]-RadioGiro)/RadioGiro);


	// Ecuacion Momento generado por los pares de la rueda (Par corrector de yaw solo)
	//  Mz=sum( Ai*Par_i)
	//A1= (-DIST_F_LEFT*cos(Delta_WFL)  + DIST_F_LX*sin(Delta_WFL))/(DIAMETRO_WF/2);
	//A3= ( DIST_F_RIGHT*cos(Delta_WFR) + DIST_F_LX*sin(Delta_WFR))/(DIAMETRO_WF/2);
	//Momento_provocado = A1*Par_inicial(1) + A3*Par_inicial(3)  +A2*Par_inicial(2) + A4*Par_inicial(4);

//	Par_FL_Mz=(Momento_corrector-Momento_provocado)*0.3/A1; //Momento en Z generado al aplicar par en la rueda Par_ij
//	Par_FR_Mz=(Momento_corrector-Momento_provocado)*0.3/A3;
//	Par_RL_Mz=(Momento_corrector-Momento_provocado)*0.2/A2;
//	Par_RR_Mz=(Momento_corrector-Momento_provocado)*0.2/A4;
//	dM=[Par_RL_Mz Par_FL_Mz Par_RR_Mz Par_FR_Mz;]
Par_calculado = Par_inicial_met1/Relacion_ParPedal; //EN V

//MTTE
alpha_FL[1]=alpha_FL[0];
alpha_RL[1]=alpha_RL[0];
alpha_FR[1]=alpha_FR[0];
alpha_RR[1]=alpha_RR[0];
alpha_FL[0] = 0.15*(acel_est_FL[0]/((DIAMETRO_WF/2)*acel_ang_est_FL[0])) + 0.85*alpha_FL[1];
alpha_RL[0] = 0.15*(acel_est_RL[0]/((DIAMETRO_WR/2)*acel_ang_est_RL[0])) + 0.85*alpha_RL[1];
alpha_FR[0] = 0.15*(acel_est_FR[0]/((DIAMETRO_WF/2)*acel_ang_est_FR[0])) + 0.85*alpha_FR[1];
alpha_RR[0] = 0.15*(acel_est_RR[0]/((DIAMETRO_WR/2)*acel_ang_est_RR[0])) + 0.85*alpha_RR[1];
    
Ffricc_FL[1] = Ffricc_FL[0];
Ffricc_FL[0] = (Par_calculado[1]*Relacion_ParPedal - J_WF*acel_ang_est_FL[0])/(DIAMETRO_WF/2); //Fuerza de Friccion en la rueda Frontal Izquierda (M2)
Ffricc_RL[1] = Ffricc_RL[0];
Ffricc_RL[0] = (Par_calculado[0]*Relacion_ParPedal - J_WR*acel_ang_est_RL[0])/(DIAMETRO_WR/2); //Fuerza de Friccion en la rueda Trasera Izquierda (M1)
Ffricc_FR[1] = Ffricc_FR[0];
Ffricc_FR[0] = (Par_calculado[3]*Relacion_ParPedal - J_WF*acel_ang_est_FR[0])/(DIAMETRO_WF/2); //Fuerza de Friccion en la rueda Frontal Derecha   (M4)
Ffricc_RR[1] = Ffricc_RR[0];
Ffricc_RR[0] = (Par_calculado[2]*Relacion_ParPedal - J_WR*acel_ang_est_RR[0])/(DIAMETRO_WR/2); //Fuerza de Friccion en la rueda Trasera Derecha   (M3)

T_max_FL = (DIAMETRO_WF/2)*Ffricc_FL[0]*( 1 + J_WF/(alpha_FL[0]*MASS_FOX*(DIAMETRO_WF/2)^2));
T_max_RL = (DIAMETRO_WR/2)*Ffricc_RL[0]*( 1 + J_WR/(alpha_RL[0]*MASS_FOX*(DIAMETRO_WR/2)^2));
T_max_FR = (DIAMETRO_WF/2)*Ffricc_FR[0]*( 1 + J_WF/(alpha_FR[0]*MASS_FOX*(DIAMETRO_WF/2)^2));
T_max_RR = (DIAMETRO_WR/2)*Ffricc_RR[0]*( 1 + J_WR/(alpha_RR[0]*MASS_FOX*(DIAMETRO_WR/2)^2));  
T_max =[T_max_RL/Relacion_ParPedal, T_max_FL/Relacion_ParPedal, T_max_RR/Relacion_ParPedal, T_max_FR/Relacion_ParPedal];


for (i=0;i<NUM_MOTORES;i++){
	if (abs(Par_calculado[i]) > T_max[i]){
		trac_acelera_local[i]= sign(Par_calculado[i])*T_max[i];
	}
	else{
		trac_acelera_local[i]= Par_calculado[i];
	}
}

for (i=0;i<NUM_MOTORES;i++){
	//if (abs(trac_acelera_local[i]) > 5){
	//	trac_acelera_local[i]= sign(trac_acelera_local[i])*5;
	//}
	trac_acelera_local[i]=acelera_local;

}



//calculo_angulo_volante(volante_local, &volante_ang);
//calculo_curvatura_ackermann(volante_ang, &curvatura_acker);
//yaw_rate_des=curvatura_acker*velocidad_local;
//error_yr=yaw_rate_des-yaw_rate_local;
//ctrl_pi(error_yr, &u_pi);
/* A partir del resultado del controlador se calcula la distribucion
		dist[0]=u_pi*(1-u_pi);	// Interior trasera.
		dist[1]=pow((1-u_pi),2); //Interior delantera.
		dist[2]=pow(u_pi,2);	// Exterior trasera.
		dist[3]=u_pi*(1-u_pi);	// Exterior delantera.

		if (volante_ang<0) //Giro a la izquierda. Se hace diferencia porque las ruedas exteriores e interiores cambian.
		{
			for (i=0;i<NUM_MOTORES;i++)
				senhal_motores[i]=dist[i]*NUM_MOTORES*acelerador_local;
		}else //Giro a la derecha
		{
			for (i=0;i<NUM_MOTORES/2;i++)
				senhal_motores[i]=dist[i+NUM_MOTORES/2]*NUM_MOTORES*acelerador_local;
			for (i=NUM_MOTORES/2;i<NUM_MOTORES;i++)
				senhal_motores[i]=dist[i-NUM_MOTORES/2]*NUM_MOTORES*acelerador_local;
		}

		for (i=0;i<NUM_MOTORES;i++) //Saturacion
		{
			if (senhal_motores[i]>5)
				senhal_motores[i]=5;
		}


		if (k%100==0)
		{
			printf("k=%d\n",k);
			for (i=0;i<NUM_MOTORES;i++)
				printf("dist[%d]=%f, senhal_motores[%d]=%f\n",i,dist[i],i,senhal_motores[i]);
		}
*/

		if(k<NUMERO_MUESTRAS)
		{
			/*buffer[TAMANHO_MUESTRA*k+0]=volante_local;
			buffer[TAMANHO_MUESTRA*k+1]=acelerador_local;
			buffer[TAMANHO_MUESTRA*k+2]=yaw_rate_local;
			buffer[TAMANHO_MUESTRA*k+3]=velocidad_local;
			buffer[TAMANHO_MUESTRA*k+4]=volante_ang;
			buffer[TAMANHO_MUESTRA*k+5]=curvatura_acker;
			buffer[TAMANHO_MUESTRA*k+6]=yaw_rate_des;
			buffer[TAMANHO_MUESTRA*k+7]=u_pi;
			buffer[TAMANHO_MUESTRA*k+8]=dist[0];
			buffer[TAMANHO_MUESTRA*k+9]=dist[1];
			buffer[TAMANHO_MUESTRA*k+10]=dist[2];
			buffer[TAMANHO_MUESTRA*k+11]=dist[3];
			buffer[TAMANHO_MUESTRA*k+12]=senhal_motores[0];
			buffer[TAMANHO_MUESTRA*k+13]=senhal_motores[1];
			buffer[TAMANHO_MUESTRA*k+14]=senhal_motores[2];
			buffer[TAMANHO_MUESTRA*k+15]=senhal_motores[3];
			buffer[TAMANHO_MUESTRA*k+16]=latitud;
			buffer[TAMANHO_MUESTRA*k+17]=longitud;*/
			
			bufferjc[NMUESTRAJC*k+0]	=posicion_GPS[0]; //latitud
			bufferjc[NMUESTRAJC*k+1]	=posicion_GPS[1]; //Longitud
			bufferjc[NMUESTRAJC*k+2]	= aceleracion[0];//Acelerometrox sin filtro
			bufferjc[NMUESTRAJC*k+3]	= aceleracion[1];//Acelerometroy sin filtro
			bufferjc[NMUESTRAJC*k+4]	= aceleracion[2];//Acelerometroz sin filtro
			bufferjc[NMUESTRAJC*k+5]	= acelerometrox[0];//Acelerometrox con filtro
			bufferjc[NMUESTRAJC*k+6]	= acelerometroy[0];//Acelerometrox con filtro
			bufferjc[NMUESTRAJC*k+7]	= velocidad_Ang[0];//veloc ang coche x
			bufferjc[NMUESTRAJC*k+8]	= velocidad_Ang[1];//veloc ang coche y
			bufferjc[NMUESTRAJC*k+9]	= velocidad_Ang[2];//veloc ang coche z (YR)
			bufferjc[NMUESTRAJC*k+10]	=ang_Euler[0];//Roll
			bufferjc[NMUESTRAJC*k+11]	=ang_Euler[1];//Pitch
			bufferjc[NMUESTRAJC*k+12]	=ang_Euler[2];//Yaw
			bufferjc[NMUESTRAJC*k+13]	=velocidad_GPS[0];//vx_IMU
			bufferjc[NMUESTRAJC*k+14]	=velocidad_GPS[1];//vy_IMU
			bufferjc[NMUESTRAJC*k+15]	=velocidad_GPS[2];//vz_IMU
			bufferjc[NMUESTRAJC*k+16]	=velocidad_GPS[3];//vmod_IMU
			bufferjc[NMUESTRAJC*k+17]	=velocidad_COG[0];//vx_COG
			bufferjc[NMUESTRAJC*k+18]	=velocidad_COG[1];//vy_COG
			bufferjc[NMUESTRAJC*k+19]	=velocidad_COG[3];//modulo_vCOG

			bufferjc[NMUESTRAJC*k+20]	=veloc_ang_WRL[0];//En rad/s
			bufferjc[NMUESTRAJC*k+21]	=veloc_ang_WFL[0];
			bufferjc[NMUESTRAJC*k+22]	=veloc_ang_WRR[0];
			bufferjc[NMUESTRAJC*k+23]	=veloc_ang_WFR[0];
			bufferjc[NMUESTRAJC*k+24]	=veloc_WRL[0];
			bufferjc[NMUESTRAJC*k+25]	=veloc_WRL[1];
			bufferjc[NMUESTRAJC*k+26]	=veloc_WFL[0];
			bufferjc[NMUESTRAJC*k+27]	=veloc_WFL[1];
			bufferjc[NMUESTRAJC*k+28]	=veloc_WRR[0];
			bufferjc[NMUESTRAJC*k+29]	=veloc_WRR[1];
			bufferjc[NMUESTRAJC*k+30]	=veloc_WFR[0];
			bufferjc[NMUESTRAJC*k+31]	=veloc_WFR[1];
			bufferjc[NMUESTRAJC*k+32]	=acel_est_RL[0];
			bufferjc[NMUESTRAJC*k+33]	=acel_est_FL[0];
			bufferjc[NMUESTRAJC*k+34]	=acel_est_RR[0];
			bufferjc[NMUESTRAJC*k+35]	=acel_est_FR[0];
			bufferjc[NMUESTRAJC*k+36]	=acel_ang_est_RL[0];
			bufferjc[NMUESTRAJC*k+37]	=acel_ang_est_FL[0];
			bufferjc[NMUESTRAJC*k+38]	=acel_ang_est_RR[0];
			bufferjc[NMUESTRAJC*k+39]	=acel_ang_est_FR[0];

			bufferjc[NMUESTRAJC*k+40] = volante_local;
			bufferjc[NMUESTRAJC*k+41] = freno_local;
			bufferjc[NMUESTRAJC*k+42] = acelera_local;
			bufferjc[NMUESTRAJC*k+43] = Steering_angle;	
			bufferjc[NMUESTRAJC*k+44] = susp_ti[0];
			bufferjc[NMUESTRAJC*k+45] = susp_td[0];
			bufferjc[NMUESTRAJC*k+46] = susp_di[0];
			bufferjc[NMUESTRAJC*k+47] = susp_dd[0];
			
			bufferjc[NMUESTRAJC*k+48] = desliz_L_RL;
			bufferjc[NMUESTRAJC*k+49] = desliz_L_FL;
			bufferjc[NMUESTRAJC*k+50] = desliz_L_RR;
			bufferjc[NMUESTRAJC*k+51] = desliz_L_FR;
			bufferjc[NMUESTRAJC*k+52] = desliz_S_RL;
			bufferjc[NMUESTRAJC*k+53] = desliz_S_FL;
			bufferjc[NMUESTRAJC*k+54] = desliz_S_RR;
			bufferjc[NMUESTRAJC*k+55] = desliz_S_FR;
			bufferjc[NMUESTRAJC*k+56] = pseudodes[0];
			bufferjc[NMUESTRAJC*k+57] = pseudodes[1];
			bufferjc[NMUESTRAJC*k+58] = pseudodes[2];
			bufferjc[NMUESTRAJC*k+59] = pseudodes[3];
			bufferjc[NMUESTRAJC*k+60] = nu_WRL;
			bufferjc[NMUESTRAJC*k+61] = nu_WFL;
			bufferjc[NMUESTRAJC*k+62] = nu_WRR;
			bufferjc[NMUESTRAJC*k+63] = nu_WFR;
						
			bufferjc[NMUESTRAJC*k+64] = Fz_RL;
			bufferjc[NMUESTRAJC*k+65] = Fz_FL;
			bufferjc[NMUESTRAJC*k+66] = Fz_RR;
			bufferjc[NMUESTRAJC*k+67] = Fz_FR;
			bufferjc[NMUESTRAJC*k+68] = RadioGiro;
			
			bufferjc[NMUESTRAJC*k+69] = alpha_RL[0];
			bufferjc[NMUESTRAJC*k+70] = alpha_FL[0];
			bufferjc[NMUESTRAJC*k+71] = alpha_RR[0];
			bufferjc[NMUESTRAJC*k+72] = alpha_FR[0];
			bufferjc[NMUESTRAJC*k+73] = Ffricc_RL[0];
			bufferjc[NMUESTRAJC*k+74] = Ffricc_FL[0];
			bufferjc[NMUESTRAJC*k+75] = Ffricc_RR[0];
			bufferjc[NMUESTRAJC*k+76] = Ffricc_FR[0];
			bufferjc[NMUESTRAJC*k+77] = T_max_RL;
			bufferjc[NMUESTRAJC*k+78] = T_max_FL;
			bufferjc[NMUESTRAJC*k+79] = T_max_RR;
			bufferjc[NMUESTRAJC*k+80] = T_max_FR;
		

			k++;

			//Por seguridad, para probar:
			for (i=0;i<NUM_MOTORES;i++)
				senhal_motores[i]=acelerador_local;
		}
		else 
		{
			printf("flag_archivo=%d, k=%d",(int)flag_archivo,k);
			for (i=0;i<4;i++)
				senhal_motores[i]=0;
			pthread_mutex_lock(&mut_potencias);
				memcpy(potencias.acelerador, senhal_motores, sizeof(senhal_motores));
			pthread_mutex_unlock(&mut_potencias);
			
			if(flag_archivo){
				clock_gettime(CLOCK_REALTIME,&tiempo_actual);
				sprintf(nombre_archivo,"/datos/log_ecu/ctrl_pi_%d.txt",(int)tiempo_actual.tv_sec);
				printf("Se acabaron los 5 minutos\n");
				printf("Guardando archivo en %s\n",nombre_archivo);
				//fp=fopen(nombre_archivo,"w");
				//fprintf(fp,"volante_local\t acelerador_local\t yaw_rate_local\t velocidad_local\t volante_ang\t curvatura_acker\t yaw_rate_des\t u_pi\t dist[0]\t dist[1]\t dist[2]\t dist[3]\t senhal_motores[0]\t senhal_motores[1]\t senhal_motores[2]\t senhal_motores[3]\t latitud\t longitud\n");
				flag_archivo=0;
				/*for(i=0;i<NUMERO_MUESTRAS;i++)
				{
					for(j=0;j<TAMANHO_MUESTRA;j++)
					{
						fprintf(fp,"%.12f\t",buffer[TAMANHO_MUESTRA*i+j]);
					}
					fprintf(fp,"\n");
				}
				fclose(fp);*/


				sprintf(nombre_archivo,"/datos/log_ecu/ctrl_jose_carlos_%d.txt",(int)tiempo_actual.tv_sec);
				fp=fopen(nombre_archivo,"w");
				for(i=0;i<NUMERO_MUESTRAS;i++){
					for(j=0;j<NMUESTRAJC;j++){
						fprintf(fp,"%.12f\t",bufferjc[NMUESTRAJC*i+j]);
					}
					fprintf(fp,"\n");
				}
				fclose(fp);
				printf("Archivos guardados \n\n",nombre_archivo);
			}			
		}
		
		//Espera al temporizador
		sigwaitinfo(&cjto_temp, NULL);


		/* Actualizacion de variable global */
		pthread_mutex_lock(&mut_potencias);
		memcpy(potencias.acelerador, senhal_motores, sizeof(senhal_motores));
		pthread_mutex_unlock(&mut_potencias);


//		nanosleep (&espera, NULL);

		pthread_mutex_lock(&mut_hilos_listos);
		if ((hilos_listos&HILO_TRAC_LISTO)==0x0000)
			fin_local = 1;
		pthread_mutex_unlock(&mut_hilos_listos);
	} // Fin bucle while

	pthread_exit(NULL);
	return(NULL);
}


void calculo_angulo_volante(float volante_local, float *volante_ang)
{
	/* volante_local es la medida del volante en voltios, y volante_ang es el giro del volante en grados.*/
	static float vol_min=3.8, vol_max=4.4; //valores iniciales, que seran superados en la calibracion.

	//Calibracion:
	if (vol_min==0||volante_local<vol_min)
		vol_min=volante_local;
	else if (vol_max==0||volante_local>vol_max)
		vol_max=volante_local;

	// Calculo del angulo del volante
	*volante_ang=(ANG_MIN_VOL-ANG_MAX_VOL)*(volante_local-vol_min)/(vol_max-vol_min)+ANG_MAX_VOL;
}
// Definiciones a anhadir en el .h:

#define ANG_MIN_VOL -180.0
#define ANG_MAX_VOL 180.0
#define MINIMO_U_PI 0.2
#define MAXIMO_U_PI 0.8

// Cabeceras a anhadir en el .h:
void calculo_angulo_volante(float volante_local, float *volante_ang);
void calculo_curvatura_ackermann(float volante_ang, float *curvatura_acker);
void ctrl_pi(float error_yr, float *u_pi);

void calculo_curvatura_ackermann(float volante_ang, float *curvatura_acker)
{
	/*volante_ang es el angulo del volante en grados, y curvatura acker es 1/radio_ackermann, en m^-1.*/
	/* Aunque en Matlab se calcula por tablas, la relacion es practicamente lineal */
	*curvatura_acker=0.001081*volante_ang;
}

void ctrl_pi(float error_yr, float *u_pi)
{
	/*Controlador PI. error_yr es el error en yaw rate y u_pi es la senhal de control*/
	static float integral_error=0, u_pi_local=0;
	float kp=1.0;
	float ki=20.0;
	float t_muestreo;

	t_muestreo=TIEMPO_BUCLE_NS/1000000000.0;

	if (!((error_yr*u_pi_local>0)&&(u_pi_local==MAXIMO_U_PI||u_pi_local==MINIMO_U_PI))) //Con este if se hace el antiwindup
		integral_error+=error_yr*t_muestreo;

	u_pi_local=0.5+kp*error_yr+ki*integral_error;

	if (u_pi_local>MAXIMO_U_PI)
		u_pi_local=MAXIMO_U_PI;
	else if (u_pi_local<MINIMO_U_PI)
		u_pi_local=MINIMO_U_PI;
		
	*u_pi=u_pi_local;
}















void carga_modelo(int estado, float *K, float *F, float *M, float *N){
/*
	if estado==0{
		fp = fopen("K15.txt", "r");

		for (j = 0; j < 10; j++) {
			for (i = 0; i < 50; i++) {
				fscanf(fp,"%f",&K[j][i]);
			}
		}

		fclose(fp);
*/
		/*Leemos las matrices del modelo de Modelo.txt*/
/*		fp = fopen("Modelo15.txt", "r");

		for (j = 0; j < 50; j++) {
			for (i = 0; i < 6; i++) {
				fscanf(fp,"%f",&F[j][i]);
			}
		}
		for (j = 0; j < 6; j++) {
			for (i = 0; i < 6; i++) {
				fscanf(fp,"%f",&M[j][i]);
			}
		}
		for(i=0;i<6;i++){
			fscanf(fp,"%f",&N[i]);
		}

		fclose(fp);
	}

	if(estado==1){
		fp = fopen("K50.txt", "r");

		for (j = 0; j < 10; j++) {
			for (i = 0; i < 50; i++) {
				fscanf(fp,"%f",&K[j][i]);
			}
		}

		fclose(fp);

		fp = fopen("Modelo50.txt", "r");

		for (j = 0; j < 50; j++) {
			for (i = 0; i < 6; i++) {
				fscanf(fp,"%f",&F[j][i]);
			}
		}
		for (j = 0; j < 6; j++) {
			for (i = 0; i < 6; i++) {
				fscanf(fp,"%f",&M[j][i]);
			}
		}
		for(i=0;i<6;i++){
			fscanf(fp,"%f",&N[i]);
		}

		fclose(fp);
	}*/
}


   
void *ctrl_estabilidad_main (void *pi)
{
	/**************************************************
	***		VARIABLES			***
	**************************************************/

	float freno_local;			// Valor del freno accionado por el conductor
	float acelera_local;		// Valor del freno accionado por el conductor
	BOOL act_acel_local;
	BOOL motor_on_local[NUM_MOTORES];

	float trac_freno_local[NUM_MOTORES];	// Valor de freno que se manda a cada motor
	float trac_acelera_local[NUM_MOTORES];
	BOOL act_freno_local[NUM_MOTORES];


	/*Variables locales*/
	float volante_local;    //V en potenciemetro del volante (0-5V)
	float aceleracion[3];   //Valor del aceleremetro (en g): Aceleracien del vehiculo en el COG en ejes locales
	float velocidad_Ang[3]; //Valor del girescopo: Velocidad angular (rad/s) del COG en ejes locales
	float yaw_r[10];			//Yaw rate: velocidad angular alrededor del eje Z GONZALO
	float ang_Euler[3];     //Valor del inclinemetro: [Roll Pitch Yaw] (rad)
	float velocidad_GPS[6]; //Velocidad lineal del COG medida por el gps (m/s)
	float velocidad_COG[4]; //Velocidad lineal del COG  (m/s)

	float Beta;         //Angulo que forma la velocidad con el sistema de ejes local (SRNI)//
	float sslip[10];		//Mismo engulo pero en tabla para guardar su valor anterior GONZALO

	float d_sslip;		//Derivada del engulo sideslip GONZALO
	float d_yaw_r;		//Derivada de la velocidad angular GONZALO

	/*Velocidad lineal de las ruedas[x,y] segun sus propios ejes, Modulo de Velocidad(Actual y Anterior), Velocidad Angular*/
	float veloc_WFL[2], veloc_WFR[2], veloc_WRL[2], veloc_WRR[2];
	float veloc_WFL_src[2], veloc_WFR_src[2];
	float veloc_mod_WFL[10] ={0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	float veloc_ang_WFL[10] ={0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	float veloc_mod_WFR[10] ={0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	float veloc_ang_WFR[10] ={0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	float veloc_mod_WRL[10] ={0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	float veloc_ang_WRL[10] ={0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	float veloc_mod_WRR[10] ={0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	float veloc_ang_WRR[10] ={0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	int flag=0;

	/*Fuerza de Friccion en el neumatico,
		  Par aplicado en la rueda aplicado,
		  Aceleracion angular "estimada",
		  Aceleracion lineal "estimada",
		  Paremetro auxiliar alpha,
		  Par meximo aplicable a la rueda*/

	float Ffricc_FR, acel_ang_est_FR,acel_est_FR, alpha_FR, T_max_FR=0;
	float Ffricc_FL, acel_ang_est_FL,acel_est_FL, alpha_FL, T_max_FL=0;
	float Ffricc_RR, acel_ang_est_RR,acel_est_RR, alpha_RR, T_max_RR=0;
	float Ffricc_RL, acel_ang_est_RL,acel_est_RL, alpha_RL, T_max_RL=0;
	float T_max[4];  //Nivel de tension correspondiente al Par Maximo Aplicable
	float YR_d;      //Yaw rate deseado (rad/s)

	float Steering_angle, Delta_WFL,  Delta_WFR; //Direccion del vehiculo, direccion ruedas delanteras
	float Momento_necesario; //Momento que hay que aplicar al vehiculo,Variables auxiliar en el calculo del par diferencial
	float dM[4]; //Diferencial del par a aplicar

	//Variables MPC
	float Xext[6]={0,0,0,0,0,0}; //Vector estados ampliado
	float Xext2[6]={0,0,0,0,0,0};
	float w[NP*2]; //Referencia
	float f[NP*2]; //Salidas estimados
	float F[NP*2][6];
	float ef[NP*2]; //Vector de errores
	float u[NU]; //Actuaciones futuras
	float deltaU; //Valor incremental de la actuacien
	float K[NU][NP*2];
	float N[6];
	float M[6][6];

	//Variables para el temporizador:
	sigset_t cjto_temp;
	timer_t tempo;
	struct sigaction accion;
	struct sigevent eventos_temp;
	struct itimerspec prg_temp;
	struct timespec periodo_temp;

	//Limitador de momento correctivo
	float error_yr;
	float error_min;
	float por_act;

	//Actualizacion de modelo
	int estado;
	int modelo;

	//Calibracien del volante
	float vol_min=3.99;
	float vol_max=4.01;

	//Variables diferencial central
	float diff,difr;
	float Mzf,Mzr;
	float Par[4]; 
	float T_max_mot;
	float T_min_mot;
	float Relacion_ParPedal=7.30964; //<----Incluir como DEFINE

	struct timespec espera = {0, TIEMPO_BUCLE_NS};
	BOOL fin_local = 0;
	
	float susp_ti;
	
	int i, j, k;
	int ii;
	FILE *fp;
	
	//Volcado de variables en vectores
	float *v_Mz;
	float *v_yawr;
	float *v_YR_d;
	float *v_sslip;
	float *par_FL;
	float *par_FR;
	float *par_RL;
	float *par_RR;
	float *v_StWheel;
	float *v_d_yawr;
	float *v_d_sslip;
	float *angvel_FL;
	float *angvel_FR;
	float *angvel_RL;
	float *angvel_RR;
	float *vx;
	float *vy;
	float *v_mod;

	//21Julio
	float *VgpsN;
	float *VgpsE;
	float *VgpsD;
	float *Euler0;
	float *Euler1;
	float *Euler2;
	float *v_WheelVol;

	int nmuestras=6000;
	int vcont=0;
	
	char nombre_archivo[100];
	struct timespec tiempo_actual;

	T_max_mot=40/Relacion_ParPedal;
	T_min_mot=-40/Relacion_ParPedal;

	/*Leemos la ganacia del controlador de un K50.txt*/
	fp = fopen("K15.txt", "r");

	for (j = 0; j < 10; j++) {
		for (i = 0; i < 50; i++) {
			fscanf(fp,"%f",&K[j][i]);
		}
	}

	fclose(fp);

	/*Leemos las matrices del modelo de Modelo.txt*/
	fp = fopen("Modelo15.txt", "r");

	for (j = 0; j < 50; j++) {
			for (i = 0; i < 6; i++) {
				fscanf(fp,"%f",&F[j][i]);
			}
		}
	for (j = 0; j < 6; j++) {
			for (i = 0; i < 6; i++) {
				fscanf(fp,"%f",&M[j][i]);
			}
		}
	for(i=0;i<6;i++){
			fscanf(fp,"%f",&N[i]);
		}

	fclose(fp);

	modelo=0; //Cargado el modelo de baja velocidad
	estado=0;


	//Creaci�n de vectores
	v_StWheel=calloc(nmuestras, sizeof(float));
	v_d_yawr=calloc(nmuestras, sizeof(float));
	v_d_sslip=calloc(nmuestras, sizeof(float));
	v_Mz=calloc(nmuestras, sizeof(float));
	v_yawr=calloc(nmuestras, sizeof(float));
	v_YR_d=calloc(nmuestras, sizeof(float));
	v_sslip=calloc(nmuestras, sizeof(float));
	angvel_FL=calloc(nmuestras, sizeof(float));
	angvel_FR=calloc(nmuestras, sizeof(float));
	angvel_RL=calloc(nmuestras, sizeof(float));
	angvel_RR=calloc(nmuestras, sizeof(float));
	par_FL=calloc(nmuestras, sizeof(float));
	par_FR=calloc(nmuestras, sizeof(float));
	par_RL=calloc(nmuestras, sizeof(float));
	par_RR=calloc(nmuestras, sizeof(float));

	vx=calloc(nmuestras, sizeof(float));
	vy=calloc(nmuestras, sizeof(float));
	v_mod=calloc(nmuestras, sizeof(float));
	
		//21Julio
	VgpsN = calloc(nmuestras, sizeof(float));
	VgpsE = calloc(nmuestras, sizeof(float));
	VgpsD = calloc(nmuestras, sizeof(float));
	Euler0 = calloc(nmuestras, sizeof(float));
	Euler1 =calloc(nmuestras, sizeof(float));
	Euler2 =calloc(nmuestras, sizeof(float));
	v_WheelVol = calloc(nmuestras, sizeof(float));


	/**************************************************
	***	CONFIGURACION Y LANZAMIENTO DEL TEMPORIZADOR ***
	**************************************************/

	// Usamos SIG_CTRL_EST_SINC como se�al en el temporizador:
	sigemptyset(&cjto_temp);
	sigaddset(&cjto_temp, SIG_CTRL_EST_SINC);
	// Programacion del periodo
	periodo_temp.tv_sec=0;
	periodo_temp.tv_nsec=TIEMPO_BUCLE_NS;
	prg_temp.it_value=periodo_temp;
	prg_temp.it_interval=periodo_temp;

	activar_temporizador_esp_sinc(&tempo, SIG_CTRL_EST_SINC,  &accion, &eventos_temp, prg_temp);


	/* Indica que el hilo del Control de estabilidad esta listo */
	pthread_mutex_lock(&mut_hilos_listos);
	hilos_listos |= HILO_ESTAB_LISTO;
	pthread_mutex_unlock(&mut_hilos_listos);
	printf("Hilo ESTAB listo\n");
	printf("Hilo ESTAB listo\n");
	pthread_mutex_lock(&mut_inicio);
	while(!cond_inicio)
	pthread_cond_wait(&inicio, &mut_inicio);
	pthread_mutex_unlock(&mut_inicio);
	printf("Hilo ESTAB listo\n");





	/* Algoritmo de control de estabilidad */

	while(!fin_local)
	{

		/* Volcado de las variables globales a locales */
		pthread_mutex_lock(&mut_vehiculo);
		acelera_local = vehiculo.acelerador;
		freno_local = vehiculo.freno;
		act_acel_local = vehiculo.act_acel;
		volante_local = vehiculo.volante;
		susp_ti=vehiculo.susp_ti;
		pthread_mutex_unlock(&mut_vehiculo);

		pthread_mutex_lock(&mDatosImu);
		memcpy(aceleracion, tDatosImu.pfAccelScal, sizeof(aceleracion));
		memcpy(velocidad_Ang, tDatosImu.pfGyroScal, sizeof(velocidad_Ang)); /*Variacien engulos_euler [x,y,z](rad/s) medido por el girescopo*/
		memcpy(ang_Euler, tDatosImu.pfEuler, sizeof(ang_Euler)); /* Angulo de Euler(rad) */
		memcpy(velocidad_GPS, tDatosImu.pfNEDVeloc, sizeof(velocidad_GPS)); /*Velocidad lineal medida por GPS (m/s)[Norte,Este,Modulo, Modulo proyectado*/
		pthread_mutex_unlock(&mDatosImu);

		//CAMBIO DE SIGNO POR POSICION DE LA IMU			
		aceleracion[1] = -aceleracion[1];
		aceleracion[2] = -aceleracion[2];
		velocidad_Ang[1]=-velocidad_Ang[1];
		velocidad_Ang[2]=-velocidad_Ang[2];
		
		
		for(i=9; i>0; i--){
			veloc_ang_WFL[i] = veloc_ang_WFL[i-1];
		}
		pthread_mutex_lock(&mut_motor1);
		veloc_ang_WFL[0] = motor1.rpm;		//Motor1=WFL
		pthread_mutex_unlock(&mut_motor1);
		veloc_ang_WFL[0] = RPM2RAD * veloc_ang_WFL[0];

		for(i=9; i>0; i--){
			veloc_ang_WRL[i] = veloc_ang_WRL[i-1];
		}
		pthread_mutex_lock(&mut_motor2);
		veloc_ang_WRL[0] = motor2.rpm;		//Motor2=WRL
		pthread_mutex_unlock(&mut_motor2);
		veloc_ang_WRL[0] = RPM2RAD * veloc_ang_WRL[0];

		for(i=9; i>0; i--){
			veloc_ang_WFR[i] = veloc_ang_WFR[i-1];
		}
		pthread_mutex_lock(&mut_motor3);
		veloc_ang_WFR[0] = motor3.rpm;		//Motor3=WFR
		pthread_mutex_unlock(&mut_motor3);
		veloc_ang_WFR[0] = RPM2RAD * veloc_ang_WFR[0];

		for(i=9; i>0; i--){
			veloc_ang_WRR[i] = veloc_ang_WRR[i-1];
		}
		pthread_mutex_lock(&mut_motor4);
		veloc_ang_WRR[0] = motor4.rpm;		//Motor4=WRR
		pthread_mutex_unlock(&mut_motor4);
		veloc_ang_WRR[0] = RPM2RAD * veloc_ang_WRR[0];

		/*MUTEX SUPERVISOR*/
		pthread_mutex_lock(&mut_supervisor);
		memcpy(motor_on_local, supervisor.motor_on, sizeof(motor_on_local));
		pthread_mutex_unlock(&mut_supervisor);

		/*Conversion unidades de aceleracion*/
		for (i = 0; i < 3; i++) {
			aceleracion[i] = GRAV * aceleracion[i];
		}
	

		//Calibracien del volante
		if (volante_local < vol_min) {
			vol_min=volante_local;
		}
		if (volante_local > vol_max) {
			vol_max=volante_local;
		}

		/*CALCULO DE DIRECCION*/
		Steering_angle = 0.9280*volante_local - 3.8254; //Entrada de volante en tnsion, salida de engulo en radianes
		Delta_WFL= -0.2086*pow(volante_local,4) +   3.6383*pow(volante_local,3) + -23.4499*pow(volante_local,2) +   67.1542*volante_local -72.9841;
		Delta_WFR= -0.7504*pow(volante_local,4) + 12.5845*pow(volante_local,3) + -79.0022*pow(volante_local,2) + 220.9029*volante_local - 232.9891;

		/*Conversien de valores de velocidad*/
		CambioSR_Veloc(velocidad_GPS, ang_Euler, velocidad_COG);
		velocidad_COG[1]=-velocidad_COG[1];
		velocidad_COG[2]=-velocidad_COG[2];
		Beta = atan2(velocidad_COG[1], velocidad_COG[0]); //velocidad[0](Vx)= velocidad[3](Modulo)*cos(Beta)

		/*CALCULO VELOCIDAD ANGULAR RUEDAS*/
		/*Calculo de velocidad en Rueda 2 - EJES COCHE - Frontal Izquierda*/
		veloc_WFL_src[0] = velocidad_COG[3]*cos(Beta) - velocidad_Ang[2]*DIST_F_LEFT;
		veloc_WFL_src[1] = velocidad_COG[3]*sin(Beta) + velocidad_Ang[2]*DIST_F_LX;
		/*Conversion a EJES RUEDA*/
		veloc_WFL[0] =  cos(Delta_WFL)*veloc_WFL_src[0] +  sin(Delta_WFL)*veloc_WFL_src[1];
		veloc_WFL[1] = -sin(Delta_WFL)*veloc_WFL_src[0] +  cos(Delta_WFL)*veloc_WFL_src[1];
		//La 1a componente del vector, se desplaza a la segunda, el nuevo valor ocupa la posicion 0//
		for(i=9; i>0; i--){
			veloc_mod_WFL[i] = veloc_mod_WFL[i-1];
		}
		veloc_mod_WFL[0] = sqrt(pow(veloc_WFL[0], 2) + pow(veloc_WFL[1], 2));

		/*Calculo de velocidad en Rueda 4 - EJES COCHE - Frontal Derecha*/
		veloc_WFR_src[0] = velocidad_COG[3]*cos(Beta) + velocidad_Ang[2]*DIST_F_RIGHT;
		veloc_WFR_src[1] = velocidad_COG[3]*sin(Beta) + velocidad_Ang[2]*DIST_F_LX;
		/*Conversion a EJES RUEDA*/
		veloc_WFR[0] =  cos(Delta_WFR)*veloc_WFR_src[0] +  sin(Delta_WFR)*veloc_WFR_src[1];
		veloc_WFR[1] = -sin(Delta_WFR)*veloc_WFR_src[0] +  cos(Delta_WFR)*veloc_WFR_src[1];
		for(i=9; i>0; i--){
			veloc_mod_WFR[i] = veloc_mod_WFR[i-1];
		}
		veloc_mod_WFR[0] = sqrt(pow(veloc_WFR[0], 2) + pow(veloc_WFR[1], 2));

		/*Calculo de velocidad en Rueda 1 - EJES COCHE=EJES RUEDA - Trasera Izquierda*/
		veloc_WRL[0] = velocidad_COG[3]*cos(Beta) - velocidad_Ang[2]*DIST_R_LEFT;
		veloc_WRL[1] = velocidad_COG[3]*sin(Beta) - velocidad_Ang[2]*DIST_R_LX;
		for(i=9; i>0; i--){
			veloc_mod_WRL[i] = veloc_mod_WRL[i-1];
		}
		veloc_mod_WRL[0] = sqrt(pow(veloc_WRL[0], 2) + pow(veloc_WRL[1], 2));

		/*Calculo de velocidad en Rueda 3 - EJES COCHE = EJES RUEDA - Trasera Derecha*/
		veloc_WRR[0] = velocidad_COG[3]*cos(Beta) + velocidad_Ang[2]*DIST_R_RIGHT;
		veloc_WRR[1] = velocidad_COG[3]*sin(Beta) - velocidad_Ang[2]*DIST_R_LX;
		for(i=9; i>0; i--){
			veloc_mod_WRR[i] = veloc_mod_WRR[i-1];
		}
		veloc_mod_WRR[0] = sqrt(pow(veloc_WRR[0], 2) + pow(veloc_WRR[1], 2));

		/*CALCULO DE LAS DERIVADAS EN LAS RUEDAS*/
		acel_est_FL=(veloc_mod_WFL[0]-veloc_mod_WFL[9])/(10*TIEMPO_MUESTREO); //Aceleracion estimada
		acel_est_FR=(veloc_mod_WFR[0]-veloc_mod_WFR[9])/(10*TIEMPO_MUESTREO);
		acel_est_RL=(veloc_mod_WRL[0]-veloc_mod_WRL[9])/(10*TIEMPO_MUESTREO);
		acel_est_RR=(veloc_mod_WRR[0]-veloc_mod_WRR[9])/(10*TIEMPO_MUESTREO);

		acel_ang_est_FL = (veloc_ang_WFL[0]-veloc_ang_WFL[9])/(10*TIEMPO_MUESTREO); //M1
		acel_ang_est_RL = (veloc_ang_WRL[0]-veloc_ang_WRL[9])/(10*TIEMPO_MUESTREO); //M2
		acel_ang_est_FR = (veloc_ang_WFR[0]-veloc_ang_WFR[9])/(10*TIEMPO_MUESTREO); //M3
		acel_ang_est_RR = (veloc_ang_WFR[0]-veloc_ang_WRR[9])/(10*TIEMPO_MUESTREO); //M4

		

		/*REPARTO DE PAR (IGUAL AL PEDAL E IGUAL PARA CADA RUEDA)*/
		for (i = POS_M1; i < (POS_M4 + 1); i++) {
			trac_acelera_local[i] = acelera_local;
		}

		/*CALCULO MOMENTO ANGULAR*/

//		//Actualizacion modelos segun velocidad
//		if(estado==0){
//			if(velocidad_COG[3]>9.72){
//				estado=1; //Estado alta velocidad
//			}
//		}
//		if(estado==1){
//			if(velocidad_COG[3]<6.94){
//				estado=0; //Estado baja velocidad
//			}
//		}
//
//		//Comprobaci�n modelo cargado y modificaci�n
//		if(estado==0 && modelo==1){
//			/*Leemos la ganacia del controlador de un K50.txt*/
//			fp = fopen("K15.txt", "r");
//
//			for (j = 0; j < 10; j++) {
//				for (i = 0; i < 50; i++) {
//					fscanf(fp,"%f",&K[j][i]);
//				}
//			}
//
//			fclose(fp);
//
//			/*Leemos las matrices del modelo de Modelo.txt*/
//			fp = fopen("Modelo15.txt", "r");
//
//			for (j = 0; j < 50; j++) {
//				for (i = 0; i < 6; i++) {
//					fscanf(fp,"%f",&F[j][i]);
//				}
//			}
//			for (j = 0; j < 6; j++) {
//				for (i = 0; i < 6; i++) {
//					fscanf(fp,"%f",&M[j][i]);
//				}
//			}
//			for(i=0;i<6;i++){
//				fscanf(fp,"%f",&N[i]);
//			}
//
//			fclose(fp);

//			Modo funcion--> carga_modelo(estado, K, F, M, N)
//		}
//		if(estado==1 && modelo==0){
//			/*Leemos la ganacia del controlador de un K50.txt*/
//			fp = fopen("K50.txt", "r");
//
//			for (j = 0; j < 10; j++) {
//				for (i = 0; i < 50; i++) {
//					fscanf(fp,"%f",&K[j][i]);
//				}
//			}
//
//			fclose(fp);
//
//			/*Leemos las matrices del modelo de Modelo.txt*/
//			fp = fopen("Modelo50.txt", "r");
//
//			for (j = 0; j < 50; j++) {
//				for (i = 0; i < 6; i++) {
//					fscanf(fp,"%f",&F[j][i]);
//				}
//			}
//			for (j = 0; j < 6; j++) {
//				for (i = 0; i < 6; i++) {
//					fscanf(fp,"%f",&M[j][i]);
//				}
//			}
//			for(i=0;i<6;i++){
//				fscanf(fp,"%f",&N[i]);
//			}
//
//			fclose(fp);

//			Modo funcion--> carga_modelo(estado, K, F, M, N)
//		}

		//Calculo de derivadas y actualizacon de variables
		for(i=9; i>0; i--){
			sslip[i] = sslip[i-1];
		}
		sslip[0]=Beta;	//Actualizamos el nuevo valor

		for(i=9; i>0; i--){
			yaw_r[i] = yaw_r[i-1];
		}
		yaw_r[0]=velocidad_Ang[2]; //Cambio de signo de la velocidad angular

		/*Calculo derivadas variables de control*/
		d_sslip=(sslip[0]-sslip[9]) / (10*TIEMPO_MUESTREO); //Derivada del angulo de deriva lateral
		d_yaw_r=(yaw_r[0]-yaw_r[9]) / (10*TIEMPO_MUESTREO);

		//Actualizacion vector de estados
		Xext[0]=sslip[0];
		Xext[1]=d_sslip;
		Xext[2]=yaw_r[0];
		Xext[3]=d_yaw_r;
		Xext[4]=Steering_angle;
		Xext[5]=Xext2[5];

		//Calculo de la velocidad angular deseada
		YR_d= (velocidad_COG[3]*Steering_angle)/(DIST_EJES_LX +((MASS_FOX/DIST_EJES_LX)*((DIST_R_LX/CF)-(DIST_F_LX/CR))*(pow(velocidad_COG[3],2))));


		//Actualizacion de la referencia
		for(i=0; i<NP; i++){
			w[i*2]=0;
			w[i*2+1]=YR_d;
		}

		//Calculo del siguiente estado
		for(i=0;i<6;i++){
			Xext2[i]=N[i]*deltaU;
			for(k=0;k<6;k++){
				Xext2[i]=(Xext2[i]+(M[i][k]*Xext[k]));
			}
		}

		//Calculo de la salida estimada
		for(i=0;i<NP*2;i++){
			f[i]=0;
			for(k=0;k<6;k++){
				f[i]=(f[i]+(F[i][k]*Xext2[k]));
			}
		}

		//Error cometido entre la salida estimada y la referencia
		for(i=0; i<NP*2; i++){
			ef[i]=w[i]-f[i];
		}

		//Calculo del vector de actuaciones incrementales
		for(i=0;i<NU;i++){
			u[i]=0;
			for(k=0;k<NP*2;k++){
				u[i]=(u[i]+(K[i][k]*ef[k]));
			}
		}

		//Seleccionamos unicamente el primer elemento de la actuacion incremental
		deltaU=u[0];

		//Actulizamos la actuacion total al sistema
		Momento_necesario=Momento_necesario+deltaU;

		//Limitacion momento correctivo
		if (Momento_necesario > 1000){
			Momento_necesario = 1000;
		} else if (Momento_necesario < -1000){
			Momento_necesario = -1000;
		}

		/*Limitador de actuaci�n*/

		error_yr=fabs(yaw_r[0]-YR_d); //Seg�n el error de la velocidad angular

		if(error_yr<0.006){
			Momento_necesario=0;
		}

//		error_min=0.00075*vx-0.005;
//
//		if(velocidad_COG[3]>13.88){
//		    error_min=2*error_min;
//		}
//		if(velocidad_COG[3]<9.72){
//		    error_min=0.002;
//		}
//
//		if(error_yr<1.5*error_min){
//		    por_act=(1/(0.5*error_min))*error_yr-2;
//		    if(por_act>1){
//		        por_act=1;
//		    }
//		    if(por_act<0){
//		        por_act=0;
//		    }
//		    Momento_necesario=por_act*Momento_necesario;
//		}
//		else{
//			por_act=1;
//		}

		if(velocidad_COG[3]<1.4){ //Umbral minimo de accion con respecto a velocidad (5 km/h)
			Momento_necesario=0;
		}

		/*Reparto de par delantero trasero*/
		difr=0.5;
		diff=0.5;

		Mzf=Momento_necesario*diff;
		Mzr=Momento_necesario*difr;

		//Calculo par diferencial
		Par[1]=Mzf*(DIAMETRO_WF/2*DIST_F_LY)+J_WF*acel_ang_est_FL; //Rueda FL
		Par[3]=Mzf*(DIAMETRO_WF/2*DIST_F_LY)+J_WF*acel_ang_est_FR; //Rueda FR

		Par[0]=Mzr*(DIAMETRO_WR/2*DIST_R_LY)+J_WR*acel_ang_est_RL; //Rueda RL
		Par[2]=Mzr*(DIAMETRO_WR/2*DIST_R_LY)+J_WR*acel_ang_est_RR; //Rueda RR


		/*CARLCULO DEL MTTE*/
		/*Calculo de la fuerza de deslizamiento
		Siendo trac_acelera_local[0] la tension correspondiente al par aplicado en la rueda FL;
		Siendo trac_acelera_local[1] la tension correspondiente al par aplicado en la rueda RL;
		Siendo trac_acelera_local[2] la tension correspondiente al  par aplicado en la rueda FR;
		Siendo trac_acelera_local[3] la tension correspondiente al  par aplicado en la rueda FL;*/
		Ffricc_FL = (1/(DIAMETRO_WF/2))*(trac_acelera_local[0]*Relacion_ParPedal - (J_WF*acel_est_FL)/(DIAMETRO_WF/2)); //Fuerza de Friccion en la rueda Frontal Izquierda (M1)
		Ffricc_RL = (1/(DIAMETRO_WR/2))*(trac_acelera_local[1]*Relacion_ParPedal - (J_WR*acel_est_RL)/(DIAMETRO_WR/2)); //Fuerza de Friccion en la rueda Trasera Izquierda (M2)
		Ffricc_FR = (1/(DIAMETRO_WF/2))*(trac_acelera_local[2]*Relacion_ParPedal - (J_WF*acel_est_FR)/(DIAMETRO_WF/2)); //Fuerza de Friccion en la rueda Frontal Derecha   (M3)
		Ffricc_RR = (1/(DIAMETRO_WR/2))*(trac_acelera_local[3]*Relacion_ParPedal - (J_WR*acel_est_RR)/(DIAMETRO_WR/2)); //Fuerza de Friccion en la rueda Trasera Derecha   (M4)

		/*Calculo de la variable auxiliar*/
		alpha_FL = acel_est_FL/((DIAMETRO_WR/2)*acel_ang_est_FL);     //Aceleracion long Vehiculo/ Aceleracion long Rueda
		alpha_RL = acel_est_RL/((DIAMETRO_WR/2)*acel_ang_est_RL);
		alpha_FR = acel_est_FR/((DIAMETRO_WR/2)*acel_ang_est_FR);
		alpha_RR = acel_est_RR/((DIAMETRO_WR/2)*acel_ang_est_RR);


		/*Calculo del Par Maximo aplicable*/
		T_max_FL = (DIAMETRO_WF/2)*Ffricc_FL*  (J_WF/(alpha_FL  * MASS_FOX*pow(DIAMETRO_WF/2,2))  +1);
		T_max_RL = (DIAMETRO_WR/2)*Ffricc_RL* (J_WR/(alpha_RL  * MASS_FOX*pow(DIAMETRO_WR/2,2)) +1);
		T_max_FR = (DIAMETRO_WF/2)*Ffricc_FR*  (J_WF/(alpha_FR * MASS_FOX*pow(DIAMETRO_WF/2, 2)) +1);
		T_max_RR = (DIAMETRO_WR/2)*Ffricc_RR* (J_WR/(alpha_RR * MASS_FOX*pow(DIAMETRO_WR/2,2))  +1);

		T_max[0] = T_max_RL/Relacion_ParPedal;  //Nivel de V correspondiente a par max
		T_max[1] = T_max_FL/Relacion_ParPedal;
		T_max[2] = T_max_RR/Relacion_ParPedal;
		T_max[3] = T_max_FR/Relacion_ParPedal;


		/*Limitacion del Par maximo que pueden dar los motores*/
		for(i=POS_M1; i<(POS_M4+1); i++)
		{
			if(T_max[i]>T_max_mot)
				T_max[i]=T_max_mot;
			else if(T_max[i]<T_min_mot)
				T_max[i]=T_min_mot;
		}

		//Aplicacien del par diferencial
		for(i=POS_M1; i<(POS_M4+1); i++)
		{
			//trac_acelera_local[i]=trac_acelera_local[i]+(Par[i]/Relacion_ParPedal);
			trac_acelera_local[i]=acelera_local;
		}


		//Aplicacien del par a la variable local y MTTE
		for(i=POS_M1; i<(POS_M4+1); i++)
		{
			if (freno_local > 0.5)
			{
				act_freno_local[i] = TRUE;
				trac_freno_local[i] = freno_local;
			}
			else
			{	act_freno_local[i] = FALSE;
				trac_freno_local[i] = freno_local;
			}


			if (motor_on_local[i] == TRUE)
			{
				if ( trac_acelera_local[i] > T_max[i]   &&  T_max[i]>0 ) //Si supera el limite superior, acoto a T_max
					trac_acelera_local[i] = T_max[i];
				else if(trac_acelera_local[i] < -T_max[i]  &&  -T_max[i]< 0 ) //Si supera el limite inferior, acoto a -T_max
					trac_acelera_local[i] = -T_max[i];
			}
			else{
				trac_acelera_local[i] = 0;
			}
		}

		if (ii==20)
		{
//			printf("\nSteering_Wheel, Beta, velocidad_COG[0], velocdad_COG[1], velocidad_COG[2], veloc_WFR[0], acel_est_FR, acel_ang_est_FR, velocidad_Ang, susp_ti\n");
//			printf("%.12f, %.12f, %.12f, %.12f, %.12f, %.12f, %.12f, %.12f, %.12f, %.12f\n",Steering_angle, Beta, velocidad_COG[0], velocidad_COG[1], velocidad_COG[2], veloc_WFR[0], acel_est_FR, acel_ang_est_FR, velocidad_Ang[2],susp_ti);
//			printf("\n velocidad_Ang[0] velocidad_Ang[1]  velocidad_Ang[2] susp_ti\n");
//			printf("%.12f, %.12f, %.12f, %.12f \n",velocidad_Ang[0], velocidad_Ang[1],  velocidad_Ang[2], susp_ti);
/*			printf("\e[1;1H\e[2J"); //Limpia la pantalla
			printf("Steering_wheel=%.12f\n", Steering_angle);
			printf("Aceleraciones Angulares \n");
			printf("acel_ang_est_FL=%.12f | acel_ang_est_FR=%.12f\n", acel_ang_est_FL, acel_ang_est_FR);
			printf("acel_ang_est_RL=%.12f | acel_ang_est_RR=%.12f\n", acel_ang_est_RL, acel_ang_est_RR);
			printf("Pares Conductor \n");
			printf("ParFL=%.12f | ParFR=%.12f\n", Par[1],Par[3]);
			printf("ParRL=%.12f | ParRR=%.12f\n", Par[0],Par[2]);
			printf("ParesMPC \n");
			printf("ParFL=%.12f | ParFR=%.12f\n",trac_acelera_local[1],trac_acelera_local[3]);
			printf("ParRL=%.12f | ParRR=%.12f\n",trac_acelera_local[0],trac_acelera_local[2]);
			printf("Momento necesario=%.12f\n", Momento_necesario);
			printf("velocidad_COG[X]=%.12f\n", velocidad_COG[0]);
			printf("velocidad_COG[Y]=%.12f\n", velocidad_COG[1]);
			printf("velocidad_COG[MOD]=%.12f\n", velocidad_COG[3]);
*/			/*Codigo para borrar pantalla*/

			ii=0;
		}
		ii++;

		/*Volcado de datos en vectores*/
		if(vcont<nmuestras){
			v_Mz[vcont]=Momento_necesario;
			v_yawr[vcont]=yaw_r[0];
			v_YR_d[vcont]=YR_d;
			v_sslip[vcont]=sslip[0];
			par_FL[vcont]=Par[1];
			par_FR[vcont]=Par[3];
			par_RL[vcont]=Par[0];
			par_RR[vcont]=Par[2];
			v_StWheel[vcont]=Steering_angle;
			v_d_sslip[vcont]=d_sslip;
			v_d_yawr[vcont]=d_yaw_r;
			angvel_FL[vcont]=veloc_ang_WFL[0];
			angvel_FR[vcont]=veloc_ang_WFR[0];
			angvel_RL[vcont]=veloc_ang_WRL[0];
			angvel_RR[vcont]=veloc_ang_WRR[0];
			vx[vcont] = velocidad_COG[0];
			vy[vcont] = velocidad_COG[1];
			v_mod[vcont] = velocidad_COG[3]; 
			
			//21Julio
			VgpsN[vcont] = velocidad_GPS[0];
			VgpsE[vcont] = velocidad_GPS[1];
			VgpsD[vcont] = velocidad_GPS[2];
			Euler0[vcont] = ang_Euler[0];
			Euler1[vcont] =ang_Euler[1];
			Euler2[vcont] =ang_Euler[2];
			v_WheelVol[vcont] = volante_local;

		}
		else{ //Cuando el buffer este lleno
			for(i=POS_M1; i<(POS_M4+1); i++)
			{
				trac_acelera_local[i]=0; //Detenemos la actuaci�n de los motores
			}
			//Guardamos los datos en un fichero
			if(vcont==6000){
			
			clock_gettime(CLOCK_REALTIME,&tiempo_actual);
			sprintf(nombre_archivo,"/datos/log_ecu/Datos_%d.txt",(int)tiempo_actual.tv_sec);
			
			fp = fopen ( nombre_archivo,"w");

			//			fprintf(fp,"v_Mz=\n");
			for (i = 0; i < nmuestras; i++){
				fprintf(fp,"%.12f ",v_Mz[i]);
			}
			fprintf(fp,"\n");

			for (i = 0; i < nmuestras; i++){
				fprintf(fp,"%.12f ",v_StWheel[i]);
			}
			fprintf(fp,"\n");

			//			fprintf(fp,"v_yawr=\n");
			for (i = 0; i < nmuestras; i++){
				fprintf(fp,"%.12f ",v_yawr[i]);
			}
			fprintf(fp,"\n");
			//			fprintf(fp,"\n\n");
			for (i = 0; i < nmuestras; i++){
				fprintf(fp,"%.12f ",v_d_yawr[i]);
			}
			fprintf(fp,"\n");

			//			fprintf(fp,"v_YR_d=\n");
			for (i = 0; i < nmuestras; i++){
				fprintf(fp,"%.12f ",v_YR_d[i]);
			}
			fprintf(fp,"\n");
			//			fprintf(fp,"\n\n");

			//			fprintf(fp,"v_sslip=\n");
			for (i = 0; i < nmuestras; i++){
				fprintf(fp,"%.12f ",v_sslip[i]);
			}
			fprintf(fp,"\n");
			//			fprintf(fp,"\n\n");
			for (i = 0; i < nmuestras; i++){
				fprintf(fp,"%.12f ",v_d_sslip[i]);
			}
			fprintf(fp,"\n");

			//			fprintf(fp,"par_FL=\n");
			for (i = 0; i < nmuestras; i++){
				fprintf(fp,"%.12f ",par_FL[i]);
			}
			fprintf(fp,"\n");
			//			fprintf(fp,"\n\n");

			//			fprintf(fp,"par_FR=\n");
			for (i = 0; i < nmuestras; i++){
				fprintf(fp,"%.12f ",par_FR[i]);
			}
			fprintf(fp,"\n");
			//			fprintf(fp,"\n\n");

			//			fprintf(fp,"par_RL=\n");
			for (i = 0; i < nmuestras; i++){
				fprintf(fp,"%.12f ",par_RL[i]);
			}
			fprintf(fp,"\n");
			//			fprintf(fp,"\n\n");

			//			fprintf(fp,"par_RR=\n");
			for (i = 0; i < nmuestras; i++){
				fprintf(fp,"%.12f ",par_RR[i]);
			}
			fprintf(fp,"\n");
			//			fprintf(fp,"\n\n");

			//Velocidades de las ruedas: FL, FR, RL, RR
			for (i = 0; i < nmuestras; i++){
				fprintf(fp,"%.12f ",angvel_FL[i]);
			}
			fprintf(fp,"\n");

			for (i = 0; i < nmuestras; i++){
				fprintf(fp,"%.12f ",angvel_FR[i]);
			}
			fprintf(fp,"\n");

			for (i = 0; i < nmuestras; i++){
				fprintf(fp,"%.12f ",angvel_RL[i]);
			}
			fprintf(fp,"\n");

			for (i = 0; i < nmuestras; i++){
				fprintf(fp,"%.12f ",angvel_RR[i]);
			}
			fprintf(fp,"\n");

			for (i = 0; i < nmuestras; i++){
				fprintf(fp,"%.12f ",vx[i]);
			}
			fprintf(fp,"\n");

			for (i = 0; i < nmuestras; i++){
				fprintf(fp,"%.12f ",vy[i]);
			}
			fprintf(fp,"\n");
			for (i = 0; i < nmuestras; i++){
				fprintf(fp,"%.12f ",v_mod[i]);
			}
			fprintf(fp,"\n");
			//21Julio
			for (i = 0; i < nmuestras; i++){
				fprintf(fp,"%.8f ",VgpsN[i]);
			}
			fprintf(fp,"\n");

			for (i = 0; i < nmuestras; i++){
				fprintf(fp,"%.8f ",VgpsE[i]);
			}
			fprintf(fp,"\n");

			for (i = 0; i < nmuestras; i++){
				fprintf(fp,"%.8f ",VgpsD[i]);
			}
			fprintf(fp,"\n");

			for (i = 0; i < nmuestras; i++){
				fprintf(fp,"%.8f ",Euler0[i]);
			}
			fprintf(fp,"\n");

			for (i = 0; i < nmuestras; i++){
				fprintf(fp,"%.8f ",Euler1[i]);
			}
			fprintf(fp,"\n");

			for (i = 0; i < nmuestras; i++){
				fprintf(fp,"%.8f ",Euler2[i]);
			}
			fprintf(fp,"\n");

			for (i = 0; i < nmuestras; i++) {
				fprintf(fp, "%.8f ",v_WheelVol[i]);
			}
			fprintf(fp, "\n");
			fclose(fp);
			}
		}
		vcont++;

		/* Actualizacion de variable global
		pthread_mutex_lock(&mut_potencias);
		memcpy(potencias.act_freno, act_freno_local, sizeof(act_freno_local));
		memcpy(potencias.acelerador, trac_acelera_local, sizeof(trac_acelera_local));
		memcpy(potencias.freno, trac_freno_local, sizeof(trac_freno_local));
		pthread_mutex_unlock(&mut_potencias);*/

		//Espera al temporizador
		sigwaitinfo(&cjto_temp, NULL);

//		nanosleep (&espera, NULL);

		pthread_mutex_lock(&mut_hilos_listos);
		if ((hilos_listos&HILO_ESTAB_LISTO)==0x0000)
			fin_local = 1;
		pthread_mutex_unlock(&mut_hilos_listos);
	} // Fin bucle while

	timer_delete(tempo);
	pthread_exit(NULL);
	return(NULL);
}

void  CambioSR_Veloc(float *Vgps, float *Ang,float *velocidad_COG)
{

//Vgps[0]:Componente Norte de la Velocidad (pfNEDVeloc[0])
//Vgps[1]:Componente Este  de la Velocidad (pfNEDVeloc[1])
//Vgps[2]:Componente Abajo de la Velocidad (pfNEDVeloc[2])
//Vgps[4]:Modulo de la proyeccion en tierra(pfNEDVeloc[4])
//Ang[0] :Valor de Roll/Balanceo (Giro en X) Phi
//Ang[1] :Valor de Pitch/Cabeceo (Giro en Y) Theta
//Ang[2] :Valor de Yaw/Guinhada  (Giro en Z) Psi
	/*MatrizCambio={{,,},{,,},{,,}};
	for (nf=0;nf<3;nf++){
		velocidad_COG[nf]=0;
		for(nc=0;nc<3;nc++){
			velocidad_COG[nf]=velocidad_COG[nf]+ MatrizCambio[nf][nc]*Vgps[nc];
		}
	}*/
	velocidad_COG[0]=  Vgps[0]*(cos(Ang[2])*cos(Ang[1]))									               + Vgps[1]*(sin(Ang[2])*cos(Ang[1]))											        - Vgps[2]*sin(Ang[1]);					//Componente longitudinal de velocidad
	velocidad_COG[1]=  Vgps[0]*(cos(Ang[2])*sin(Ang[1])*sin(Ang[0]) - sin(Ang[2])*cos(Ang[0]))             + Vgps[1]*(cos(Ang[2])*cos(Ang[0])			 + sin(Ang[2])*sin(Ang[1])*sin(Ang[0]))	+ Vgps[2]*cos(Ang[1])*sin(Ang[0]);		//Componente transversal de velocidad
	velocidad_COG[2]=  Vgps[0]*(sin(Ang[2])*sin(Ang[0])             + cos(Ang[2])*cos(Ang[0])*sin(Ang[1])) + Vgps[1]*(sin(Ang[2])*sin(Ang[1])*cos(Ang[1])-cos(Ang[2])*sin(Ang[0]))		        + Vgps[2]*cos(Ang[1])*cos(Ang[0]);      //Componente vertical de velocidad
	velocidad_COG[3]=  Vgps[4];																				   //Modulo de velocidad en XY
}



void  Actualiza_Var(float *Var)
{ //Actualizacion del buffer de variables recordadas
	//TAMANHOVAR=length(Var)??
	for(i=TAMANHOVAR;i--;i>0){
		Var[i]=Var[i-1];
	}
}
	
