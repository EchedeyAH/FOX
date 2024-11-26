/**************************************************************************
***									***
***	Fichero: can2_fox.c						***
***	Fecha: 02/04/2013						***
***	Autor: Elena Gonzalez						***
***	Descripción: Proceso de adquisicion de datos por CAN		***
***	Recogida de datos CAN desde la BMS.				***
***									***
**************************************************************************/

// CABECERAS
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
/* Can includes */
#include "./include/candef.h"
#include "./include/canstr.h"
#include "./include/canglob.h"
/* Include local */
#include "./include/can2_fox.h"

/**************************************************
***		  VARIABLES GLOBALES		***
**************************************************/
/* Estructuras globales */
dato_cola_t dato_cola;

/* Identificador de la conexion CAN */
canhdl_t hdl;

/******************************************************************************************
***				DEFINICION FUNCION: main				***
*** Configura e inicia el driver CAN y la cola de mensajes. Recibe los mensajes CAN	***
*** de la BMS, los interpreta y envia los datos al proceso principal de la ECU mediante	***
*** la cola de mensajes.								***
******************************************************************************************/
int main(void)
{
	/**************************************************
	***		VARIABLES			***
	**************************************************/
	int fin = 0;	// Marca el fin del bucle de mensajes CAN
	
	/* Estatus del driver CAN */
	struct status st;
	/* Recoge el estado del buffer CAN */
	short resp;
	/* Parametos para comprobar resultados de las funciones del driver CAN */
	int chid;
	int coid;
	int rcvid;
	int condriver;
	/* Variables para temporizacion */
	struct sigevent pulse_event;			// Manejo del evento del pulso
//	struct sigevent event;
	struct _pulse pulse;				// Variable para manejar el pulso			
	struct can_object msg_can_in;			// Mensaje CAN de entrada
	struct timespec espera={0, TEMPO_CAN};  // Tiempo base para espera de mensajes CAN
//	struct timespec espera_proc={1, 0};
	/* Datos colas de mensajes - comunicacion proceso can2 con proceso ppal. de la ECU */
	mqd_t cola;
	struct mq_attr atrib_cola;
	int estado_cola = 0;
	short contador_timeout = 0;
	short cont_error_cola = 0;
	/* Variables para la funcion timeout */
//	struct timespec timeout={TEMPO_TIMEOUT_CAN, 0};

	
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
		printf("CAN2 channel error\n");
		//printf("CAN channel error\n");
		dato_cola.error_com_tcan = TRUE;
	}
	else
	{
		/* Si no hay error en la creacion del canal */
//		printf("No error creacion canal can2\n");
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
			printf("CAN2 connect channel error\n");
			/* Eliminamos el canal */
			ChannelDestroy(chid);
			//printf("CAN connect channel error\n");
			dato_cola.error_com_tcan = TRUE;
		}
		else
		{
			/* Si no hay error en la conexion del canal */
			contador_timeout = 0;
			/* Inicializacion de evento de notificacion */
			SIGEV_PULSE_INIT(&pulse_event, coid, getprio(0), _PULSE_CODE_MINAVAIL, 0);

			/* Arranca el driver CAN en el canal 2 */
			do {
				condriver = ConnectDriver(2, "CANDRV", &hdl);
				printf("Arranque CAN2: %d \n", condriver);
				/* Espera 0,5 segundos */
				nanosleep(&espera, NULL);
				contador_timeout++;
			} while ((condriver != ERR_OK) &&(contador_timeout <= TIEMPO_EXP_CAN_X));
			/* No continua hasta que no esta todo OK */
			if (contador_timeout > TIEMPO_EXP_CAN_X)
			{
				/* Error de arranque can */
				printf("CAN2 connect driver error\n");
				//printf("CAN connect driver error\n");
				dato_cola.error_com_tcan = TRUE;
			}
			else
			{
				/* Inicializacion de comunicacion CAN correcta */
				contador_timeout = 0;
				dato_cola.error_com_tcan = FALSE;
				/* Resetea el buffer CAN */
				CanRestart(hdl);
				/* Espera de pulsos: registra un pulso por cada lectura CAN */
			   	RegRdPulse(hdl, &pulse_event);
				/* Imprime el estado del driver CAN */
				printf ("\nCAN2 status: %04x\n", CanGetStatus(hdl, &st));
			}
		}
	}

	/**************************************************
	***		COLA DE MENSAJES		***
	**************************************************/
	/* Destruyo la cola en caso de haberla */
//	mq_unlink(NOMBRE_COLA);
	
	/* Configura los atributos para la cola */
	atrib_cola.mq_maxmsg = TAMANHO_COLA_CAN2;
	atrib_cola.mq_msgsize = sizeof(dato_cola_t);
	atrib_cola.mq_flags = 0;
	
	/* Creacion de la cola de mensajes */
	cola = mq_open(NOMBRE_COLA, O_RDWR|O_NONBLOCK, S_IRWXU, NULL);
//	perror("mq_open\n");
	printf("Creacion de cola, proceso can2. cola=%d - fin=%d\nerrno=%d\n", cola,  fin, errno);

	/* Esperamos a que el proceso principal se inicie */
//	nanosleep(&espera_proc, NULL);

	/**************************************************
	***	 LECTURA MENSAJES CAN Y ENVIO POR COLA	***
	**************************************************/
	while(!fin)
	{
//		printf("Esperando pulso\n");
		/* Establecemos el timer timeout */
//		timer_timeout(CLOCK_REALTIME, _NTO_TIMEOUT_RECEIVE, NULL, &timeout, NULL);
		/* Espera la recepcion del mensaje */
		rcvid = MsgReceivePulse_r(chid, &pulse, sizeof(struct _pulse), NULL);
//		printf("Estado recepcion de pulso: %d\n", rcvid);
		if(rcvid != 0)
		{
			printf("recv can2\n");	// Si hay error en la recepcion del pulso
			contador_timeout++;
			printf("Error recepcion pulso, contador: %d\n", contador_timeout);
			if (contador_timeout > 80)	// Para evitar que desborde el contador
				contador_timeout = 80;
		}
		else
		{
//			printf("Cola can2, funcionamiento correcto\n");
			/* Cuando llega un mensaje nuevo reiniciamos la cuenta de error */
			contador_timeout = 0;
			/* Lectura del siguiente mensaje CAN recibido */
			resp = CanRead(hdl, &msg_can_in, NULL);
			/* Lee los mensajes del can si la ID es correcta */
			if (msg_can_in.id == ID_CAN_BMS)
			{
				interpreta_can_entrada(&msg_can_in);
				/* Envio de mensajes en la cola */
				estado_cola = mq_send(cola, (char*)&dato_cola, sizeof(dato_cola_t), 0);
				if (estado_cola == -1)
				{
					cont_error_cola++;
					printf("Cola llena, contador: %d\n", (int)cont_error_cola);
				}
				else
					cont_error_cola = 0;	// Envio por cola correcto -> reseteamos contador				
			
				if (cont_error_cola > TIMEOUT_COLA)
				{
					/* Error de comunicacion de cola - terminamos el proceso */
					fin = 1;
					printf("Error comunicacion de cola, fin=%d\n", (int)fin);
				}		
			}
		}
		/* Error continuado de recepcion CAN desde la BMS o timed out en recepcion de pulso */
		if ((contador_timeout > TIEMPO_EXP_CAN_X) || (rcvid == ((-1)*ETIMEDOUT)))
		{
			printf("Error: contador=%d | rcvid=%d\n", (int)contador_timeout, rcvid);
			// Marcamos el error de comunicacion con BMS y lo enviamos el proceso ppal
			//dato_cola.error_com_bms = TRUE;
			// Envio de mensajes en la cola
			estado_cola = mq_send(cola, (char*)&dato_cola, sizeof(dato_cola_t), 0);
			if (estado_cola == -1)
			{
				printf("Cola llena can2 - envio error\n");
			}
			/* Terminamos el programa */
			fin = 1;
			printf("Error de com. can2: Terminando proceso can2...\n");
		}
	}
	
	/* Cierre ordenado del programa */
	DisConnectDriver(&hdl);		// Desconexion del driver CAN
	mq_close(cola);			// Cierre de la cola de mensajes
	mq_unlink(NOMBRE_COLA);		// Elimina cola de mensajes
	return(NULL);
}

/******************************************************************************************
***				DEFINICION FUNCION: interpreta_can_entrada		***
*** Identifica el msg CAN recibido y lo procesa en funcion del tipo de mensaje y del	***
*** identificador CAN.									***
*** Parametros:										***
*** - struct can_object *p_can: Puntero a la estructura CAN recibida.			***
******************************************************************************************/
void interpreta_can_entrada(struct can_object *p_can)
{
	/* Variables auxiliares */
//	char c_aux_param = '\0';
	char c_aux_index[NUM_MAX_DIG_INDEX+3];
	char c_aux_value[NUM_MAX_DIG_VALUE+3];
	int i = 0;
	int index = 0;
	char c_param;
	int param = 0;
	int value = 0;
	
	/* Inicializacion */
	c_aux_index[0] = '0';
	c_aux_index[1] = 'x';
	c_aux_index[NUM_MAX_DIG_INDEX+2] = '\0';
	c_aux_value[0] = '0';
	c_aux_value[1] = 'x';
	c_aux_value[NUM_MAX_DIG_VALUE+2] = '\0';
	
	/* Leemos el tipo de mensaje recibido */
	for (i=0; i<NUM_MAX_DIG_INDEX;i++)
		c_aux_index[i+2] = (char)p_can->data[i];
//	printf("Tipo de mensaje recibido: %s\n", c_aux_index);
	/* Lo convertimos a formato numerico */
	index = (int)strtol(c_aux_index, NULL, 0);
//	printf("Index=%d\n", index);
	/* Leemos el parametro en ASCII que define el tipo de mensaje */
	c_param = p_can->data[2];
	param = c_param;
//	printf("Parametro recibido: char: %c, int: %d\n", c_param, param);
	/* Leemos el valor del dato del mensaje */
	for (i=3; i<(NUM_MAX_DIG_VALUE+3);i++)
		c_aux_value[i-1] = (char)p_can->data[i];
//	printf("Valor del dato del mensaje: %s\n", c_aux_value);
	/* Lo convertimos a formato numerico */
	value = (int)strtol(c_aux_value, NULL, 0);
//	printf("Value=%d\n", value);
	
	/* Filtramos los mensajes CAN recibidos por tipo de mensaje */
	switch(param)
	{
		/* Primero filtramos el tipo de mensaje por el parametro */
		case VOLTAJE_T:
			if((index > 0) && (index <= NUM_CEL_BAT))
				dato_cola.mv_cel[index-1]=(uint16_t)value;
		break;
		case TEMPERATURA_T:
			if((index > 0) && (index <= NUM_CEL_BAT))
				dato_cola.temp_cel[index-1]=(uint8_t)value;
		break;
		case ESTADO_T:
			switch(index)
			{
				case 0:
					// No nos interesa
				break;
				case 1:
					// No nos interesa
				break;
				case 2:
					dato_cola.num_cel_scan = (uint8_t)value;
				break;
				case 3:
					dato_cola.temp_media = (uint8_t)value;
				break;
				case 4:
					dato_cola.v_medio = (uint16_t)value;
				break;
				case 5:
					dato_cola.temp_max = (uint8_t)value;
				break;
				case 6:
					dato_cola.cel_temp_max = (uint8_t)value;
				break;
				case 7:
					dato_cola.v_max = (uint16_t)value;
				break;
				case 8:
					dato_cola.cel_v_max = (uint8_t)value;
				break;
				case 9:
					dato_cola.v_min = (uint16_t)value;
				break;
				case 10:
					dato_cola.cel_v_min = (uint8_t)value;
				break;
				case 11:
					dato_cola.v_pack = (uint32_t)value;
				break;
				case 12:
					dato_cola.i_pack = (int16_t)value;
				break;
				case 13:
					dato_cola.soc = (uint8_t)value;
				break;
				case 15:
					dato_cola.timestamp = (uint16_t)value;
				break;
				default:
				//	printf("No se reconoce el mensaje de estado de la BMS.\n");

			}
		break;
		case ALARMA_T:
			switch(index)
			{
				case 0:
					//printf("Numero de alarmas = %d\n", value);
				break;
				case 1:
					dato_cola.nivel_alarma = (uint8_t) NO_ALARMA;
					dato_cola.alarma = (uint8_t) PACK_OK;
				break;
				case 2:
					if(value!=0)
					{
						dato_cola.nivel_alarma = (uint8_t) ALARMA;
						dato_cola.alarma = (uint8_t) CELL_TEMP_HIGH;
					}
				break;
				case 3:
					if(value!=0)
					{
						dato_cola.nivel_alarma = (uint8_t) ALARMA;
						dato_cola.alarma = (uint8_t) PACK_V_HIGH;
					}
				break;
				case 4:
					if(value!=0)
					{
						dato_cola.nivel_alarma = (uint8_t) WARNING;
						dato_cola.alarma = (uint8_t) PACK_V_HIGH;
					}
				break;
				case 5:
					if(value!=0)
					{
						dato_cola.nivel_alarma = (uint8_t) ALARMA;
						dato_cola.alarma = (uint8_t) PACK_V_LOW;
					}
				break;
				case 6:
					if(value!=0)
					{
						dato_cola.nivel_alarma = (uint8_t) WARNING;
						dato_cola.alarma = (uint8_t) PACK_V_LOW;
					}
				break;
				case 7:
					if(value!=0)
					{
						dato_cola.nivel_alarma = (uint8_t) ALARMA;
						dato_cola.alarma = (uint8_t) PACK_I_HIGH;
					}
				break;
				case 8:
					if(value!=0)
					{
						dato_cola.nivel_alarma = (uint8_t) WARNING;
						dato_cola.alarma = (uint8_t) PACK_I_HIGH;
					}
				break;
				case 9:
					if(value!=0)
					{
						dato_cola.nivel_alarma = (uint8_t) ALARMA_CRITICA;
						dato_cola.alarma = (uint8_t) CHASIS_CONECT;
					}
				break;
				case 10:
					if(value!=0)
					{
						dato_cola.nivel_alarma = (uint8_t) ALARMA_CRITICA;
						dato_cola.alarma = (uint8_t) CELL_COM_ERROR;
					}
				break;
				case 11:
					if(value!=0)
					{
						dato_cola.nivel_alarma = (uint8_t) ALARMA_CRITICA;
						dato_cola.alarma = (uint8_t) SYSTEM_ERROR;
					}
				break;
				case 12:
					//printf("Mensaje alarma BMS numero %d\n", index);
				break;
				default:
				//	printf("No se reconoce el mensaje de alarma de la BMS.\n");
			}
		break;
		default:
			printf("No se reconoce el mensaje de la BMS.\n");
	}
}
