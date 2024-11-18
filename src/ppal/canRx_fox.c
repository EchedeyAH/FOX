/**************************************************************************
***									***
***	Fichero: canRx_fox.c						***
***	Fecha: 01/10/2013						***
***	Autor: Elena Gonzalez						***
***	Descripción: Hilo de adquisicion de datos por CAN para motores.	***
***	Recogida de datos CAN de los motores.				***
***	Fragmentos de codigo de Alejandro Oliva Torres.			***
***									***
**************************************************************************/

/**************************************************
***		  CABECERAS			***
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

/* Estructuras globales */
extern est_error_t errores;

/* Mutex globales (inic. en el main) */
extern pthread_mutex_t mut_errores;
extern pthread_mutex_t mut_hilos_listos;
extern pthread_mutex_t mut_inicio;
extern pthread_cond_t inicio;

/* Buffers CAN */
int cont_buff_rx = 0;	// Contador de mensajes en el buffer de recepcion del supervisor
struct can_object *buff_rx_superv; // Buffer de recepcion de mensajes procedientes del supervisor
struct can_object buff_rx_mot1;		// Buffer de recepcion de mensajes procedientes del motor 1
struct can_object buff_rx_mot2;		// Buffer de recepcion de mensajes procedientes del motor 2
struct can_object buff_rx_mot3;		// Buffer de recepcion de mensajes procedientes del motor 3
struct can_object buff_rx_mot4;		// Buffer de recepcion de mensajes procedientes del motor 4

/* Identificador de la conexion CAN */
canhdl_t hdl;

/* Mutex */
pthread_mutex_t mut_rx_mot1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mut_rx_mot2 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mut_rx_mot3 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mut_rx_mot4 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mut_rx_superv = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t mut_hay_rx_mot1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mut_hay_rx_mot2 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mut_hay_rx_mot3 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mut_hay_rx_mot4 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t hay_rx_mot1 = PTHREAD_COND_INITIALIZER;
pthread_cond_t hay_rx_mot2 = PTHREAD_COND_INITIALIZER;
pthread_cond_t hay_rx_mot3 = PTHREAD_COND_INITIALIZER;
pthread_cond_t hay_rx_mot4 = PTHREAD_COND_INITIALIZER;
/* Bloquea el handler de tx CAN */
pthread_mutex_t mut_hdl = PTHREAD_MUTEX_INITIALIZER;


/******************************************************************************************
***			FUNCION PRINCIPAL: canRx_main (hilo) 				***
******************************************************************************************/
void *can_rx(void *pi)
{
	BOOL fin_local = 0;
	/* Estatus del driver CAN */
	struct status st;
	/* Recoge el estado del buffer CAN */
//	short resp;
	/* Recoge el mensaje CAN */
	struct can_object rxmsg;
	/* Parametos para comprobar resultados de las funciones del driver CAN */
	int chid;
	int coid;
	int rcvid;
	int condriver;
	/* Variables para temporizacion */
	struct sigevent pulse_event;		// Manejo del evento del pulso
	struct _pulse pulse;			// Variable para manejar el pulso			
//	struct can_object msg_can_in;		// Mensaje CAN de entrada
	struct timespec espera={0, TEMPO_CAN};  // Tiempo base para espera de mensajes CAN
	short contador_timeout = 0;
	/* Recoge el valor devuelto por las funciones CAN */
	short dev_can;
	/* Recoge la configuracion del driver can y la establece */
//	struct config conf_can;

	/**************************************************
	***		DRIVER CAN			***
	**************************************************/
	/* Crea canal de notificacion */
	do {
		chid = ChannelCreate(0);
		/* Espera 0,5 segundos */
		nanosleep(&espera, NULL);
		contador_timeout++;
	} while ((chid == -1) && (contador_timeout <= TIEMPO_EXP_CAN_X));
	/* No continua hasta que no esta todo OK */

	if (contador_timeout > TIEMPO_EXP_CAN_X)
	{
		/* Error de arranque can */
		printf("CAN1 channel error\n");
		pthread_mutex_lock(&mut_errores);
		errores.er_critico_1 |= ERC_ECU_COM_TCAN1;
		pthread_mutex_unlock(&mut_errores);
	}
	else
	{
		contador_timeout = 0;
		/* Conecta el canal al proceso */
		do {
			coid = ConnectAttach(0, 0, chid, _NTO_SIDE_CHANNEL, 0);
			/* Espera 0,5 segundos */
			nanosleep(&espera, NULL);
			contador_timeout++;
		} while ((coid == -1) && (contador_timeout <= TIEMPO_EXP_CAN_X));
		/* No continua hasta que no esta todo OK */
	
		if (contador_timeout > TIEMPO_EXP_CAN_X)
		{
			/* Error de arranque can */
			perror("CAN1 connect channel error\n");
			pthread_mutex_lock(&mut_errores);
			errores.er_critico_1 |= ERC_ECU_COM_TCAN1;
			pthread_mutex_unlock(&mut_errores);
		}
		else
		{
			contador_timeout = 0;
			/* Inicializacion de evento de notificacion */
			SIGEV_PULSE_INIT(&pulse_event, coid, getprio(0), _PULSE_CODE_MINAVAIL, 0);

			/* Arranca el driver CAN en el canal 1 */
			do {
				condriver = ConnectDriver(1, "CANDRV", &hdl);
				printf("Arranque CAN1: %d \n", condriver);
				/* Espera 0,5 segundos */
				nanosleep(&espera, NULL);
				contador_timeout++;
			} while((condriver != ERR_OK) && (contador_timeout <= TIEMPO_EXP_CAN_X));
			/* No continua hasta que no esta todo OK */
	
			if (contador_timeout > TIEMPO_EXP_CAN_X)
			{
				/* Error de arranque can */
				perror("CAN1 connect driver error\n");
				pthread_mutex_lock(&mut_errores);
				errores.er_critico_1 |= ERC_ECU_COM_TCAN1;
				pthread_mutex_unlock(&mut_errores);
			}
			else
			{
				contador_timeout = 0;		// Resetea el contador de timeout
				/* Resetea el buffer CAN */
				dev_can = CanRestart(hdl);
				printf("CAN1: CanRestart devuelve %d\n", (int)dev_can);
				/* Espera de pulsos: registra un pulso por cada lectura CAN */
			   	RegRdPulse(hdl, &pulse_event);
			   	printf ("\nCAN1 status: %04x\n", CanGetStatus(hdl, &st));
			}
		}
	}
	
   	/* Reservamos memoria para el buffer de recepcion del supervisor */
	buff_rx_superv = (struct can_object *) malloc(TAM_BUFF_RX * sizeof(struct can_object));

	/* Indica que el hilo de recepcion CAN esta listo */
	pthread_mutex_lock(&mut_hilos_listos);
	hilos_listos |= HILO_CANRX_LISTO;
	pthread_mutex_unlock(&mut_hilos_listos);
	printf("Hilo Rx CAN listo\n");
	pthread_mutex_lock(&mut_inicio);
	pthread_cond_wait(&inicio, &mut_inicio);
	pthread_mutex_unlock(&mut_inicio);
	
	while (!fin_local)
	{
		/* Espera al pulso que se da cuando se recibe un mensaje por el canal CAN */
		rcvid = MsgReceivePulse(chid, &pulse, sizeof(struct _pulse), NULL);
//		printf("rcvid=%d - canRx\n", rcvid);
		if (rcvid == -1)
		{
			printf("Error recepcion CAN bus 1\n");
			contador_timeout++;
			if (contador_timeout > TIEMPO_EXP_CAN_X)
			{
				/* Error critico recepcion can */
				printf("CAN error critico recepcion\n");
				pthread_mutex_lock(&mut_errores);
				errores.er_critico_1 |= ERC_ECU_COM_TCAN1;
				pthread_mutex_unlock(&mut_errores);
			}
		}
		else
		{
			contador_timeout = 0;
			/* Leemos el siguiente mensaje */
			pthread_mutex_lock(&mut_hdl);
			dev_can = CanRead (hdl, &rxmsg, NULL);
			pthread_mutex_unlock(&mut_hdl);			
			/* Filtra a partir del identificador del mensaje CAN recibido */
			switch (rxmsg.id)
			{
				 case ID_CTRL_MOTOR_1_C_ECU:
					pthread_mutex_lock(&mut_rx_mot1);
					buff_rx_mot1 = rxmsg;
					pthread_mutex_unlock(&mut_rx_mot1);
					pthread_mutex_lock(&mut_hay_rx_mot1);
					pthread_cond_signal(&hay_rx_mot1);
					pthread_mutex_unlock(&mut_hay_rx_mot1);
				break;
				case ID_CTRL_MOTOR_2_C_ECU:
					pthread_mutex_lock(&mut_rx_mot2);
					buff_rx_mot2 = rxmsg;
					pthread_mutex_unlock(&mut_rx_mot2);
					pthread_mutex_lock(&mut_hay_rx_mot2);
					pthread_cond_signal(&hay_rx_mot2);
					pthread_mutex_unlock(&mut_hay_rx_mot2);
				break;
				case ID_CTRL_MOTOR_3_C_ECU:
					pthread_mutex_lock(&mut_rx_mot3);
					buff_rx_mot3 = rxmsg;
					pthread_mutex_unlock(&mut_rx_mot3);
					pthread_mutex_lock(&mut_hay_rx_mot3);
					pthread_cond_signal(&hay_rx_mot3);
					pthread_mutex_unlock(&mut_hay_rx_mot3);
				break;
				case ID_CTRL_MOTOR_4_C_ECU:
					pthread_mutex_lock(&mut_rx_mot4);
					buff_rx_mot4 = rxmsg;
					pthread_mutex_unlock(&mut_rx_mot4);
					pthread_mutex_lock(&mut_hay_rx_mot4);
					pthread_cond_signal(&hay_rx_mot4);
					pthread_mutex_unlock(&mut_hay_rx_mot4);
				break;				
				default:
					if ((rxmsg.id==ID_SUPERV_HEARTBEAT)||(rxmsg.id==ID_SUPERV_OFF))
					{
						pthread_mutex_lock(&mut_rx_superv);
						if (cont_buff_rx > TAM_BUFF_RX)
						{
							cont_buff_rx = TAM_BUFF_RX;
							printf("Overflow buffer Rx supervisor\n");
						}
						buff_rx_superv[cont_buff_rx] = rxmsg;
						cont_buff_rx++;
						pthread_mutex_unlock(&mut_rx_superv);
					}
			}
		}
		pthread_mutex_lock(&mut_hilos_listos);
		if ((hilos_listos&HILO_CANRX_LISTO)==0x0000)
			fin_local = 1;
		pthread_mutex_unlock(&mut_hilos_listos);
	}
	
	/* Liberamos la memoria del buffer de recepcion del supervisor */
	free (buff_rx_superv);
	/* Desconectamos el driver CAN */
	DisConnectDriver(&hdl);
	/* Cerramos el hilo */
	pthread_exit(NULL);
	
	return NULL;
}
