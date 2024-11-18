/**************************************************************************
***									***
***	Fichero: gestionpot_fox.c					***
***	Fecha: 10/10/2013						***
***	Autor: Elena Gonzalez						***
***	Descripción: Hilo de gestion de potencia.			***
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
***		VARIABLES GLOBALES		***
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
extern datosImu_t tDatosImu;

/* MUTEX */
extern pthread_mutex_t mut_vehiculo;
extern pthread_mutex_t mut_bateria;
extern pthread_mutex_t mut_motor1;
extern pthread_mutex_t mut_motor2;
extern pthread_mutex_t mut_motor3;
extern pthread_mutex_t mut_motor4;
extern pthread_mutex_t mut_errores;
extern pthread_mutex_t mut_potencias;
extern pthread_mutex_t mDatosImu;
extern pthread_mutex_t mut_hilos_listos;
extern pthread_mutex_t mut_inicio;
extern pthread_cond_t inicio;


void *gestion_potencia_main (void *pi)
{
	/**************************************************
	***		VARIABLES			***
	**************************************************/
	BOOL fin_local = 0;
	struct timespec espera = {0, TIEMPO_BUCLE_NS};
	
	/* Indica que el hilo de gestion de potencia esta listo */
	pthread_mutex_lock(&mut_hilos_listos);
	hilos_listos |= HILO_GPOT_LISTO;
	pthread_mutex_unlock(&mut_hilos_listos);
	printf("Hilo GPOT listo\n");
	pthread_mutex_lock(&mut_inicio);
	pthread_cond_wait(&inicio, &mut_inicio);
	pthread_mutex_unlock(&mut_inicio);
	
	while (!fin_local)
	{	
		/* Espera el tiempo de muestreo establecido */
		nanosleep(&espera, NULL);
		
		pthread_mutex_lock(&mut_hilos_listos);
		if ((hilos_listos&HILO_GPOT_LISTO)==0x0000)
			fin_local = 1;
		pthread_mutex_unlock(&mut_hilos_listos);
	
	} // Fin bucle while

	pthread_exit(NULL);
	return(NULL);
}


