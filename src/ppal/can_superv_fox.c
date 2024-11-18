/**************************************************************************
***									***
***	Fichero: can_superv_fox.c					***
***	Fecha: 01/04/2013						***
***	Autor: Elena Gonzalez						***
***	Descripción: Hilo de adquisicion de datos por CAN para motores.	***
***	Recogida de datos CAN de los motores.				***
***									***
**************************************************************************/

/**************************************************
***		CABECERAS			***
**************************************************/
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
#include <pthread.h>
/* Can includes */
#include "./include/candef.h"
#include "./include/canstr.h"
#include "./include/canglob.h"
/* Include local */
#include "./include/constantes_fox.h"
#include "./include/estructuras_fox.h"
#include "./include/declaraciones_fox.h"

/**************************************************
***		  VARIABLES GLOBALES		***
**************************************************/
/* Variables globales */
extern uint16_t hilos_listos;
extern int cont_buff_rx;	// Contador de mensajes en el buffer de recepcion del supervisor

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

/* Buffers CAN */
extern struct can_object *buff_rx_superv;		// Buffer de recepcion de mensajes procedientes del supervisor
extern struct can_object buff_rx_mot1;		// Buffer de recepcion de mensajes procedientes del motor 1
extern struct can_object buff_rx_mot2;		// Buffer de recepcion de mensajes procedientes del motor 2
extern struct can_object buff_rx_mot3;		// Buffer de recepcion de mensajes procedientes del motor 3
extern struct can_object buff_rx_mot4;		// Buffer de recepcion de mensajes procedientes del motor 4

/* Identificador de la conexion CAN */
extern canhdl_t hdl;

/* Mutex */
extern pthread_mutex_t mDatosImu;
extern pthread_mutex_t mut_supervisor;
extern pthread_mutex_t mut_vehiculo;
extern pthread_mutex_t mut_bateria;
extern pthread_mutex_t mut_motor1;
extern pthread_mutex_t mut_motor2;
extern pthread_mutex_t mut_motor3;
extern pthread_mutex_t mut_motor4;
extern pthread_mutex_t mut_errores;
extern pthread_mutex_t mut_potencias;
extern pthread_mutex_t mut_hdl;
extern pthread_mutex_t mut_hilos_listos;
extern pthread_mutex_t mut_inicio;
extern pthread_cond_t inicio;
extern pthread_mutex_t mut_rx_superv;

/******************************************************************************************
***			FUNCION PRINCIPAL: can_superv_main (hilo)			***
******************************************************************************************/
void *can_superv(void *pi)
{
	/* Variables */
	int i;
	BOOL fin_local = 0;
	struct can_object superv_rx_msg;
	struct can_object *buff_rx_superv_local;
	int cont_buff_rx_local = 0;
	
	/* Variables para el temporizador */
	timer_t temporizador;
	struct sigaction accion;
	struct sigevent eventos;
	struct itimerspec programacion;
	struct timespec periodo;	
	sigset_t segnales;
	
	/**************************************************
	***	CONFIGURACION E INICIO TEMPORIZADOR	***
	**************************************************/
	/* Programacion del ciclo del temporizador */
	periodo.tv_sec = 0;				
	periodo.tv_nsec = TIEMPO_CAN_SUP_NS;
	programacion.it_value = periodo;
	programacion.it_interval = periodo;
	/* Vinculacion de SIGRTMIN con el temporizador */
	eventos.sigev_signo = SIG_CAN_SUP;
	eventos.sigev_notify = SIGEV_SIGNAL;
	eventos.sigev_value.sival_ptr = (void*) &temporizador;
	/* Accion a realizar cuando se reciba la segnal del temporizador */
	accion.sa_sigaction = manejador_envia_msgCAN;	// EN ESTE MANEJADOR SE METE LO QUE SE ENVIA AL SUPERVISOR
	accion.sa_flags = SA_SIGINFO;
	sigemptyset(&accion.sa_mask);
	sigaction(SIG_CAN_SUP, &accion, NULL);
	/* Mascara del proceso */
	sigemptyset(&segnales);
	sigaddset(&segnales, SIG_CAN_SUP);
	pthread_sigmask(SIG_UNBLOCK, &segnales, NULL);
	
	/* Reservamos memoria para el buffer local de mensajes CAN */
	buff_rx_superv_local = (struct can_object *) malloc(TAM_BUFF_RX * sizeof(struct can_object));
	
	/* Activacion del temporizador */
	timer_create(CLOCK_REALTIME, &eventos, &temporizador);
	timer_settime(temporizador, 0, &programacion, NULL);
	
	/* Indica que el hilo CAN del supervisor esta listo */
	pthread_mutex_lock(&mut_hilos_listos);
	hilos_listos |= HILO_SUPERV_LISTO;
	pthread_mutex_unlock(&mut_hilos_listos);
	printf("Hilo SUPERV listo\n");
	pthread_mutex_lock(&mut_inicio);
	pthread_cond_wait(&inicio, &mut_inicio);
	pthread_mutex_unlock(&mut_inicio);
	
	while (!fin_local)
	{
		/* Cargamos el indice del buffer y el contenido del buffer en las variables locales */
		pthread_mutex_lock(&mut_rx_superv);
		cont_buff_rx_local = cont_buff_rx;
		cont_buff_rx = 0;
		memcpy(buff_rx_superv_local, buff_rx_superv, cont_buff_rx_local * sizeof(struct can_object));
		pthread_mutex_unlock(&mut_rx_superv);
		/* Leemos los mensajes del buffer uno a uno hasta vaciarlo */
		for (i=cont_buff_rx_local; i>0; i--)
		{
			superv_rx_msg = buff_rx_superv_local[cont_buff_rx_local - i];
			/* Interpretamos el mensaje */
			can_in_superv(superv_rx_msg);
		}
		
		//El bucle espera a que llegue interrupcion del temporizador
		nanosleep(&periodo,NULL);
		
		pthread_mutex_lock(&mut_hilos_listos);
		if ((hilos_listos&HILO_SUPERV_LISTO) == 0x0000)
			fin_local = 1;
		pthread_mutex_unlock(&mut_hilos_listos);	

	} // Fin bucle while

	timer_delete(temporizador);
	/* Liberamos memoria del buffer local de mensajes CAN */
	free (buff_rx_superv_local);
	pthread_exit(NULL);
	return NULL;
}

//------------->Modificar esto
void can_in_superv(struct can_object rxmsg)
{
	switch (rxmsg.id)
	{
	//NOTA: incluir los ID en canRX_fox.c
		case ID_SUPERV_HEARTBEAT:
			pthread_mutex_lock(&mut_supervisor);
			supervisor.heartbeat = (BOOL) rxmsg.data[0];
			supervisor.motor_on[POS_M1] = (BOOL) rxmsg.data[1];
			supervisor.motor_on[POS_M2] = (BOOL) rxmsg.data[2];
			supervisor.motor_on[POS_M3] = (BOOL) rxmsg.data[3];
			supervisor.motor_on[POS_M4] = (BOOL) rxmsg.data[4];
			supervisor.on = TRUE;
			pthread_mutex_unlock(&mut_supervisor);
		break;
		case ID_SUPERV_OFF:
		break;
		default:
//			perror("Mensaje desconocido.\n");
	}
}


/**************************************************************************
***		DEFINICION FUNCION: manejador_envia_msgCAN		***
*** Manejador para envio de mensajes CAN al supervisor.			***
*** Parametros:								***
***	- int signo:							***
***	- siginfo_t *info:						***
***	- void *nada:							***
**************************************************************************/
void manejador_envia_msgCAN(int signo, siginfo_t *info, void *nada)
{
	/**************************************************
	***		VARIABLES			***
	**************************************************/
	/* Estructura para mensaje CAN a enviar */
	struct can_object msg_can_out;		
	/* Variables auxiliares */
	BOOL heartbeat_superv_local = FALSE;
	BYTE uc_aux1 = 0x00;
	BYTE uc_aux2 = 0x00;
	BYTE uc_aux3 = 0x00;
	BYTE uc_aux4 = 0x00;
	uint16_t us_aux1 = 0x0000;
	uint16_t us_aux2 = 0x0000;
	uint16_t us_aux3 = 0x0000;
	uint16_t us_aux4 = 0x0000;
	int16_t s_aux1 = 0x0000;
	int16_t s_aux2 = 0x0000;
	int16_t s_aux3 = 0x0000;
	int16_t s_aux4 = 0x0000;
	uint32_t local_timestamp;
	BYTE local_temp_cel[NUM_CEL_BAT];
	uint16_t local_mv_cel[NUM_CEL_BAT];
	int i, j;
	/* Contador para envio de mensajes CAN: multiplos del temporizador TIEMPO_CAN_SUP_NS */
	static uint16_t cnt_ciclos_can = 0;
	static int cont_heartbeat_superv = 0;
	char peticion_off_local = 0;
	static BOOL superv_encendido = FALSE;
	
	
	/**************************************************
	***		INICIALIZACIONES		***
	**************************************************/
	/* Configura formato de mensajes CAN */
	msg_can_out.frame_inf.inf.FF = ExtFF;		// Modo extendido
	msg_can_out.frame_inf.inf.RTR = 0;		// Trama de datos

//----------------->Modificar esto	
	/* Para establecer el flag de envio solo una vez al inicio cuando el superv esta encendido */
	pthread_mutex_lock(&mut_supervisor);
	if ((supervisor.on == TRUE)&&(superv_encendido == FALSE))
		superv_encendido = TRUE;
	pthread_mutex_unlock(&mut_supervisor);


	/**************************************************
	***	ENVIO UNICAMENTE AL INICIO		***
	**************************************************/
	
	/* Envio unicamente una vez cuando el supervisor esta encendido */
	if(superv_encendido == TRUE)
	{
		superv_encendido = 2;
	}
	
	/* Envio solo la primera vez */
	if(cnt_ciclos_can == 0)
	{
	/*
		// Envio de mensaje: Modelo del controlador de motor 1
		msg_can_out.id = ID_MOTOR1_MODELO_CTRL;
		msg_can_out.frame_inf.inf.DLC = 8;
		pthread_mutex_lock(&mut_motor1);
		for (i=0; i<8; i++)
			msg_can_out.data[i] = motor1.modelo[i];
		pthread_mutex_unlock(&mut_motor1);
		pthread_mutex_lock(&mut_hdl);
		CanWrite(hdl, &msg_can_out);
		pthread_mutex_unlock(&mut_hdl);
		// Envio de mensaje: Modelo del controlador de motor 2
		msg_can_out.id = ID_MOTOR2_MODELO_CTRL;
		msg_can_out.frame_inf.inf.DLC = 8;
		pthread_mutex_lock(&mut_motor2);
		for (i=0; i<8; i++)
			msg_can_out.data[i] = motor2.modelo[i];
		pthread_mutex_unlock(&mut_motor2);
		pthread_mutex_lock(&mut_hdl);
		CanWrite(hdl, &msg_can_out);
		pthread_mutex_unlock(&mut_hdl);
		// Envio de mensaje: Modelo del controlador de motor 3
		msg_can_out.id = ID_MOTOR3_MODELO_CTRL;
		msg_can_out.frame_inf.inf.DLC = 8;
		pthread_mutex_lock(&mut_motor3);
		for (i=0; i<8; i++)
			msg_can_out.data[i] = motor3.modelo[i];
		pthread_mutex_unlock(&mut_motor3);
		pthread_mutex_lock(&mut_hdl);
		CanWrite(hdl, &msg_can_out);
		pthread_mutex_unlock(&mut_hdl);
		// Envio de mensaje: Modelo del controlador de motor 4
		msg_can_out.id = ID_MOTOR4_MODELO_CTRL;
		msg_can_out.frame_inf.inf.DLC = 8;
		pthread_mutex_lock(&mut_motor4);
		for (i=0; i<8; i++)
			msg_can_out.data[i] = motor4.modelo[i];
		pthread_mutex_unlock(&mut_motor4);
		pthread_mutex_lock(&mut_hdl);
		CanWrite(hdl, &msg_can_out);
		pthread_mutex_unlock(&mut_hdl);
	*/
	}

	/* Incremento del contador de ciclos can */
	cnt_ciclos_can++;
	/* Si el contador desborda */
	if (cnt_ciclos_can == 0) cnt_ciclos_can = 1;

	/**************************************************
	***	PERIODO = 1 x Temporizador TIEMPO_CAN_SUP_NS***
	**************************************************/
	/* Envio de mensaje: Errores a Supervisor - Criticos */
	msg_can_out.id = ID_ECU_SUPERV_ERC;
	msg_can_out.frame_inf.inf.DLC = 2;
	pthread_mutex_lock(&mut_errores);
	msg_can_out.data[0] = (BYTE) ((MASK_BYTE_ALTO & errores.er_critico_1)>>8);
	msg_can_out.data[1] = (BYTE) (MASK_BYTE_BAJO & errores.er_critico_1);
	pthread_mutex_unlock(&mut_errores);	
	pthread_mutex_lock(&mut_hdl);
	CanWrite(hdl, &msg_can_out);
	pthread_mutex_unlock(&mut_hdl);

	/* Envio de mensaje: Actuaciones del vehiculo */
	msg_can_out.id = ID_VEHICULO_ACTUACION;
	msg_can_out.frame_inf.inf.DLC = 6;
	pthread_mutex_lock(&mut_vehiculo);
	us_aux1 = (uint16_t) roundf(100 * vehiculo.freno);
	us_aux2 = (uint16_t) roundf(100 * vehiculo.acelerador);
	us_aux3 = (uint16_t) roundf(100 * vehiculo.volante);
	pthread_mutex_unlock(&mut_vehiculo);
	msg_can_out.data[0] = (BYTE) ((us_aux1 & MASK_BYTE_ALTO)>>8);
	msg_can_out.data[1] = (BYTE) (us_aux1 & MASK_BYTE_BAJO);
	msg_can_out.data[2] = (BYTE) ((us_aux2 & MASK_BYTE_ALTO)>>8);
	msg_can_out.data[3] = (BYTE) (us_aux2 & MASK_BYTE_BAJO);
	msg_can_out.data[4] = (BYTE) ((us_aux3 & MASK_BYTE_ALTO)>>8);
	msg_can_out.data[5] = (BYTE) (us_aux3 & MASK_BYTE_BAJO);
	pthread_mutex_lock(&mut_hdl);
	CanWrite(hdl, &msg_can_out);
	pthread_mutex_unlock(&mut_hdl);
	
	/* Envio de mensaje: Lectura de sensores de corriente del coche */
	msg_can_out.id = ID_VEHICULO_CORRIENTE;
	msg_can_out.frame_inf.inf.DLC = 4;
	pthread_mutex_lock(&mut_vehiculo);
	s_aux1 = (int16_t) roundf(100 * vehiculo.i_eje_d);
	s_aux2 = (int16_t) roundf(100 * vehiculo.i_eje_t);
	pthread_mutex_unlock(&mut_vehiculo);
	msg_can_out.data[0] = (BYTE) ((s_aux1 & MASK_BYTE_ALTO)>>8);
	msg_can_out.data[1] = (BYTE) (s_aux1 & MASK_BYTE_BAJO);
	msg_can_out.data[2] = (BYTE) ((s_aux2 & MASK_BYTE_ALTO)>>8);
	msg_can_out.data[3] = (BYTE) (s_aux2 & MASK_BYTE_BAJO);
	pthread_mutex_lock(&mut_hdl);
	CanWrite(hdl, &msg_can_out);
	pthread_mutex_unlock(&mut_hdl);
	
	/* Envio de mensaje: Estado general bateria */
	msg_can_out.id = ID_BMS_ESTADO;
	msg_can_out.frame_inf.inf.DLC = 7;
	pthread_mutex_lock(&mut_bateria);
	us_aux1 = (uint16_t) roundf((float) bateria.v_pack/10);
	msg_can_out.data[0] = (BYTE) bateria.num_cel_scan;
	msg_can_out.data[1] = (BYTE) bateria.temp_media;
	msg_can_out.data[2] = (BYTE) ((us_aux1 & MASK_BYTE_ALTO)>>8);	
	msg_can_out.data[3] = (BYTE) (us_aux1 & MASK_BYTE_BAJO);
	msg_can_out.data[4] = (BYTE) ((bateria.i_pack & MASK_BYTE_ALTO)>>8);
	msg_can_out.data[5] = (BYTE) (bateria.i_pack & MASK_BYTE_BAJO);
	msg_can_out.data[6] = (BYTE) bateria.soc;
	pthread_mutex_unlock(&mut_bateria);
	pthread_mutex_lock(&mut_hdl);
	CanWrite(hdl, &msg_can_out);
	pthread_mutex_unlock(&mut_hdl);
	
	/* Envio de mensaje: Alarmas BMS */
	msg_can_out.id = ID_BMS_ESTADO_T;
//	msg_can_out.frame_inf.inf.DLC = 4;
	msg_can_out.frame_inf.inf.DLC = 2;
	pthread_mutex_lock(&mut_bateria);
	msg_can_out.data[0] = (BYTE) bateria.temp_max;
	msg_can_out.data[1] = (BYTE) bateria.cel_temp_max;
//	msg_can_out.data[2] = (BYTE) bateria.nivel_alarma;
//	msg_can_out.data[3] = (BYTE) bateria.alarma;
	pthread_mutex_unlock(&mut_bateria);
	pthread_mutex_lock(&mut_hdl);
	CanWrite(hdl, &msg_can_out);
	pthread_mutex_unlock(&mut_hdl);
	
	
	/**************************************************
	***	PERIODO = 2 x Temporizador TIEMPO_CAN_SUP_NS	***
	**************************************************/
	if((cnt_ciclos_can % 2) == 0)
	{
		/* Envio de mensaje: Errores a Supervisor - Graves */
		msg_can_out.id = ID_ECU_SUPERV_ERG;
		msg_can_out.frame_inf.inf.DLC = 4;
		pthread_mutex_lock(&mut_errores);
		msg_can_out.data[0] = (BYTE) ((MASK_BYTE_ALTO & errores.er_grave_1)>>8);
		msg_can_out.data[1] = (BYTE) (MASK_BYTE_BAJO & errores.er_grave_1);
		msg_can_out.data[2] = (BYTE) ((MASK_BYTE_ALTO & errores.er_grave_2)>>8);
		msg_can_out.data[3] = (BYTE) (MASK_BYTE_BAJO & errores.er_grave_2);
		pthread_mutex_unlock(&mut_errores);
		pthread_mutex_lock(&mut_hdl);
		CanWrite(hdl, &msg_can_out);
		pthread_mutex_unlock(&mut_hdl);

		/* Envio de mensaje: Suspensiones del vehiculo */
		msg_can_out.id = ID_VEHICULO_SUSPENSION;
		msg_can_out.frame_inf.inf.DLC = 8;
		pthread_mutex_lock(&mut_vehiculo);
		us_aux1 = (uint16_t) roundf(100 * vehiculo.susp_ti);
		us_aux2 = (uint16_t) roundf(100 * vehiculo.susp_td);
		us_aux3 = (uint16_t) roundf(100 * vehiculo.susp_di);
		us_aux4 = (uint16_t) roundf(100 * vehiculo.susp_dd);
		pthread_mutex_unlock(&mut_vehiculo);
		msg_can_out.data[0] = (BYTE) ((us_aux1 & MASK_BYTE_ALTO)>>8);
		msg_can_out.data[1] = (BYTE) (us_aux1 & MASK_BYTE_BAJO);
		msg_can_out.data[2] = (BYTE) ((us_aux2 & MASK_BYTE_ALTO)>>8);
		msg_can_out.data[3] = (BYTE) (us_aux2 & MASK_BYTE_BAJO);
		msg_can_out.data[4] = (BYTE) ((us_aux3 & MASK_BYTE_ALTO)>>8);
		msg_can_out.data[5] = (BYTE) (us_aux3 & MASK_BYTE_BAJO);
		msg_can_out.data[6] = (BYTE) ((us_aux4 & MASK_BYTE_ALTO)>>8);
		msg_can_out.data[7] = (BYTE) (us_aux4 & MASK_BYTE_BAJO);
		pthread_mutex_lock(&mut_hdl);
		CanWrite(hdl, &msg_can_out);
		pthread_mutex_unlock(&mut_hdl);
	
		/* Envio de mensaje: medidas de la IMU del vehiculo - Acelerometros */
		msg_can_out.id = ID_IMU_DATOS_1;
		msg_can_out.frame_inf.inf.DLC = 6;
		pthread_mutex_lock(&mDatosImu);
		s_aux1 = (int16_t) roundf(tDatosImu.pfAccelScal[0] * 100);
		s_aux2 = (int16_t) roundf(tDatosImu.pfAccelScal[1] * 100);
		s_aux3 = (int16_t) roundf(tDatosImu.pfAccelScal[2] * 100);
		pthread_mutex_unlock(&mDatosImu);
		msg_can_out.data[0] = (BYTE) ((s_aux1 & MASK_BYTE_ALTO)>>8);
		msg_can_out.data[1] = (BYTE) (s_aux1 & MASK_BYTE_BAJO);
		msg_can_out.data[2] = (BYTE) ((s_aux2 & MASK_BYTE_ALTO)>>8);
		msg_can_out.data[3] = (BYTE) (s_aux2 & MASK_BYTE_BAJO);
		msg_can_out.data[4] = (BYTE) ((s_aux3 & MASK_BYTE_ALTO)>>8);
		msg_can_out.data[5] = (BYTE) (s_aux3 & MASK_BYTE_BAJO);
		pthread_mutex_lock(&mut_hdl);
		CanWrite(hdl, &msg_can_out);
		pthread_mutex_unlock(&mut_hdl);
	
		/* Envio de mensaje: medidas de la IMU del vehiculo - Giroscopos */
		msg_can_out.id = ID_IMU_DATOS_2;
		msg_can_out.frame_inf.inf.DLC = 6;
		pthread_mutex_lock(&mDatosImu);
		s_aux1 = (int16_t) roundf(tDatosImu.pfGyroScal[0] * 100);
		s_aux2 = (int16_t) roundf(tDatosImu.pfGyroScal[1] * 100);
		s_aux3 = (int16_t) roundf(tDatosImu.pfGyroScal[2] * 100);
		pthread_mutex_unlock(&mDatosImu);
		msg_can_out.data[0] = (BYTE) ((s_aux1 & MASK_BYTE_ALTO)>>8);
		msg_can_out.data[1] = (BYTE) (s_aux1 & MASK_BYTE_BAJO);
		msg_can_out.data[2] = (BYTE) ((s_aux2 & MASK_BYTE_ALTO)>>8);
		msg_can_out.data[3] = (BYTE) (s_aux2 & MASK_BYTE_BAJO);
		msg_can_out.data[4] = (BYTE) ((s_aux3 & MASK_BYTE_ALTO)>>8);
		msg_can_out.data[5] = (BYTE) (s_aux3 & MASK_BYTE_BAJO);
		pthread_mutex_lock(&mut_hdl);
		CanWrite(hdl, &msg_can_out);
		pthread_mutex_unlock(&mut_hdl);

		/* Envio de mensaje: medidas de la IMU del vehiculo - Timestamp */
		msg_can_out.id = ID_IMU_DATOS_3;
		msg_can_out.frame_inf.inf.DLC = 4;
		pthread_mutex_lock(&mDatosImu);
		local_timestamp = (uint32_t) roundf((float)tDatosImu.iTimeStamp/62500);
		pthread_mutex_unlock(&mDatosImu);
		us_aux1 = (uint16_t) ((local_timestamp & 0xFFFF0000)>>16);
		us_aux2 = (uint16_t) (local_timestamp & 0x0000FFFF);
		msg_can_out.data[0] = (BYTE) ((us_aux1 & MASK_BYTE_ALTO)>>8);
		msg_can_out.data[1] = (BYTE) (us_aux1 & MASK_BYTE_BAJO);
		msg_can_out.data[2] = (BYTE) ((us_aux2 & MASK_BYTE_ALTO)>>8);
		msg_can_out.data[3] = (BYTE) (us_aux2 & MASK_BYTE_BAJO);
		pthread_mutex_lock(&mut_hdl);
		CanWrite(hdl, &msg_can_out);
		pthread_mutex_unlock(&mut_hdl);

		/* Envio de mensaje: medidas de la IMU del vehiculo - Posicion LLH */
		msg_can_out.id = ID_IMU_DATOS_4;
		msg_can_out.frame_inf.inf.DLC = 6;
		pthread_mutex_lock(&mDatosImu);
		s_aux1 = (int16_t) round(tDatosImu.pdLLHPos[0] * 100);
		s_aux2 = (int16_t) round(tDatosImu.pdLLHPos[1] * 100);
		s_aux3 = (int16_t) round(tDatosImu.pdLLHPos[3] * 100);
		pthread_mutex_unlock(&mDatosImu);
		msg_can_out.data[0] = (BYTE) ((s_aux1 & MASK_BYTE_ALTO)>>8);
		msg_can_out.data[1] = (BYTE) (s_aux1 & MASK_BYTE_BAJO);
		msg_can_out.data[2] = (BYTE) ((s_aux2 & MASK_BYTE_ALTO)>>8);
		msg_can_out.data[3] = (BYTE) (s_aux2 & MASK_BYTE_BAJO);
		msg_can_out.data[4] = (BYTE) ((s_aux3 & MASK_BYTE_ALTO)>>8);
		msg_can_out.data[5] = (BYTE) (s_aux3 & MASK_BYTE_BAJO);
		pthread_mutex_lock(&mut_hdl);
		CanWrite(hdl, &msg_can_out);
		pthread_mutex_unlock(&mut_hdl);

		/* Envio de mensaje: medidas de la IMU del vehiculo - Modulo velocidad NED y vector de Euler */
		msg_can_out.id = ID_IMU_DATOS_5;
		msg_can_out.frame_inf.inf.DLC = 8;
		pthread_mutex_lock(&mDatosImu);
		s_aux1 = (int16_t) roundf(tDatosImu.pfNEDVeloc[3] * 100);
		s_aux2 = (int16_t) roundf(tDatosImu.pfEuler[0] * 100);
		s_aux3 = (int16_t) roundf(tDatosImu.pfEuler[1] * 100);
		s_aux4 = (int16_t) roundf(tDatosImu.pfEuler[2] * 100);
		pthread_mutex_unlock(&mDatosImu);
		msg_can_out.data[0] = (BYTE) ((s_aux1 & MASK_BYTE_ALTO)>>8);
		msg_can_out.data[1] = (BYTE) (s_aux1 & MASK_BYTE_BAJO);
		msg_can_out.data[2] = (BYTE) ((s_aux2 & MASK_BYTE_ALTO)>>8);
		msg_can_out.data[3] = (BYTE) (s_aux2 & MASK_BYTE_BAJO);
		msg_can_out.data[4] = (BYTE) ((s_aux3 & MASK_BYTE_ALTO)>>8);
		msg_can_out.data[5] = (BYTE) (s_aux3 & MASK_BYTE_BAJO);
		msg_can_out.data[6] = (BYTE) ((s_aux4 & MASK_BYTE_ALTO)>>8);
		msg_can_out.data[7] = (BYTE) (s_aux4 & MASK_BYTE_BAJO);
		pthread_mutex_lock(&mut_hdl);
		CanWrite(hdl, &msg_can_out);
		pthread_mutex_unlock(&mut_hdl);
	}
	
	/**************************************************
	***	PERIODO = 4 x Temporizador TIEMPO_CAN_SUP_NS	***
	**************************************************/
	if((cnt_ciclos_can % 4) == 0)
	{
		/* Envio de senial de apagado al supervisor si procede */
		pthread_mutex_lock(&mut_supervisor);
		peticion_off_local = supervisor.peticion_off;
		pthread_mutex_unlock(&mut_supervisor);
		if (peticion_off_local == TRUE)
		{
			msg_can_out.id = ID_SUPERV_OFF;
			msg_can_out.frame_inf.inf.DLC = 1;
			msg_can_out.data[0] = (BYTE) peticion_off_local;
			pthread_mutex_lock(&mut_hdl);
			CanWrite(hdl, &msg_can_out);
			pthread_mutex_unlock(&mut_hdl);
			pthread_mutex_lock(&mut_supervisor);
			supervisor.enviado_off = TRUE;
			pthread_mutex_unlock(&mut_supervisor);
		}
	
		/* Envio de mensaje: Valores de conversion A/D 1 - motor 1 */
		msg_can_out.id = ID_MOTOR1_CONV_AD1;
		msg_can_out.frame_inf.inf.DLC = 5;
		pthread_mutex_lock(&mut_motor1);
		msg_can_out.data[0] = (BYTE) roundf(motor1.freno*51);
		msg_can_out.data[1] = (BYTE) roundf(motor1.acel*51);
		msg_can_out.data[2] = (BYTE) roundf(motor1.v_pot*3);
		msg_can_out.data[3] = (BYTE) roundf(motor1.v_aux*6);
		msg_can_out.data[4] = (BYTE) roundf(motor1.v_bat*3);
		pthread_mutex_unlock(&mut_motor1);
		pthread_mutex_lock(&mut_hdl);
		CanWrite(hdl, &msg_can_out);
		pthread_mutex_unlock(&mut_hdl);
	
		/* Envio de mensaje: Valores de conversion A/D 2 - motor 1 */
		msg_can_out.id = ID_MOTOR1_CONV_AD2;
		msg_can_out.frame_inf.inf.DLC = 6;
		pthread_mutex_lock(&mut_motor1);
		msg_can_out.data[0] = motor1.i_a;
		msg_can_out.data[1] = motor1.i_b;
		msg_can_out.data[2] = motor1.i_c;
		msg_can_out.data[3] = (BYTE) roundf(motor1.v_a*3);
		msg_can_out.data[4] = (BYTE) roundf(motor1.v_b*3);
		msg_can_out.data[5] = (BYTE) roundf(motor1.v_c*3);
		pthread_mutex_unlock(&mut_motor1);
		pthread_mutex_lock(&mut_hdl);
		CanWrite(hdl, &msg_can_out);
		pthread_mutex_unlock(&mut_hdl);

		/* Envio de mensaje: Valores de conversion A/D 3 - motor 1 */
		msg_can_out.id = ID_MOTOR1_CONV_AD3;
		msg_can_out.frame_inf.inf.DLC = 7;
		pthread_mutex_lock(&mut_motor1);
		msg_can_out.data[0] = motor1.pwm;
		msg_can_out.data[1] = motor1.temp_int_ctrl;
		msg_can_out.data[2] = motor1.temp_sup_ctrl;
		msg_can_out.data[3] = motor1.temp_inf_ctrl;
		msg_can_out.data[4] = (BYTE) ((motor1.rpm & MASK_BYTE_ALTO)>>8);
		msg_can_out.data[5] = (BYTE) (motor1.rpm & MASK_BYTE_BAJO);
		msg_can_out.data[6] = motor1.i_porcent;
		pthread_mutex_unlock(&mut_motor1);
		pthread_mutex_lock(&mut_hdl);
		CanWrite(hdl, &msg_can_out);
		pthread_mutex_unlock(&mut_hdl);

		/* Envio de mensaje: Valores de conversion A/D 1 - motor 2 */
		msg_can_out.id = ID_MOTOR2_CONV_AD1;
		msg_can_out.frame_inf.inf.DLC = 5;
		pthread_mutex_lock(&mut_motor2);
		msg_can_out.data[0] = (BYTE) roundf(motor2.freno*51);
		msg_can_out.data[1] = (BYTE) roundf(motor2.acel*51);
		msg_can_out.data[2] = (BYTE) roundf(motor2.v_pot*3);
		msg_can_out.data[3] = (BYTE) roundf(motor2.v_aux*6);
		msg_can_out.data[4] = (BYTE) roundf(motor2.v_bat*3);
		pthread_mutex_unlock(&mut_motor2);
		pthread_mutex_lock(&mut_hdl);
		CanWrite(hdl, &msg_can_out);
		pthread_mutex_unlock(&mut_hdl);
	
		/* Envio de mensaje: Valores de conversion A/D 2 - motor 2 */
		msg_can_out.id = ID_MOTOR2_CONV_AD2;
		msg_can_out.frame_inf.inf.DLC = 6;
		pthread_mutex_lock(&mut_motor2);
		msg_can_out.data[0] = motor2.i_a;
		msg_can_out.data[1] = motor2.i_b;
		msg_can_out.data[2] = motor2.i_c;
		msg_can_out.data[3] = (BYTE) roundf(motor2.v_a*3);
		msg_can_out.data[4] = (BYTE) roundf(motor2.v_b*3);
		msg_can_out.data[5] = (BYTE) roundf(motor2.v_c*3);
		pthread_mutex_unlock(&mut_motor2);
		pthread_mutex_lock(&mut_hdl);
		CanWrite(hdl, &msg_can_out);
		pthread_mutex_unlock(&mut_hdl);

		/* Envio de mensaje: Valores de conversion A/D 3 - motor 2 */
		msg_can_out.id = ID_MOTOR2_CONV_AD3;
		msg_can_out.frame_inf.inf.DLC = 7;
		pthread_mutex_lock(&mut_motor2);
		msg_can_out.data[0] = motor2.pwm;
		msg_can_out.data[1] = motor2.temp_int_ctrl;
		msg_can_out.data[2] = motor2.temp_sup_ctrl;
		msg_can_out.data[3] = motor2.temp_inf_ctrl;
		msg_can_out.data[4] = (BYTE) ((motor2.rpm & MASK_BYTE_ALTO)>>8);
		msg_can_out.data[5] = (BYTE) (motor2.rpm & MASK_BYTE_BAJO);
		msg_can_out.data[6] = motor2.i_porcent;
		pthread_mutex_unlock(&mut_motor2);
		pthread_mutex_lock(&mut_hdl);
		CanWrite(hdl, &msg_can_out);
		pthread_mutex_unlock(&mut_hdl);
		
		/* Envio de mensaje: Valores de conversion A/D 1 - motor 3 */
		msg_can_out.id = ID_MOTOR3_CONV_AD1;
		msg_can_out.frame_inf.inf.DLC = 5;
		pthread_mutex_lock(&mut_motor3);
		msg_can_out.data[0] = (BYTE) roundf(motor3.freno*51);
		msg_can_out.data[1] = (BYTE) roundf(motor3.acel*51);
		msg_can_out.data[2] = (BYTE) roundf(motor3.v_pot*3);
		msg_can_out.data[3] = (BYTE) roundf(motor3.v_aux*6);
		msg_can_out.data[4] = (BYTE) roundf(motor3.v_bat*3);
		pthread_mutex_unlock(&mut_motor3);
		pthread_mutex_lock(&mut_hdl);
		CanWrite(hdl, &msg_can_out);
		pthread_mutex_unlock(&mut_hdl);
	
		/* Envio de mensaje: Valores de conversion A/D 2 - motor 3 */
		msg_can_out.id = ID_MOTOR3_CONV_AD2;
		msg_can_out.frame_inf.inf.DLC = 6;
		pthread_mutex_lock(&mut_motor3);
		msg_can_out.data[0] = motor3.i_a;
		msg_can_out.data[1] = motor3.i_b;
		msg_can_out.data[2] = motor3.i_c;
		msg_can_out.data[3] = (BYTE) roundf(motor3.v_a*3);
		msg_can_out.data[4] = (BYTE) roundf(motor3.v_b*3);
		msg_can_out.data[5] = (BYTE) roundf(motor3.v_c*3);
		pthread_mutex_unlock(&mut_motor3);
		pthread_mutex_lock(&mut_hdl);
		CanWrite(hdl, &msg_can_out);
		pthread_mutex_unlock(&mut_hdl);

		/* Envio de mensaje: Valores de conversion A/D 3 - motor 3 */
		msg_can_out.id = ID_MOTOR3_CONV_AD3;
		msg_can_out.frame_inf.inf.DLC = 7;
		pthread_mutex_lock(&mut_motor3);
		msg_can_out.data[0] = motor3.pwm;
		msg_can_out.data[1] = motor3.temp_int_ctrl;
		msg_can_out.data[2] = motor3.temp_sup_ctrl;
		msg_can_out.data[3] = motor3.temp_inf_ctrl;
		msg_can_out.data[4] = (BYTE) ((motor3.rpm & MASK_BYTE_ALTO)>>8);
		msg_can_out.data[5] = (BYTE) (motor3.rpm & MASK_BYTE_BAJO);
		msg_can_out.data[6] = motor3.i_porcent;
		pthread_mutex_unlock(&mut_motor3);
		pthread_mutex_lock(&mut_hdl);
		CanWrite(hdl, &msg_can_out);
		pthread_mutex_unlock(&mut_hdl);
		
		/* Envio de mensaje: Valores de conversion A/D 1 - motor 4 */
		msg_can_out.id = ID_MOTOR4_CONV_AD1;
		msg_can_out.frame_inf.inf.DLC = 5;
		pthread_mutex_lock(&mut_motor4);
		msg_can_out.data[0] = (BYTE) roundf(motor4.freno*51);
		msg_can_out.data[1] = (BYTE) roundf(motor4.acel*51);
		msg_can_out.data[2] = (BYTE) roundf(motor4.v_pot*3);
		msg_can_out.data[3] = (BYTE) roundf(motor4.v_aux*6);
		msg_can_out.data[4] = (BYTE) roundf(motor4.v_bat*3);
		pthread_mutex_unlock(&mut_motor4);
		pthread_mutex_lock(&mut_hdl);
		CanWrite(hdl, &msg_can_out);
		pthread_mutex_unlock(&mut_hdl);
	
		/* Envio de mensaje: Valores de conversion A/D 2 - motor 4 */
		msg_can_out.id = ID_MOTOR4_CONV_AD2;
		msg_can_out.frame_inf.inf.DLC = 6;
		pthread_mutex_lock(&mut_motor4);
		msg_can_out.data[0] = motor4.i_a;
		msg_can_out.data[1] = motor4.i_b;
		msg_can_out.data[2] = motor4.i_c;
		msg_can_out.data[3] = (BYTE) roundf(motor4.v_a*3);
		msg_can_out.data[4] = (BYTE) roundf(motor4.v_b*3);
		msg_can_out.data[5] = (BYTE) roundf(motor4.v_c*3);
		pthread_mutex_unlock(&mut_motor4);
		pthread_mutex_lock(&mut_hdl);
		CanWrite(hdl, &msg_can_out);
		pthread_mutex_unlock(&mut_hdl);

		/* Envio de mensaje: Valores de conversion A/D 3 - motor 4 */
		msg_can_out.id = ID_MOTOR4_CONV_AD3;
		msg_can_out.frame_inf.inf.DLC = 7;
		pthread_mutex_lock(&mut_motor4);
		msg_can_out.data[0] = motor4.pwm;
		msg_can_out.data[1] = motor4.temp_int_ctrl;
		msg_can_out.data[2] = motor4.temp_sup_ctrl;
		msg_can_out.data[3] = motor4.temp_inf_ctrl;
		msg_can_out.data[4] = (BYTE) ((motor4.rpm & MASK_BYTE_ALTO)>>8);
		msg_can_out.data[5] = (BYTE) (motor4.rpm & MASK_BYTE_BAJO);
		msg_can_out.data[6] = motor4.i_porcent;
		pthread_mutex_unlock(&mut_motor4);
		pthread_mutex_lock(&mut_hdl);
		CanWrite(hdl, &msg_can_out);
		pthread_mutex_unlock(&mut_hdl);
	}
	
	/**************************************************
	***	PERIODO = 5 x Temporizador TIEMPO_CAN_SUP_NS	***
	**************************************************/
	if((cnt_ciclos_can % 5) == 0)
	{
//		printf("Envio de mensajes can al supervisor 5.\n");
		/* Envio de mensaje: Voltajes generales */
		msg_can_out.id = ID_BMS_ESTADO_V;
		msg_can_out.frame_inf.inf.DLC = 8;
		pthread_mutex_lock(&mut_bateria);
		us_aux1 = bateria.v_medio;
		us_aux2 = bateria.v_max;
		uc_aux1 = bateria.cel_v_max;
		us_aux3 = bateria.v_min;
		uc_aux2 = bateria.cel_v_min;
		pthread_mutex_unlock(&mut_bateria);
		msg_can_out.data[0] = (BYTE) ((us_aux1 & MASK_BYTE_ALTO)>>8);
		msg_can_out.data[1] = (BYTE) (us_aux1 & MASK_BYTE_BAJO);
		msg_can_out.data[2] = (BYTE) ((us_aux2 & MASK_BYTE_ALTO)>>8);
		msg_can_out.data[3] = (BYTE) (us_aux2 & MASK_BYTE_BAJO);
		msg_can_out.data[4] = (BYTE) uc_aux1;
		msg_can_out.data[5] = (BYTE) ((us_aux3 & MASK_BYTE_ALTO)>>8);
		msg_can_out.data[6] = (BYTE) (us_aux3 & MASK_BYTE_BAJO);
		msg_can_out.data[7] = (BYTE) uc_aux2;
		pthread_mutex_lock(&mut_hdl);
		CanWrite(hdl, &msg_can_out);
		pthread_mutex_unlock(&mut_hdl);

		/* Envio de mensaje: Temperatura individual de celda */
		/* Vuelca la variable global en la local */
		pthread_mutex_lock(&mut_bateria);
		memcpy(local_temp_cel, bateria.temp_cel, sizeof(local_temp_cel));
		pthread_mutex_unlock(&mut_bateria);
		/* Construccion y envio de mensajes */
		msg_can_out.id = ID_BMS_TEMP_CELDAS_1;
		msg_can_out.frame_inf.inf.DLC = 8;
		for(i=0; i<8; i++)
			msg_can_out.data[i] = (BYTE) local_temp_cel[i];
		pthread_mutex_lock(&mut_hdl);
		CanWrite(hdl, &msg_can_out);
		pthread_mutex_unlock(&mut_hdl);
		
		msg_can_out.id = ID_BMS_TEMP_CELDAS_2;
		msg_can_out.frame_inf.inf.DLC = 8;
		for(i=0; i<8; i++)
			msg_can_out.data[i] = (BYTE) local_temp_cel[i+8];
		pthread_mutex_lock(&mut_hdl);
		CanWrite(hdl, &msg_can_out);
		pthread_mutex_unlock(&mut_hdl);
		
		msg_can_out.id = ID_BMS_TEMP_CELDAS_3;
		msg_can_out.frame_inf.inf.DLC = 8;
		for(i=0; i<8; i++)
			msg_can_out.data[i] = (BYTE) local_temp_cel[i+16];
		pthread_mutex_lock(&mut_hdl);
		CanWrite(hdl, &msg_can_out);
		pthread_mutex_unlock(&mut_hdl);
		
		/* Envio de mensaje: Voltaje individual de celda */
		/* Vuelca la variable global en la local */
		pthread_mutex_lock(&mut_bateria);
		memcpy(local_mv_cel, bateria.mv_cel, sizeof(local_mv_cel));
		pthread_mutex_unlock(&mut_bateria);
		/* Construccion y envio de mensajes */
		msg_can_out.id = ID_BMS_V_CELDAS_1;
		msg_can_out.frame_inf.inf.DLC = 8;
		j = 0;
		for(i=0; i<8; i+=2)
		{
			msg_can_out.data[i] = (BYTE)((local_mv_cel[j] & MASK_BYTE_ALTO)>>8);
			msg_can_out.data[i+1] = (BYTE)(local_mv_cel[j] & MASK_BYTE_BAJO);
			j++;
		}
		pthread_mutex_lock(&mut_hdl);
		CanWrite(hdl, &msg_can_out);
		pthread_mutex_unlock(&mut_hdl);
		msg_can_out.id = ID_BMS_V_CELDAS_2;
		msg_can_out.frame_inf.inf.DLC = 8;
		for(i=0; i<8; i+=2)
		{
			msg_can_out.data[i] = (BYTE)((local_mv_cel[j] & MASK_BYTE_ALTO)>>8);
			msg_can_out.data[i+1] = (BYTE)(local_mv_cel[j] & MASK_BYTE_BAJO);
			j++;
		}
		pthread_mutex_lock(&mut_hdl);
		CanWrite(hdl, &msg_can_out);
		pthread_mutex_unlock(&mut_hdl);
		msg_can_out.id = ID_BMS_V_CELDAS_3;
		msg_can_out.frame_inf.inf.DLC = 8;
		for(i=0; i<8; i+=2)
		{
			msg_can_out.data[i] = (BYTE)((local_mv_cel[j] & MASK_BYTE_ALTO)>>8);
			msg_can_out.data[i+1] = (BYTE)(local_mv_cel[j] & MASK_BYTE_BAJO);
			j++;
		}
		pthread_mutex_lock(&mut_hdl);
		CanWrite(hdl, &msg_can_out);
		pthread_mutex_unlock(&mut_hdl);
		msg_can_out.id = ID_BMS_V_CELDAS_4;
		msg_can_out.frame_inf.inf.DLC = 8;
		for(i=0; i<8; i+=2)
		{
			msg_can_out.data[i] = (BYTE)((local_mv_cel[j] & MASK_BYTE_ALTO)>>8);
			msg_can_out.data[i+1] = (BYTE)(local_mv_cel[j] & MASK_BYTE_BAJO);
			j++;
		}
		pthread_mutex_lock(&mut_hdl);
		CanWrite(hdl, &msg_can_out);
		pthread_mutex_unlock(&mut_hdl);
		msg_can_out.id = ID_BMS_V_CELDAS_5;
		msg_can_out.frame_inf.inf.DLC = 8;
		for(i=0; i<8; i+=2)
		{
			msg_can_out.data[i] = (BYTE) ((local_mv_cel[j] & MASK_BYTE_ALTO)>>8);
			msg_can_out.data[i+1] = (BYTE) (local_mv_cel[j] & MASK_BYTE_BAJO);
			j++;
		}
		pthread_mutex_lock(&mut_hdl);
		CanWrite(hdl, &msg_can_out);
		pthread_mutex_unlock(&mut_hdl);
		msg_can_out.id = ID_BMS_V_CELDAS_6;
		msg_can_out.frame_inf.inf.DLC = 8;
		for(i=0; i<8; i+=2)
		{
			msg_can_out.data[i] = (BYTE) ((local_mv_cel[j] & MASK_BYTE_ALTO)>>8);
			msg_can_out.data[i+1] = (BYTE) (local_mv_cel[j] & MASK_BYTE_BAJO);
			j++;
		}
		pthread_mutex_lock(&mut_hdl);
		CanWrite(hdl, &msg_can_out);
		pthread_mutex_unlock(&mut_hdl);
		
		/* Envio de mensaje: Errores a Supervisor - Leves */
		msg_can_out.id = ID_ECU_SUPERV_ERL;
		msg_can_out.frame_inf.inf.DLC = 4;
		pthread_mutex_lock(&mut_errores);
		uc_aux1 = (MASK_BYTE_ALTO & errores.er_leve_1);
		uc_aux2 = (MASK_BYTE_BAJO & errores.er_leve_1);
		uc_aux3 = (MASK_BYTE_ALTO & errores.er_leve_2);
		uc_aux4 = (MASK_BYTE_BAJO & errores.er_leve_2);
		pthread_mutex_unlock(&mut_errores);
		msg_can_out.data[0] = (BYTE) (uc_aux1>>8);
		msg_can_out.data[1] = (BYTE) uc_aux2;
		msg_can_out.data[2] = (BYTE) (uc_aux3>>8);
		msg_can_out.data[3] = (BYTE) uc_aux4;
		pthread_mutex_lock(&mut_hdl);
		CanWrite(hdl, &msg_can_out);
		pthread_mutex_unlock(&mut_hdl);
		/* Reseteo de variables auxiliares */
		uc_aux1 = 0;
		uc_aux2 = 0;
		uc_aux3 = 0;
		uc_aux4 = 0;	
	}
	
	/**************************************************
	***	PERIODO = 6 x Temporizador TIEMPO_CAN_SUP_NS	***
	**************************************************/
	if((cnt_ciclos_can % 6) == 0)
	{
	
	}

	/**************************************************
	***	PERIODO = 8 x Temporizador TIEMPO_CAN_SUP_NS	***
	**************************************************/
	if((cnt_ciclos_can % 8) == 0)
	{
	
		
	}
	
	/**************************************************
	***	PERIODO = 10 x Temporizador TIEMPO_CAN_SUP_NS	***
	**************************************************/
	if((cnt_ciclos_can % 10) == 0)
	{
		/* Mensaje de heartbeat */
		msg_can_out.id = ID_WATCHDOG;
		msg_can_out.frame_inf.inf.DLC = 1;
		msg_can_out.data[0] = (BYTE) 0x0000;
		pthread_mutex_lock(&mut_hdl);
		CanWrite(hdl, &msg_can_out);
		pthread_mutex_unlock(&mut_hdl);
	
		/* Comprueba watchdog del supervisor */
		pthread_mutex_lock(&mut_supervisor);
		heartbeat_superv_local = supervisor.heartbeat;
		pthread_mutex_unlock(&mut_supervisor);
		if(heartbeat_superv_local == FALSE)
		{
			cont_heartbeat_superv++;
			/* Si hay error leve de falta de heartbeat de supervisor */
			if (cont_heartbeat_superv >= LIMITE_ERL_HEARTBEAT)
			{
				pthread_mutex_lock(&mut_errores);
				errores.er_leve_1 |= ERL_ECU_COM_SUPERV;
				pthread_mutex_unlock(&mut_errores);
			}
			/* Si hay error grave de falta de heartbeat de supervisor */
			if (cont_heartbeat_superv >= LIMITE_ERG_HEARTBEAT)
			{
				pthread_mutex_lock(&mut_errores);
				errores.er_leve_1 &= ~ERL_ECU_COM_SUPERV;
				errores.er_grave_1 |= ERG_ECU_COM_SUPERV;
				pthread_mutex_unlock(&mut_errores);
			}
			/* Para evitar desbordamiento */
			if (cont_heartbeat_superv > 30)
				cont_heartbeat_superv = 30;	
		}
		else
		{
			cont_heartbeat_superv = 0;
			/* Si ha llegado el heartbeat del supervisor significa que ya no hay error */
			pthread_mutex_lock(&mut_errores);
			errores.er_leve_1 &= ~ERL_ECU_COM_SUPERV;
			errores.er_grave_1 &= ~ERG_ECU_COM_SUPERV;
			pthread_mutex_unlock(&mut_errores);
			pthread_mutex_lock(&mut_supervisor);
			supervisor.heartbeat = FALSE;
			pthread_mutex_unlock(&mut_supervisor);
		}
	}
	/**************************************************
	***	PERIODO = 120 x Temporizador TIEMPO_CAN_SUP_NS	***
	**************************************************/
	if((cnt_ciclos_can % 120) == 0)
	{
		/* Mensaje de version de la ECU para el supervisor */
		msg_can_out.id = ID_SUPERV_VER;
		msg_can_out.frame_inf.inf.DLC = 1;
		msg_can_out.data[0] = (BYTE) (VERSION_ECU/10);
		pthread_mutex_lock(&mut_hdl);
		CanWrite(hdl, &msg_can_out);
		pthread_mutex_unlock(&mut_hdl);
		
		/* Envio de mensaje: Zonas muertas de acelerador y freno en motor 1 */
		msg_can_out.id = ID_MOTOR1_ZONA_MUERTA;
		msg_can_out.frame_inf.inf.DLC = 5;
		pthread_mutex_lock(&mut_motor1);
		msg_can_out.data[0] = (BYTE) motor1.version;
		msg_can_out.data[1] = (BYTE) motor1.zm_inf_acel;
		msg_can_out.data[2] = (BYTE) motor1.zm_sup_acel;
		msg_can_out.data[3] = (BYTE) motor1.zm_inf_fren;
		msg_can_out.data[4] = (BYTE) motor1.zm_sup_fren;
		pthread_mutex_unlock(&mut_motor1);
		pthread_mutex_lock(&mut_hdl);
		CanWrite(hdl, &msg_can_out);
		pthread_mutex_unlock(&mut_hdl);
	
		/* Envio de mensaje: Zonas muertas de acelerador y freno en motor 2 */
		msg_can_out.id = ID_MOTOR2_ZONA_MUERTA;
		msg_can_out.frame_inf.inf.DLC = 5;
		pthread_mutex_lock(&mut_motor2);
		msg_can_out.data[0] = (BYTE) motor2.version;
		msg_can_out.data[1] = (BYTE) motor2.zm_inf_acel;
		msg_can_out.data[2] = (BYTE) motor2.zm_sup_acel;
		msg_can_out.data[3] = (BYTE) motor2.zm_inf_fren;
		msg_can_out.data[4] = (BYTE) motor2.zm_sup_fren;
		pthread_mutex_unlock(&mut_motor2);
		pthread_mutex_lock(&mut_hdl);
		CanWrite(hdl, &msg_can_out);
		pthread_mutex_unlock(&mut_hdl);
		
		/* Envio de mensaje: Zonas muertas de acelerador y freno en motor 3 */
		msg_can_out.id = ID_MOTOR3_ZONA_MUERTA;
		msg_can_out.frame_inf.inf.DLC = 5;
		pthread_mutex_lock(&mut_motor3);
		msg_can_out.data[0] = (BYTE) motor3.version;
		msg_can_out.data[1] = (BYTE) motor3.zm_inf_acel;
		msg_can_out.data[2] = (BYTE) motor3.zm_sup_acel;
		msg_can_out.data[3] = (BYTE) motor3.zm_inf_fren;
		msg_can_out.data[4] = (BYTE) motor3.zm_sup_fren;
		pthread_mutex_unlock(&mut_motor3);
		pthread_mutex_lock(&mut_hdl);
		CanWrite(hdl, &msg_can_out);
		pthread_mutex_unlock(&mut_hdl);
		
		/* Envio de mensaje: Zonas muertas de acelerador y freno en motor 4 */
		msg_can_out.id = ID_MOTOR4_ZONA_MUERTA;
		msg_can_out.frame_inf.inf.DLC = 5;
		pthread_mutex_lock(&mut_motor4);
		msg_can_out.data[0] = (BYTE) motor4.version;
		msg_can_out.data[1] = (BYTE) motor4.zm_inf_acel;
		msg_can_out.data[2] = (BYTE) motor4.zm_sup_acel;
		msg_can_out.data[3] = (BYTE) motor4.zm_inf_fren;
		msg_can_out.data[4] = (BYTE) motor4.zm_sup_fren;
		pthread_mutex_unlock(&mut_motor4);
		pthread_mutex_lock(&mut_hdl);
		CanWrite(hdl, &msg_can_out);
		pthread_mutex_unlock(&mut_hdl);
	}
}