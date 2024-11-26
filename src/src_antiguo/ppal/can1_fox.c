/**********************************************************************************
***										***
***	Fichero: can1_fox.c							***
***	Fecha: 03/10/2013							***
***	Autor: Elena Gonzalez							***
***	Descripción: Hilo de adquisicion de datos por CAN para motores.		***
***	Recogida de datos CAN de los motores desde buffer de hilo canRX_fox.c	***
***	Fragmentos de codigo de Alejandro Oliva Torres.				***
***										***
**********************************************************************************/

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
#include <string.h>
/* Can includes */
#include "./include/candef.h"
#include "./include/canstr.h"
#include "./include/canglob.h"
/* Include local */
#include "./include/constantes_fox.h"
#include "./include/estructuras_fox.h"
#include "./include/declaraciones_fox.h"

/**************************************************
***		 VARIABLES GLOBALES		***
**************************************************/
extern uint16_t hilos_listos;
/* Estructuras globales */
extern est_error_t errores;
extern est_motor_t motor1;	// Almacena los datos del motor 1 - Trasero izquierdo
extern est_motor_t motor2;	// Almacena los datos del motor 2 - Delantero izquierdo
extern est_motor_t motor3;	// Almacena los datos del motor 3 - Trasero derecho
extern est_motor_t motor4;	// Almacena los datos del motor 4 - Delantero derecho

/* Buffers CAN */
extern struct can_object buff_rx_mot1;	// Buffer de recepcion de mensajes procedientes del motor 1
extern struct can_object buff_rx_mot2;	// Buffer de recepcion de mensajes procedientes del motor 2
extern struct can_object buff_rx_mot3;	// Buffer de recepcion de mensajes procedientes del motor 3
extern struct can_object buff_rx_mot4;	// Buffer de recepcion de mensajes procedientes del motor 4

/* Mutex globales */
extern pthread_mutex_t mut_errores;
extern pthread_mutex_t mut_motor1;
extern pthread_mutex_t mut_motor2;
extern pthread_mutex_t mut_motor3;
extern pthread_mutex_t mut_motor4;
extern pthread_mutex_t mut_rx_mot1;
extern pthread_mutex_t mut_rx_mot2;
extern pthread_mutex_t mut_rx_mot3;
extern pthread_mutex_t mut_rx_mot4;
extern pthread_mutex_t mut_hay_rx_mot1;
extern pthread_mutex_t mut_hay_rx_mot2;
extern pthread_mutex_t mut_hay_rx_mot3;
extern pthread_mutex_t mut_hay_rx_mot4;
extern pthread_cond_t hay_rx_mot1;
extern pthread_cond_t hay_rx_mot2;
extern pthread_cond_t hay_rx_mot3;
extern pthread_cond_t hay_rx_mot4;
extern pthread_mutex_t mut_hilos_listos;
extern pthread_mutex_t mut_inicio;
extern pthread_cond_t inicio;
/* Bloquea el handler de tx CAN */
extern pthread_mutex_t mut_hdl;
/* Identificador de la conexion CAN */
extern canhdl_t hdl;
/* Indica que todos los hilos se estan ejecutando */

/******************************************************************************************************
***				FUNCION PRINCIPAL: canMotor (hilo)													***
*** 	Gestiona el bus CAN para el controlador del motor n.										***
***	Envia los mensajes de peticion al controlador n y los recibe secuencialmente desde el buffer.	***
*******************************************************************************************************/
void *canMotor (void *cm)
{
	/**************************************************
	***		VARIABLES								***
	**************************************************/
	struct timespec tiempo_exp_resp;
	struct can_object rxmsg;
	int cont_intentos = 0;
	int retval = 0;
	BOOL er_com_motor = FALSE;
	BOOL fin_local = 0;
	uint16_t cont_bucle = 0;
	int tipo_mens;
	int numMotor;
	struct timespec pausa_can_motores = {0, TIEMPO_BUCLE_NS};
	
	uint16_t h_listo_local;
	int id_motor_local;
	pthread_mutex_t *pmut_hay_rx_mot;
	pthread_cond_t *phay_rx_mot;
	uint16_t er_grave_local;
	uint16_t er_critico_local;
	pthread_mutex_t *pmut_rx_mot;
	struct can_object *buff_rx_local;

	numMotor = *((int *) cm);
	
	switch (numMotor)
	{
		case POS_M1:
			h_listo_local = HILO_MOT1_LISTO;
			id_motor_local = ID_CTRL_MOTOR_1_ECU_C;
			pmut_hay_rx_mot = &mut_hay_rx_mot1;
			phay_rx_mot = &hay_rx_mot1;
			er_grave_local = ERG_ECU_COM_M1;
			er_critico_local = ERC_ECU_COM_M1;
			pmut_rx_mot = &mut_rx_mot1;
			buff_rx_local = &buff_rx_mot1;
		break;
		case POS_M2:
			h_listo_local = HILO_MOT2_LISTO;
			id_motor_local = ID_CTRL_MOTOR_2_ECU_C;
			pmut_hay_rx_mot = &mut_hay_rx_mot2;
			phay_rx_mot = &hay_rx_mot2;
			er_grave_local = ERG_ECU_COM_M2;
			er_critico_local = ERC_ECU_COM_M2;
			pmut_rx_mot = &mut_rx_mot2;
			buff_rx_local = &buff_rx_mot2;
		break;
		case POS_M3:
			h_listo_local = HILO_MOT3_LISTO;
			id_motor_local = ID_CTRL_MOTOR_3_ECU_C;
			pmut_hay_rx_mot = &mut_hay_rx_mot3;
			phay_rx_mot = &hay_rx_mot3;
			er_grave_local = ERG_ECU_COM_M3;
			er_critico_local = ERC_ECU_COM_M3;
			pmut_rx_mot = &mut_rx_mot3;
			buff_rx_local = &buff_rx_mot3;
		break;
		case POS_M4:
			h_listo_local = HILO_MOT4_LISTO;
			id_motor_local = ID_CTRL_MOTOR_4_ECU_C;
			pmut_hay_rx_mot = &mut_hay_rx_mot4;
			phay_rx_mot = &hay_rx_mot4;
			er_grave_local = ERG_ECU_COM_M4;
			er_critico_local = ERC_ECU_COM_M4;
			pmut_rx_mot = &mut_rx_mot4;
			buff_rx_local = &buff_rx_mot4;
		break;
		default:
			printf("El zorro solo tiene 4 patas.\n");
	}
	
	/* Indica que el hilo de comunicacion CAN del Motor esta listo */
	pthread_mutex_lock(&mut_hilos_listos);
	hilos_listos |= h_listo_local;
	pthread_mutex_unlock(&mut_hilos_listos);
	printf("Hilo CAN Motor %d listo\n",numMotor+1);
	pthread_mutex_lock(&mut_inicio);
	pthread_cond_wait(&inicio, &mut_inicio);
	pthread_mutex_unlock(&mut_inicio);

	while(!fin_local)
	{
		for (tipo_mens = MSG_TIPO_01; tipo_mens < (MSG_TIPO_13+1); tipo_mens++)
		{
			if ((tipo_mens <= MSG_TIPO_02)&&(cont_bucle % 200 != 0))
				continue;			
			if ((tipo_mens > MSG_TIPO_02)&&(tipo_mens <= MSG_TIPO_06)&&(cont_bucle % 10 != 0))
				continue;
			do {
				/* Envia mensaje Tipo n */
				envia_can(tipo_mens, id_motor_local);
		//		printf("Enviados mensajes tipo %d a motor %d\n",tipo_mens,numMotor+1);
				/* Espera la respuesta */
		//		printf("Esperando respuesta tipo %d de motor %d\n",tipo_mens,numMotor+1);
				clock_gettime(CLOCK_REALTIME, &tiempo_exp_resp);
				tiempo_exp_resp.tv_sec += TIEMPO_EXP_MOT_S;
				pthread_mutex_lock(pmut_hay_rx_mot);
				retval = pthread_cond_timedwait(phay_rx_mot, pmut_hay_rx_mot, &tiempo_exp_resp);
				pthread_mutex_unlock(pmut_hay_rx_mot);
				if (retval  == ETIMEDOUT)
					cont_intentos++;
				/* Si hay error grave por falta de mensaje del motor */
				if (cont_intentos >= LIMITE_CONT_MOTOR_G)
				{
					pthread_mutex_lock(&mut_errores);
					errores.er_grave_1 |= er_grave_local;
					pthread_mutex_unlock(&mut_errores);
				}
				/* Si hay error critico por falta de mensaje del motor */
				if (cont_intentos > LIMITE_CONT_MOTOR_C)
				{
					er_com_motor = TRUE;
					pthread_mutex_lock(&mut_errores);
					errores.er_critico_1 |= er_critico_local;
					errores.er_grave_1 &= ~er_grave_local;
					pthread_mutex_unlock(&mut_errores);
				}
			} while ((retval == ETIMEDOUT) && (er_com_motor == FALSE));
			/* Si hemos recibido el mensaje y no tenemos error critico */
			if ((retval != ETIMEDOUT) && (er_com_motor == FALSE))
			{
				cont_intentos = 0;
				/* Eliminamos los errores de comunicacion con los motores */
				pthread_mutex_lock(&mut_errores);
				errores.er_critico_1 &= ~er_critico_local;
				errores.er_grave_1 &= ~er_grave_local;
				pthread_mutex_unlock(&mut_errores);
				
				/* Leemos el mensaje del buffer */
				pthread_mutex_lock(pmut_rx_mot);
				rxmsg = *buff_rx_local;
				pthread_mutex_unlock(pmut_rx_mot);
				/* Interpreta el mensaje CAN, almacenando cada campo rx en la variable correspondiente */
				interpreta_can_entrada(tipo_mens, &rxmsg);
			}
			
			nanosleep(&pausa_can_motores, NULL);
		}
		
		pthread_mutex_lock(&mut_hilos_listos);
		if ((hilos_listos&h_listo_local)==0x0000)
			fin_local = 1;
		pthread_mutex_unlock(&mut_hilos_listos);
		
		cont_bucle ++;
	}
	
	/* Cierre ordenado del hilo */
	pthread_exit(NULL);
	return(NULL);	
}


/******************************************************************************************
***			DEFINICION FUNCION: envia_can					***
*** Envia a los controladores de motores los mensajes CAN correspondientes a la 	***
*** peticion de cada tipo de mensaje.							***
*** Parametros:										***
***	- int tipo_msg: contiene un num. de 0 a 10 que identifica el tipo de mensaje	***
***			de los controladores de los motores.				***
***	- int id_can: contiene el identificador CAN para el mensaje a enviar.		***
******************************************************************************************/
int envia_can(int tipo_msg, int id_can)
{
	/* Mensajes can de salida */
	struct can_object msg_can_out;
	/* Recoge el valor devuelto por CanWrite */
	short dev_can = 0;
	/* Estatus del driver CAN */
//	struct status st;
	
	memset(&msg_can_out, 0, sizeof(struct can_object));
	
	/* Configuracion de la trama */
	msg_can_out.frame_inf.inf.FF = StdFF;	// Modo estandar
	msg_can_out.frame_inf.inf.RTR = 0;		// Trama de datos

//	printf("Funcion de envio CAN: %d\n", tipo_msg);
		
	/* Preparacion y envio de mensajes segun tipo */
	msg_can_out.id = id_can;			// Identificador CAN de salida
	
	switch(tipo_msg)
	{
		/* TIPO 1 */
		case MSG_TIPO_01:
			msg_can_out.frame_inf.inf.DLC =	3;		// Longitud de datos en bytes
			msg_can_out.data[0] = (BYTE)(CCP_FLASH_READ);	// Datos del mensaje de salida
			msg_can_out.data[1] = (BYTE)(INFO_MODULE_NAME);
			msg_can_out.data[2] = (BYTE)(0x08);
		break;
		/* TIPO 2 */
		case MSG_TIPO_02:
			msg_can_out.frame_inf.inf.DLC =	3;		// Longitud de datos en bytes
			msg_can_out.data[0] = (BYTE)(CCP_FLASH_READ);	// Datos del mensaje de salida	
			msg_can_out.data[1] = (BYTE)(INFO_SOFTWARE_VER);
			msg_can_out.data[2] = (BYTE)(0x02);
		break;
		/* TIPO 3 */
		case MSG_TIPO_03:
			msg_can_out.frame_inf.inf.DLC =	3;		// Longitud de datos en bytes
			msg_can_out.data[0] = (BYTE)(CCP_FLASH_READ);	// Datos del mensaje de salida	
			msg_can_out.data[1] = (BYTE)(CAL_TPS_DEAD_ZONE_LOW);
			msg_can_out.data[2] = (BYTE)(0x01);
		break;
		/* TIPO 4 */
		case MSG_TIPO_04:
			msg_can_out.frame_inf.inf.DLC =	3;		// Longitud de datos en bytes
			msg_can_out.data[0] = (BYTE)(CCP_FLASH_READ);	// Datos del mensaje de salida	
			msg_can_out.data[1] = (BYTE)(CAL_TPS_DEAD_ZONE_HIGH);
			msg_can_out.data[2] = (BYTE)(0x01);
		break;
		/* TIPO 5 */
		case MSG_TIPO_05:
			msg_can_out.frame_inf.inf.DLC =	3;		// Longitud de datos en bytes
			msg_can_out.data[0] = (BYTE)(CCP_FLASH_READ);	// Datos del mensaje de salida	
			msg_can_out.data[1] = (BYTE)(CAL_BRAKE_DEAD_ZONE_LOW);
			msg_can_out.data[2] = (BYTE)(0x01);
		break;
		/* TIPO 6 */
		case MSG_TIPO_06:
			msg_can_out.frame_inf.inf.DLC =	3;		// Longitud de datos en bytes
			msg_can_out.data[0] = (BYTE)(CCP_FLASH_READ);	// Datos del mensaje de salida	
			msg_can_out.data[1] = (BYTE)(CAL_BRAKE_DEAD_ZONE_HIGH);
			msg_can_out.data[2] = (BYTE)(0x01);
		break;
		/* TIPO 7 */
		case MSG_TIPO_07:
			msg_can_out.frame_inf.inf.DLC =	1;		// Longitud de datos en bytes
			msg_can_out.data[0] = (BYTE)(CCP_A2D_BATCH_READ1);	// Datos del mensaje de salida	
		break;
		/* TIPO 8 */
		case MSG_TIPO_08:
			msg_can_out.frame_inf.inf.DLC =	1;		// Longitud de datos en bytes
			msg_can_out.data[0] = (BYTE)(CCP_A2D_BATCH_READ2);	// Datos del mensaje de salida	
		break;
		/* TIPO 9 */
		case MSG_TIPO_09:
			msg_can_out.frame_inf.inf.DLC =	1;		// Longitud de datos en bytes
			msg_can_out.data[0] = (BYTE)(CCP_MONITOR1);	// Datos del mensaje de salida	
		break;
		/* TIPO 10 */
		case MSG_TIPO_10:
			msg_can_out.frame_inf.inf.DLC =	1;		// Longitud de datos en bytes
			msg_can_out.data[0] = (BYTE)(CCP_MONITOR2);	// Datos del mensaje de salida	
		break;
		case MSG_TIPO_11:
			msg_can_out.frame_inf.inf.DLC =	2;
			msg_can_out.data[0] = (BYTE) COM_SW_ACC;
			msg_can_out.data[1] = (BYTE) COM_READING;			
		break;
		case MSG_TIPO_12:
			msg_can_out.frame_inf.inf.DLC =	2;
			msg_can_out.data[0] = (BYTE) COM_SW_BRK;
			msg_can_out.data[1] = (BYTE) COM_READING;	
		break;
		case MSG_TIPO_13:
			msg_can_out.frame_inf.inf.DLC =	2;
			msg_can_out.data[0] = (BYTE) COM_SW_REV;
			msg_can_out.data[1] = (BYTE) COM_READING;			
		break;
		default:
			printf("Error. Tipo de mensaje CAN desconocido.\n");
			return (-1);
	}	// fin SWITCH
	
	pthread_mutex_lock(&mut_hdl);
	dev_can = CanWrite(hdl, &msg_can_out);
	pthread_mutex_unlock(&mut_hdl);
	
	return(0);
}

/******************************************************************************************
***				DEFINICION FUNCION: interpreta_can_entrada		***
*** Identifica el msg CAN recibido y lo procesa en funcion del tipo de mensaje y del	*** 
*** identificador CAN.									***
*** Parametros:										***
*** - int tipo_msg: Indica el tipo de mensaje que se espera recibir (num. de 1 a 10). 	***
*** - struct can_object *p_can: Puntero a la estructura CAN recibida.			***	
******************************************************************************************/
int interpreta_can_entrada(int tipo_msg, struct can_object *p_can)
{
	char buffer1[6], buffer2[6];
	int i;
	
	pthread_mutex_t *pmut_motor;
	est_motor_t *motor_local;
	int num_motor;
	
	switch(p_can->id)
	{
		case ID_CTRL_MOTOR_1_C_ECU:
			pmut_motor = &mut_motor1;
			motor_local = &motor1;
			num_motor = POS_M1+1; //para los printf
		break;
		case ID_CTRL_MOTOR_2_C_ECU:
			pmut_motor = &mut_motor2;
			motor_local = &motor2;
			num_motor = POS_M2+1;
		break;
		case ID_CTRL_MOTOR_3_C_ECU:
			pmut_motor = &mut_motor3;
			motor_local = &motor3;
			num_motor = POS_M3+1;			
		break;
		case ID_CTRL_MOTOR_4_C_ECU:
			pmut_motor = &mut_motor4;
			motor_local = &motor4;
			num_motor = POS_M4+1;			
		break;
		default:
			printf("No se reconoce de que motor proviene el mensaje.\n");
			return(-1);
	}
	
	if ((p_can->data[0])==CCP_INVALID_COMMAND)
	{
		printf("Respuesta de mensaje invalido.\n");
		return (-1);
	}

	switch (tipo_msg)
	{
		case MSG_TIPO_01:
			/* Recoge el modelo del controlador */
			pthread_mutex_lock(pmut_motor);
			for (i=0; i<8; i++)
				motor_local->modelo[i] = (char) p_can->data[i];
			motor_local->modelo[8] = '\0';
			pthread_mutex_unlock(pmut_motor);
			/* imprime el modelo del controlador por pantalla */
	//		printf("Modelo controlador %d: %s\n", num_motor, motor_local->modelo);
		break;
		case MSG_TIPO_02:
			/* Recoge la version del controlador */
			itoa((int)(p_can->data[0]), buffer1, 10);
			itoa((int)(p_can->data[1]), buffer2, 10);
			strcat(buffer1, buffer2);
			pthread_mutex_lock(pmut_motor);
			motor_local->version = (short) atoi(buffer1);
			pthread_mutex_unlock(pmut_motor);
			/* Comprueba que controlador es e imprime la version software del controlador por pantalla */
	//		printf("Version Software del controlador %d: %d\n", num_motor, motor_local->version);
		break;
		case MSG_TIPO_03:
			/* Recoge el valor de zona muerta inferior del acelerador */
			pthread_mutex_lock(pmut_motor);
			motor_local->zm_inf_acel = (BYTE) (p_can->data[0])/2;
			pthread_mutex_unlock(pmut_motor);
		break;
		case MSG_TIPO_04:
			/* Recoge el valor de zona muerta superior del acelerador */
			pthread_mutex_lock(pmut_motor);
			motor_local->zm_sup_acel = (BYTE) (p_can->data[0])/2;
			pthread_mutex_unlock(pmut_motor);
		break;
		case MSG_TIPO_05:
			/* Recoge el valor de zona muerta inferior del freno */
			pthread_mutex_lock(pmut_motor);
			motor_local->zm_inf_fren = (BYTE) p_can->data[0];
			pthread_mutex_unlock(pmut_motor);
		break;
		case MSG_TIPO_06:
			/* Recoge el valor de zona muerta superior del freno */
			pthread_mutex_lock(pmut_motor);
			motor_local->zm_sup_fren = (BYTE) p_can->data[0];
			pthread_mutex_unlock(pmut_motor);
		break;
		case MSG_TIPO_07:
			/* Carga los datos en la estructura a enviar al supervisor */
			pthread_mutex_lock(pmut_motor);
			motor_local->freno = (float) (p_can->data[0])/51;
			motor_local->acel = (float) (p_can->data[1])/51;
			motor_local->v_pot = (float) (p_can->data[2])/2.71;
			motor_local->v_aux = (float) (p_can->data[3])/28 + 0.4643;
			motor_local->v_bat = (float) (p_can->data[4])/2.71;
			pthread_mutex_unlock(pmut_motor);
		break;
		case MSG_TIPO_08:
			/* Recoge los valores de conversion A/D 2: V e I de las 3 fases del motor */
			pthread_mutex_lock(pmut_motor);
			motor_local->i_a = p_can->data[0];
			motor_local->i_b = p_can->data[1];
			motor_local->i_c = p_can->data[2];
			motor_local->v_a = (float) (p_can->data[3])/2.71;
			motor_local->v_b = (float) (p_can->data[4])/2.71;
			motor_local->v_c = (float) (p_can->data[5])/2.71;
			pthread_mutex_unlock(pmut_motor);
		break;
		case MSG_TIPO_09:
			/* Recoge datos: PWM, temperaturas... */
			/* Carga los datos en la estructura a enviar al supervisor */
			pthread_mutex_lock(pmut_motor);
			motor_local->pwm = p_can->data[0];
			motor_local->en_motor_rot = (BOOL) p_can->data[1];
			motor_local->temp_motor = p_can->data[2];
			motor_local->temp_int_ctrl = p_can->data[3];
			motor_local->temp_sup_ctrl = p_can->data[4];
			motor_local->temp_inf_ctrl = p_can->data[5];
			pthread_mutex_unlock(pmut_motor);	
		break;
		case MSG_TIPO_10:
			/* Recoge dato de RPM e I actual en porcentaje */
			pthread_mutex_lock(pmut_motor);
			motor_local->rpm = (((uint16_t) p_can->data[1]) & MASK_BYTE_BAJO)
								|((((uint16_t) p_can->data[0])<<8) & MASK_BYTE_ALTO);
			motor_local->i_porcent = p_can->data[2];
			pthread_mutex_unlock(pmut_motor);
		break;
		case MSG_TIPO_11:
			pthread_mutex_lock(pmut_motor);		
			motor_local->acel_switch = (BOOL) p_can->data[0];
			pthread_mutex_unlock(pmut_motor);
		break;
		case MSG_TIPO_12:
			pthread_mutex_lock(pmut_motor);		
			motor_local->freno_switch = (BOOL) p_can->data[0];
			pthread_mutex_unlock(pmut_motor);
		break;
		case MSG_TIPO_13:
			pthread_mutex_lock(pmut_motor);		
			motor_local->reverse_switch = (BOOL) p_can->data[0];
			pthread_mutex_unlock(pmut_motor);
		break;
		default:
			printf("Tipo de mensaje no reconocido: no se puede recibir el mensaje.\n");
			return(-1);
	}
	return(0);
}
