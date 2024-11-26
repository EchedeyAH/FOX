/**************************************************************************
***									***
***	Fichero: err_fox.c						***
***	Fecha: 10/10/2013						***
***	Autor: Elena Gonzalez						***
***	Descripcion: Fichero de errores de la ECU. 			***
***	Se encarga de gestionar los errores del programa.		***
***																	***
*****************************************************************************/

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

/**************************************************
***		VARIABLES GLOBALES		***
**************************************************/
/* Variables globales */
extern uint16_t hilos_listos;

/* Estructuras globales */
extern est_error_t errores;
extern est_bat_t bateria;
extern est_motor_t motor1;
extern est_motor_t motor2;
extern est_motor_t motor3;
extern est_motor_t motor4;
extern est_veh_t vehiculo;
extern est_pot_t potencias;

/* MUTEX */
extern pthread_mutex_t mut_motor1;
extern pthread_mutex_t mut_motor2;
extern pthread_mutex_t mut_motor3;
extern pthread_mutex_t mut_motor4;
extern pthread_mutex_t mut_errores;
extern pthread_mutex_t mut_bateria;
extern pthread_mutex_t mut_vehiculo;
extern pthread_mutex_t mut_potencias;
extern pthread_mutex_t mut_inicio;
extern pthread_mutex_t mut_hilos_listos;
extern pthread_cond_t inicio;

void *errores_main(void *pi)
{
	/* Variables locales */
	char fin_local = 0;
	struct timespec espera = {0, TIEMPO_BUCLE_NS};
	uint8_t error_leve_local = 0;
	uint8_t error_grave_local = 0;
	uint8_t error_critico_local = 0;
	
	/* Indica que el hilo de errores esta listo */
	pthread_mutex_lock(&mut_hilos_listos);
	hilos_listos |= HILO_ERR_LISTO;
	pthread_mutex_unlock(&mut_hilos_listos);
	printf("Hilo Errores listo\n");
	pthread_mutex_lock(&mut_inicio);
	pthread_cond_wait(&inicio, &mut_inicio);
	pthread_mutex_unlock(&mut_inicio);
	
	while (!fin_local)
	{
		/* Cada funcion comprueba los errores por componentes */
//		vehiculo_errores();
		motor_errores(POS_M1);
		motor_errores(POS_M2);
		motor_errores(POS_M3);
		motor_errores(POS_M4);
		bms_errores();
		sensores_errores();
		// fc_errores();
		
		/* Comprueba los niveles de error para activar los flags correspondientes */
		pthread_mutex_lock(&mut_errores);
		/* ERRORES LEVES */
		if ((errores.er_leve_1 != 0) || (errores.er_leve_2 != 0))
			errores.error_leve = TRUE;
		else
			errores.error_leve = FALSE;
		/* ERRORES GRAVES */
		if ((errores.er_grave_1 != 0) || (errores.er_grave_2 != 0))
			errores.error_grave = TRUE;
		else
			errores.error_grave = FALSE;
		/* ERRORES CRITICOS */
		if (errores.er_critico_1 != 0)
			errores.error_critico = TRUE;
		else
			errores.error_critico = FALSE;
		pthread_mutex_unlock(&mut_errores);
		
		/* Cargamos los valores de los flags globales en variables locales */
		pthread_mutex_lock(&mut_errores);
		error_leve_local = errores.error_leve;
		error_grave_local = errores.error_grave;
		error_critico_local = errores.error_critico;
		pthread_mutex_unlock(&mut_errores);
		
		/* Actuamos en funcion de los errores */
	//	if (error_critico_local == TRUE) actuacion_critica();
	//	if (error_grave_local == TRUE) actuacion_grave();
	//	if (error_leve_local == TRUE) actuacion_leve();
		
		/* Espera del bucle */
		nanosleep(&espera, NULL);
		
		pthread_mutex_lock(&mut_hilos_listos);
		if ((hilos_listos&HILO_ERR_LISTO)==0x0000)
			fin_local = 1;
		pthread_mutex_unlock(&mut_hilos_listos);
		
	} // Fin bucle while
	
	
	pthread_exit (NULL);
	return(NULL);
}

void actuacion_critica()
{
	uint16_t er_critico_1_local;

	/* Cargamos los valores en la variable local */
	pthread_mutex_lock(&mut_errores);
	er_critico_1_local = errores.er_critico_1;
	pthread_mutex_unlock(&mut_errores);
	
	/* Si hay algun error de comunicacion de la ECU con sus tarjetas */
	if ((er_critico_1_local  & ERC_ECU_T_MASK) != 0)
	{
		/* Termina todos los hilos */
//		perror("Error critico: Error comunicacion con tarjetas internas ECU.\n");
		printf("Error critico: Error comunicacion con tarjetas internas ECU.\n");
		pthread_mutex_lock(&mut_hilos_listos);
//		hilos_listos = 0x0000;
		pthread_mutex_unlock(&mut_hilos_listos);
	}
	
	/* Si hay algun error de comunicacion de la ECU con los motores */
	if ((er_critico_1_local  & ERC_ECU_M_MASK) != 0)
	{
		/* Termina todos los hilos */
//		perror("Error critico: Error comunicacion con motores.\n");
		printf("Error critico: Error comunicacion con motores.\n");
		pthread_mutex_lock(&mut_hilos_listos);
//		hilos_listos = 0x0000;
		pthread_mutex_unlock(&mut_hilos_listos);
	}
	
	/* Si hay algun error critico en las baterias */
	if ((er_critico_1_local  & ERC_BMS_MASK) != 0)
	{
		/* Termina todos los hilos */
//		perror("Error critico: Error critico reportado por BMS.\n");
		printf("Error critico: Error critico reportado por BMS.\n");
		pthread_mutex_lock(&mut_hilos_listos);
//		hilos_listos = 0x0000;
		pthread_mutex_unlock(&mut_hilos_listos);
	}
	/* Si hay algun error critico en arranque de hilos */
	if ((er_critico_1_local  & ERC_HILOS_MASK) != 0)
	{
		/* Termina todos los hilos */
//		perror("Error critico: Error critico en arranque de hilos.\n");
		printf("Error critico: Error critico en arranque de hilos.\n");	
		pthread_mutex_lock(&mut_hilos_listos);
//		hilos_listos = 0x0000;
		pthread_mutex_unlock(&mut_hilos_listos);
	}
	
}
void actuacion_grave()
{
	int i = 0;
	uint16_t er_grave_1_local;
	uint16_t er_grave_2_local;
	
	/* Cargamos los valores en la variable local */
	pthread_mutex_lock(&mut_errores);
	er_grave_1_local = errores.er_grave_1;
	er_grave_2_local = errores.er_grave_2;	
	pthread_mutex_unlock(&mut_errores);
	
	/* Si hay error grave */
	if ((er_grave_1_local > 0) || (er_grave_2_local > 0))
	{
//		printf("Error grave, acelerador a 0\n");
		/* Anulamos el acelerador */
		pthread_mutex_lock(&mut_potencias);
		for (i=0; i<NUM_MOTORES; i++)
		{
			potencias.acelerador[i] = 0.0;
			potencias.freno[i] = 0.0;
		}
		pthread_mutex_unlock(&mut_potencias);
	}
	
}
void actuacion_leve()
{
	uint16_t er_leve_1_local;
	uint16_t er_leve_2_local;
	
	/* Cargamos los valores en la variable local */
	pthread_mutex_lock(&mut_errores);
	er_leve_1_local = errores.er_leve_1;
	er_leve_2_local = errores.er_leve_2;
	pthread_mutex_unlock(&mut_errores);
	
}

void bms_errores()
{
	/* Variables */
	uint8_t nivel_alarma_local = 0;
	uint8_t alarma_local = 0;
	uint8_t error_com_tcan_local = 0;
	uint8_t error_com_bms_local = 0;

	/* Actualizacion de variables locales desde globales */
	pthread_mutex_lock(&mut_bateria);
	nivel_alarma_local = bateria.nivel_alarma;
	alarma_local = bateria.alarma;
	error_com_tcan_local = bateria.error_com_tcan;
	error_com_bms_local = bateria.error_com_bms;
	pthread_mutex_unlock(&mut_bateria);

	/* Comprobacion de errores uno a uno */
	if(error_com_tcan_local == TRUE)
	{
		pthread_mutex_lock(&mut_errores);
		errores.er_critico_1 |= ERC_ECU_COM_TCAN2;
		pthread_mutex_unlock(&mut_errores);
	}
	if(error_com_bms_local)
	{
		pthread_mutex_lock(&mut_errores);
		errores.er_critico_1 |= ERC_ECU_COM_BMS;
		pthread_mutex_unlock(&mut_errores);
	}
	pthread_mutex_lock(&mut_errores);
	switch (nivel_alarma_local)
	{
		/* No error de BMS - Eliminamos todos los errores de BMS */
		case NO_ALARMA:
			errores.er_leve_1 &= ~ERL_BMS_PVOLT_H;
			errores.er_leve_1 &= ~ERL_BMS_PVOLT_L;
			errores.er_leve_1 &= ~ERL_BMS_PI_H;
			errores.er_grave_1 &= ~ERG_BMS_TEMP_H;
			errores.er_grave_1 &= ~ERG_BMS_PVOLT_H;
			errores.er_grave_1 &= ~ERG_BMS_PVOLT_L;
			errores.er_grave_1 &= ~ERG_BMS_PI_H;
			errores.er_critico_1 &= ~ERC_BMS_CHAS_CON;
			errores.er_critico_1 &= ~ERC_BMS_CEL_COM;
			errores.er_critico_1 &= ~ERC_BMS_SYS_ERR;
			break;
		/* Errores leves */
		case WARNING:
			switch (alarma_local)
			{
				case PACK_V_HIGH:
					errores.er_leve_1 |= ERL_BMS_PVOLT_H;
					errores.er_grave_1 &= ~ERG_BMS_PVOLT_H;
					break;
				case PACK_V_LOW:
					errores.er_leve_1 |= ERL_BMS_PVOLT_L;
					errores.er_grave_1 &= ~ERG_BMS_PVOLT_L;
					break;
				case PACK_I_HIGH:
					errores.er_leve_1 |= ERL_BMS_PI_H;
					errores.er_grave_1 &= ~ERG_BMS_PI_H;
					break;
				default:
					printf("No se reconoce la alarma leve de BMS\n");					
			}
			break;
		/* Errores graves */
		case ALARMA:
			switch (alarma_local)
			{
				case CELL_TEMP_HIGH:
					errores.er_grave_1 |= ERG_BMS_TEMP_H;
					break;
				case PACK_V_HIGH:
					errores.er_grave_1 |= ERG_BMS_PVOLT_H;
					errores.er_leve_1 &= ~ERL_BMS_PVOLT_H;
					break;
				case PACK_V_LOW:
					errores.er_grave_1 |= ERG_BMS_PVOLT_L;
					errores.er_leve_1 &= ~ERL_BMS_PVOLT_L;
					break;
				case PACK_I_HIGH:
					errores.er_grave_1 |= ERG_BMS_PI_H;
					errores.er_leve_1 &= ~ERL_BMS_PI_H;
					break;
				default:
					printf("No se reconoce la alarma grave de BMS\n");
			}
			break;
		/* Errores criticos */
		case ALARMA_CRITICA:
			switch (alarma_local)
			{
				case CHASIS_CONECT:
					errores.er_critico_1 |= ERC_BMS_CHAS_CON;
					break;
				case CELL_COM_ERROR:
					errores.er_critico_1 |= ERC_BMS_CEL_COM;
					break;
				case SYSTEM_ERROR:
					errores.er_critico_1 |= ERC_BMS_SYS_ERR;
					break;
				default:
					printf("No se reconoce la alarma critico de BMS\n");
			}
			break;
		default:
			printf("No se reconoce el nivel de error BMS\n");
	}	// Fin switch principal 
	pthread_mutex_unlock(&mut_errores);
	
}

void motor_errores(uint8_t num_motor)
{
	/* Variables */
	float V_pot_local;			// Valor leido de voltaje de potencia
	float V_aux_local;			// Valor leido de voltaje auxiliar de 5V
	float V_bat_local;			// Valor leido de voltaje de bateria
	uint8_t temp_int_ctrl_local;	// Temperatura interior del controlador
	uint8_t temp_sup_ctrl_local;	// Temperatura del lado superior del controlador
	uint8_t temp_inf_ctrl_local;	// Temperatura del lado inferior del controlador

	/* Actualizacion de variables locales desde globales */
	switch (num_motor)
	{
		case POS_M1:
			pthread_mutex_lock(&mut_motor1);
			V_pot_local = motor1.v_pot;
			V_aux_local = motor1.v_aux;
			V_bat_local = motor1.v_bat;
			temp_int_ctrl_local = motor1.temp_int_ctrl;
			temp_sup_ctrl_local = motor1.temp_sup_ctrl;
			temp_inf_ctrl_local = motor1.temp_inf_ctrl;
			pthread_mutex_unlock(&mut_motor1);
			break;
		case POS_M2:
			pthread_mutex_lock(&mut_motor2);
			V_pot_local = motor2.v_pot;
			V_aux_local = motor2.v_aux;
			V_bat_local = motor2.v_bat;
			temp_int_ctrl_local = motor2.temp_int_ctrl;
			temp_sup_ctrl_local = motor2.temp_sup_ctrl;
			temp_inf_ctrl_local = motor2.temp_inf_ctrl;
			pthread_mutex_unlock(&mut_motor2);
			break;
		case POS_M3:
			pthread_mutex_lock(&mut_motor3);
			V_pot_local = motor3.v_pot;
			V_aux_local = motor3.v_aux;
			V_bat_local = motor3.v_bat;
			temp_int_ctrl_local = motor3.temp_int_ctrl;
			temp_sup_ctrl_local = motor3.temp_sup_ctrl;
			temp_inf_ctrl_local = motor3.temp_inf_ctrl;
			pthread_mutex_unlock(&mut_motor3);
			break;
		case POS_M4:
			pthread_mutex_lock(&mut_motor4);
			V_pot_local = motor4.v_pot;
			V_aux_local = motor4.v_aux;
			V_bat_local = motor4.v_bat;
			temp_int_ctrl_local = motor4.temp_int_ctrl;
			temp_sup_ctrl_local = motor4.temp_sup_ctrl;
			temp_inf_ctrl_local = motor4.temp_inf_ctrl;
			pthread_mutex_unlock(&mut_motor4);
			break;
	}
	
	/* Comprobacion de errores uno a uno */
	/* ERRORES TENSION ALTA */
	if (V_pot_local > MOTOR_V_ALTA_GRAVE)
	{
		/* Tension alta - Error grave */
		pthread_mutex_lock(&mut_errores);
		switch (num_motor)
		{
			case POS_M1:
				errores.er_grave_1 |= ERG_M1_VOLT_H;
				errores.er_leve_1 &= ~ERL_M1_VOLT_H;
				break;
			case POS_M2:
				errores.er_grave_1 |= ERG_M2_VOLT_H;
				errores.er_leve_2 &= ~ERL_M2_VOLT_H;
				break;	
			case POS_M3:
				errores.er_grave_2 |= ERG_M3_VOLT_H;
				errores.er_leve_2 &= ~ERL_M3_VOLT_H;
				break;	
			case POS_M4:
				errores.er_grave_2 |= ERG_M4_VOLT_H;
				errores.er_leve_2 &= ~ERL_M4_VOLT_H;
				break;	
		}
		pthread_mutex_unlock(&mut_errores);
	}
	else if (V_pot_local > MOTOR_V_ALTA_LEVE)
	{
		/* Tension alta - Error leve */
		pthread_mutex_lock(&mut_errores);
		switch (num_motor)
		{
			case POS_M1:
				errores.er_leve_1 |= ERL_M1_VOLT_H;
				errores.er_grave_1 &= ~ERG_M1_VOLT_H;
				break;
			case POS_M2:
				errores.er_leve_2 |= ERL_M2_VOLT_H;
				errores.er_grave_1 &= ~ERG_M2_VOLT_H;
				break;	
			case POS_M3:
				errores.er_leve_2 |= ERL_M3_VOLT_H;
				errores.er_grave_2 &= ~ERG_M3_VOLT_H;
				break;	
			case POS_M4:
				errores.er_leve_2 |= ERL_M4_VOLT_H;
				errores.er_grave_2 &= ~ERG_M4_VOLT_H;
				break;	
		}
		pthread_mutex_unlock(&mut_errores);
	}
	else
	{
		/* Tension normal - Eliminamos los errores */
		pthread_mutex_lock(&mut_errores);
		switch (num_motor)
		{
			case POS_M1:
				errores.er_leve_1 &= ~ERL_M1_VOLT_H;
				errores.er_grave_1 &= ~ERG_M1_VOLT_H;
				break;
			case POS_M2:
				errores.er_leve_2 &= ~ERL_M2_VOLT_H;
				errores.er_grave_1 &= ~ERG_M2_VOLT_H;
				break;	
			case POS_M3:
				errores.er_leve_2 &= ~ERL_M3_VOLT_H;
				errores.er_grave_2 &= ~ERG_M3_VOLT_H;
				break;	
			case POS_M4:
				errores.er_leve_2 &= ~ERL_M4_VOLT_H;
				errores.er_grave_2 &= ~ERG_M4_VOLT_H;
				break;	
		}
		pthread_mutex_unlock(&mut_errores);
	}
	/* ERRORES TENSION BAJA */
	if (V_pot_local > MOTOR_V_BAJA_GRAVE)
	{
		/* Tension baja - Error grave */
		pthread_mutex_lock(&mut_errores);
		switch (num_motor)
		{
			case POS_M1:
				errores.er_grave_1 |= ERG_M1_VOLT_L;
				errores.er_leve_1 &= ~ERL_M1_VOLT_L;
				break;
			case POS_M2:
				errores.er_grave_1 |= ERG_M2_VOLT_L;
				errores.er_leve_2 &= ~ERL_M2_VOLT_L;
				break;	
			case POS_M3:
				errores.er_grave_2 |= ERG_M3_VOLT_L;
				errores.er_leve_2 &= ~ERL_M3_VOLT_L;
				break;	
			case POS_M4:
				errores.er_grave_2 |= ERG_M4_VOLT_L;
				errores.er_leve_2 &= ~ERL_M4_VOLT_L;
				break;	
		}
		pthread_mutex_unlock(&mut_errores);
	}
	else if (V_pot_local > MOTOR_V_BAJA_LEVE)
	{
		/* Tension baja - Error leve */
		pthread_mutex_lock(&mut_errores);
		switch (num_motor)
		{
			case POS_M1:
				errores.er_leve_1 |= ERL_M1_VOLT_L;
				errores.er_grave_1 &= ~ERG_M1_VOLT_L;
				break;
			case POS_M2:
				errores.er_leve_2 |= ERL_M2_VOLT_L;
				errores.er_grave_1 &= ~ERG_M2_VOLT_L;
				break;	
			case POS_M3:
				errores.er_leve_2 |= ERL_M3_VOLT_L;
				errores.er_grave_2 &= ~ERG_M3_VOLT_L;
				break;	
			case POS_M4:
				errores.er_leve_2 |= ERL_M4_VOLT_L;
				errores.er_grave_2 &= ~ERG_M4_VOLT_L;
				break;	
		}
		pthread_mutex_unlock(&mut_errores);
	}
	else
	{
		/* Tension normal - Eliminamos los errores */
		pthread_mutex_lock(&mut_errores);
		switch (num_motor)
		{
			case POS_M1:
				errores.er_leve_1 &= ~ERL_M1_VOLT_L;
				errores.er_grave_1 &= ~ERG_M1_VOLT_L;	
				break;
			case POS_M2:
				errores.er_leve_2 &= ~ERL_M2_VOLT_L;
				errores.er_grave_1 &= ~ERG_M2_VOLT_L;	
				break;	
			case POS_M3:
				errores.er_leve_2 &= ~ERL_M3_VOLT_L;
				errores.er_grave_2 &= ~ERG_M3_VOLT_L;	
				break;	
			case POS_M4:
				errores.er_leve_2 &= ~ERL_M4_VOLT_L;
				errores.er_grave_2 &= ~ERG_M4_VOLT_L;	
				break;	
		}
		pthread_mutex_unlock(&mut_errores);				
	}
	/* ERRORES TEMPERATURA ALTA */
	if (max(temp_int_ctrl_local, max(temp_sup_ctrl_local, temp_inf_ctrl_local)) > MOTOR_TEMP_ALTA_GRAVE)
	{
		/* Temperatura alta - Error grave */
		pthread_mutex_lock(&mut_errores);
		switch (num_motor)
		{
			case POS_M1:
				errores.er_grave_1 |= ERG_M1_TEMP_H;
				errores.er_leve_1 &= ~ERL_M1_TEMP_H;
				break;
			case POS_M2:
				errores.er_grave_2 |= ERG_M2_TEMP_H;
				errores.er_leve_2 &= ~ERL_M2_TEMP_H;	
				break;	
			case POS_M3:
				errores.er_grave_2 |= ERG_M3_TEMP_H;
				errores.er_leve_2 &= ~ERL_M3_TEMP_H;
				break;	
			case POS_M4:
				errores.er_grave_2 |= ERG_M4_TEMP_H;
				errores.er_leve_2 &= ~ERL_M4_TEMP_H;
				break;	
		}
		pthread_mutex_unlock(&mut_errores);	
	}
	else if (max(temp_int_ctrl_local, max(temp_sup_ctrl_local, temp_inf_ctrl_local)) > MOTOR_TEMP_ALTA_LEVE)
	{
		/* Temperatura alta - Error leve */
		pthread_mutex_lock(&mut_errores);
		switch (num_motor)
		{
			case POS_M1:
				errores.er_leve_1 |= ERL_M1_TEMP_H;
				errores.er_grave_1 &= ~ERG_M1_TEMP_H;
				break;
			case POS_M2:
				errores.er_leve_2 |= ERL_M2_TEMP_H;
				errores.er_grave_2 &= ~ERG_M2_TEMP_H;	
				break;	
			case POS_M3:
				errores.er_leve_2 |= ERL_M3_TEMP_H;
				errores.er_grave_2 &= ~ERG_M3_TEMP_H;
				break;	
			case POS_M4:
				errores.er_leve_2 |= ERL_M4_TEMP_H;
				errores.er_grave_2 &= ~ERG_M4_TEMP_H;
				break;	
		}
		pthread_mutex_unlock(&mut_errores);
	}
	else
	{
		/* Temperatura normal - Eliminamos los errores */
		pthread_mutex_lock(&mut_errores);
		switch (num_motor)
		{
			case POS_M1:
				errores.er_leve_1 &= ~ERL_M1_TEMP_H;
				errores.er_grave_1 &= ~ERG_M1_TEMP_H;
				break;
			case POS_M2:
				errores.er_leve_2 &= ~ERL_M2_TEMP_H;
				errores.er_grave_2 &= ~ERG_M2_TEMP_H;
				break;	
			case POS_M3:
				errores.er_leve_2 &= ~ERL_M3_TEMP_H;
				errores.er_grave_2 &= ~ERG_M3_TEMP_H;
				break;	
			case POS_M4:
				errores.er_leve_2 &= ~ERL_M4_TEMP_H;
				errores.er_grave_2 &= ~ERG_M4_TEMP_H;
				break;	
		}
		pthread_mutex_unlock(&mut_errores);				
	}
	/* ERRORES DIFERENCIA TENSION */
	if (abs(V_aux_local-V_bat_local) > MOTOR_DIFF_V_MAX_LEVE)
	{
		/* Diferencia de tension maxima - Error leve */
		pthread_mutex_lock(&mut_errores);
		switch (num_motor)
		{
			case POS_M1:
				errores.er_leve_1 |= ERL_M1_DIFF_PWR;
				break;
			case POS_M2:
				errores.er_leve_2 |= ERL_M2_DIFF_PWR;
				break;	
			case POS_M3:
				errores.er_leve_2 |= ERL_M3_DIFF_PWR;
				break;	
			case POS_M4:
				errores.er_leve_2 |= ERL_M4_DIFF_PWR;
				break;	
		}
		pthread_mutex_unlock(&mut_errores);
	}
	else
	{
		/* Diferencia de tension normal - Eliminamos los errores */
		pthread_mutex_lock(&mut_errores);
		switch (num_motor)
		{
			case POS_M1:
				errores.er_leve_1 &= ~ERL_M1_DIFF_PWR;
				break;
			case POS_M2:
				errores.er_leve_2 &= ~ERL_M2_DIFF_PWR;
				break;	
			case POS_M3:
				errores.er_leve_2 &= ~ERL_M3_DIFF_PWR;
				break;	
			case POS_M4:
				errores.er_leve_2 &= ~ERL_M4_DIFF_PWR;
				break;	
		}
		pthread_mutex_unlock(&mut_errores);
	}
	/* ERRORES TENSION AUXILIAR 5V FUERA DE RANGO */
	if ((V_aux_local > MOTOR_V_AUX_H_LEVE) || (V_aux_local < MOTOR_V_AUX_L_LEVE))
	{
		/* Tension auxiliar fuera de rango - Error leve */
		pthread_mutex_lock(&mut_errores);
		switch (num_motor)
		{
			case POS_M1:
				errores.er_leve_2 |= ERL_M1_VAUX;
				break;
			case POS_M2:
				errores.er_leve_2 |= ERL_M2_VAUX;
				break;	
			case POS_M3:
				errores.er_leve_2 |= ERL_M3_VAUX;
				break;	
			case POS_M4:
				errores.er_leve_2 |= ERL_M4_VAUX;
				break;	
		}
		pthread_mutex_unlock(&mut_errores);
	}
	else
	{
		/* Tension auxiliar dentro de rango - Eliminamos los errores */
		pthread_mutex_lock(&mut_errores);
		switch (num_motor)
		{
			case POS_M1:
				errores.er_leve_2 &= ~ERL_M1_VAUX;
				break;
			case POS_M2:
				errores.er_leve_2 &= ~ERL_M2_VAUX;
				break;	
			case POS_M3:
				errores.er_leve_2 &= ~ERL_M3_VAUX;
				break;	
			case POS_M4:
				errores.er_leve_2 &= ~ERL_M4_VAUX;
				break;	
		}
		pthread_mutex_unlock(&mut_errores);
	}
}


void sensores_errores()
{
	/* Variables */
	float susp_dd_local;
	float susp_di_local;
	float susp_td_local;
	float susp_ti_local;
	float volante_local;
	float freno_local;
	float acelerador_local;

	/* Cargamos los valores globales en variables locales */
	pthread_mutex_lock(&mut_vehiculo);
	susp_dd_local = vehiculo.susp_dd;
	susp_di_local = vehiculo.susp_di;
	susp_td_local = vehiculo.susp_td;
	susp_ti_local = vehiculo.susp_ti;
	volante_local = vehiculo.volante;
	freno_local = vehiculo.freno;
	acelerador_local = vehiculo.acelerador;
	pthread_mutex_unlock(&mut_vehiculo);	
}
