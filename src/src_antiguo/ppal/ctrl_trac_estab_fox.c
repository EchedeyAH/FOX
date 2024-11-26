/**************************************************************************
***									***
***	Fichero: ctrl_trac_estab_fox.c					***
***	Fecha: 10/10/2013						***
***	Autor: Elena Gonzalez						***		
***	Descripción: Hilo de control de traccion y estabilidad.		***
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


void *ctrl_traccion_main (void *pi)
{
	/**************************************************
	***		VARIABLES			***
	**************************************************/
	float freno_local;			// Valor del freno accionado por el conductor
	float acelera_local;
	BOOL act_acel_local;
	BOOL motor_on_local[NUM_MOTORES];
	
	float trac_freno_local[NUM_MOTORES];	// Valor de freno para mandarle a cada motor
	float trac_acelera_local[NUM_MOTORES];
	BOOL act_freno_local[NUM_MOTORES];
	
	int i;
	BOOL fin_local = 0;
	struct timespec espera = {0, TIEMPO_BUCLE_NS};
	
	/* Indica que el hilo del Control de traccion esta listo */
	pthread_mutex_lock(&mut_hilos_listos);
	hilos_listos |= HILO_TRAC_LISTO;
	pthread_mutex_unlock(&mut_hilos_listos);
	printf("Hilo TRAC listo\n");
	pthread_mutex_lock(&mut_inicio);
	pthread_cond_wait(&inicio, &mut_inicio);
	pthread_mutex_unlock(&mut_inicio);

	
	while (!fin_local)
	{
		/* Volcado de las variables globales a locales */
		pthread_mutex_lock(&mut_vehiculo);
		acelera_local = vehiculo.acelerador;
		freno_local = vehiculo.freno;
		act_acel_local = vehiculo.act_acel;
		pthread_mutex_unlock(&mut_vehiculo);
		pthread_mutex_lock(&mut_supervisor);
		memcpy(motor_on_local, supervisor.motor_on, sizeof(motor_on_local));		
		pthread_mutex_unlock(&mut_supervisor);
		
		/* Asigna el mismo valor a los 4 motores */
		for(i=POS_M1; i<(POS_M4+1); i++)
		{	
			if (freno_local > 0.5)
			{
				trac_acelera_local[i] = 0;
				act_freno_local[i] = TRUE;
				trac_freno_local[i] = freno_local;
			}
			else {
				if (motor_on_local[i] == TRUE)
					trac_acelera_local[i] = acelera_local;
				else
					trac_acelera_local[i] = 0;
				act_freno_local[i] = FALSE;
				trac_freno_local[i] = 0;
			}
		}

		/* Actualizacion de variable global */
		pthread_mutex_lock(&mut_potencias);
		memcpy(potencias.act_freno, act_freno_local, sizeof(act_freno_local));		
		memcpy(potencias.acelerador, trac_acelera_local, sizeof(trac_acelera_local));
		memcpy(potencias.freno, trac_freno_local, sizeof(trac_freno_local));
		pthread_mutex_unlock(&mut_potencias);
	
		/* Espera el tiempo de muestreo establecido */
		nanosleep(&espera, NULL);
		
		pthread_mutex_lock(&mut_hilos_listos);
		if ((hilos_listos&HILO_TRAC_LISTO)==0x0000)
			fin_local = 1;
		pthread_mutex_unlock(&mut_hilos_listos);
	} // Fin bucle while

	pthread_exit(NULL);
	return(NULL);
}

void *ctrl_estabilidad_main (void *pi)
{
	/**************************************************
	***		VARIABLES			***
	**************************************************/
	struct timespec espera = {0, TIEMPO_BUCLE_NS};
	BOOL fin_local = 0;
	
	/* Indica que el hilo del Control de estabilidad esta listo */
	pthread_mutex_lock(&mut_hilos_listos);
	hilos_listos |= HILO_ESTAB_LISTO;
	pthread_mutex_unlock(&mut_hilos_listos);
	printf("Hilo ESTAB listo\n");
	pthread_mutex_lock(&mut_inicio);
	pthread_cond_wait(&inicio, &mut_inicio);
	pthread_mutex_unlock(&mut_inicio);
	
	/* Algoritmo de control de estabilidad */
	
	while(!fin_local)
	{
		nanosleep (&espera, NULL);
		
		pthread_mutex_lock(&mut_hilos_listos);
		if ((hilos_listos&HILO_ESTAB_LISTO)==0x0000)
			fin_local = 1;
		pthread_mutex_unlock(&mut_hilos_listos);
	} // Fin bucle while

	pthread_exit(NULL);
	return(NULL);
}