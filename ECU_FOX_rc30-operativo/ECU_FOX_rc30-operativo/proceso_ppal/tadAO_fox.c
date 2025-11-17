/**********************************************************************************
***										***
***	Fichero: tadAO_fox.c							***
***	Fecha: 08/10/2013							***	
***	Autor: Alejandro Oliva Torres						***
***	Descripción: Gestion de la tarjeta de salidas analogicas. 		***
***	Se encarga de configurar y poner en marcha la TAD y sacar las AO. 	***
***										***						
**********************************************************************************/

///////////////////////Cabecera////////////////////
#include <stdio.h>	//entrada-salida estandar (printf, scanf...)
#include <stdlib.h>	//memoria dinamica (malloc...), exit, system...
#include <stdint.h>	//tipos enteros: (u)int8_t (u)int16_t (u)int32_t (u)int64_t
#include <unistd.h>	//constantes y tipos UNIX
#include <errno.h>	//constantes error
#include <time.h> 	//tiempos y temporizadores
#include <pthread.h>
#include <signal.h>
/* Tarjeta de adquisicion */
#include "./include/dscud.h"
/* Include local */
#include "./include/constantes_fox.h"
#include "./include/estructuras_fox.h"
#include "./include/declaraciones_fox.h"
///////////////////////////////////////////////////

/* Variables globales y mutex */
extern uint16_t hilos_listos;
extern est_pot_t potencias;
extern pthread_mutex_t mut_potencias;
extern est_error_t errores;

extern pthread_mutex_t mut_errores;
extern pthread_mutex_t mut_hilos_listos;
extern pthread_mutex_t mut_inicio;
extern pthread_cond_t inicio;

//Variables globales al archivo usadas en la inicializacion y
//liberacion de la TADQ.
DSCB dscb;
DSCCB dsccb;
DSCDASETTINGS dscdasettings;
////////////////////////////////////////////////////


//Rutina principal del hilo
void *adq_ao (void *pi)
{
	//Variables locales
	int i = 0;
	char inicio_ok = 0;
	char fin_local = 0;
//	DSCDACODE out_code;
	float acelerador[NUM_MOTORES];
	float freno[NUM_MOTORES];
	float adq_cda_out[NUM_CANALES_CDA];
	struct timespec periodo = {0,TIEMPO_BUCLE_NS};

	//Inicializacion de la TADQ
	inicio_ok = inicia_tadq();
	printf("Iniciado hilo ADQ salida\n");
	
	/* Si hay error */
	if (inicio_ok != 0)
	{
		pthread_mutex_lock(&mut_errores);
		errores.er_critico_1 |= ERC_ECU_COM_TAD_AO;
		pthread_mutex_unlock(&mut_errores);
	}
	
	/* Indica que el hilo de tarjeta de adquisicion de salidas analogicas esta listo */
	pthread_mutex_lock(&mut_hilos_listos);
	hilos_listos |= HILO_TADAO_LISTO;
	pthread_mutex_unlock(&mut_hilos_listos);
	printf("Hilo TADAO listo\n");
	pthread_mutex_lock(&mut_inicio);
	pthread_cond_wait(&inicio, &mut_inicio);
	pthread_mutex_unlock(&mut_inicio);
	
	while (!fin_local)
	{	
		memset(adq_cda_out, 0, sizeof(adq_cda_out));
		
		pthread_mutex_lock(&mut_potencias);
		memcpy(acelerador,potencias.acelerador,sizeof(acelerador));
		memcpy(freno,potencias.freno,sizeof(freno));
		pthread_mutex_unlock(&mut_potencias);
		
		// Preparamos los valores de las salidas analogicas
		adq_cda_out[ANLG_OUT_ACL_M1] = acelerador[POS_M1];
		adq_cda_out[ANLG_OUT_ACL_M2] = acelerador[POS_M2];
		adq_cda_out[ANLG_OUT_ACL_M3] = acelerador[POS_M3];
		adq_cda_out[ANLG_OUT_ACL_M4] = acelerador[POS_M4];
		adq_cda_out[ANLG_OUT_FRN_M1] = freno[POS_M1];
		adq_cda_out[ANLG_OUT_FRN_M2] = freno[POS_M2];
		adq_cda_out[ANLG_OUT_FRN_M3] = freno[POS_M3];
		adq_cda_out[ANLG_OUT_FRN_M4] = freno[POS_M4];
		
		// Escribe las salidas analogicas
		for (i=0; i<NUM_CANALES_CDA; i++)
			cda_wr (i, adq_cda_out[i], BIPOLAR);
			
		nanosleep(&periodo, NULL);
		
		pthread_mutex_lock(&mut_hilos_listos);
		if ((hilos_listos&HILO_TADAO_LISTO)==0x0000)
			fin_local = 1;
		pthread_mutex_unlock(&mut_hilos_listos);
	}
	
	//Poner a cero las salidas
	for (i=0; i<NUM_CANALES_CDA; i++)
		cda_wr (i, 0, BIPOLAR);
	//Liberacion de la TADQ
	//dscFreeBoard(dscb);
	dscFree();
	//Cierre del hilo
	pthread_exit (NULL);
	return(NULL);	
}


//Funcion que inicializa la TADQ. Hace uso de funciones incluidas en la libreria
//del fabricante.
char inicia_tadq (void)
{
	char result = 0;
	ERRPARAMS errparams;
	
	/* Inicia la TADQ y si hay error lo reporta */
	if ((result = dscInit(DSC_VERSION)) != DE_NONE)
	{
		dscGetLastError(&errparams);
		fprintf(stderr, "dscInit failed: %s (%s)\n", dscGetErrorString(result), errparams.errstring);
		return result;
	}
	
	memset(&dsccb, 0, sizeof(DSCCB));
	
	dsccb.boardtype = DSC_RMM1612;
	//Direccion de memoria base y nivel IRQ de la TADQ
	dsccb.base_address = DIR_BASE_ADQ_AO;
	dsccb.int_level = IRQ_ADQ;
	dsccb.RMM_external_trigger = FALSE;
	dsccb.RMM_external_trigger_c3 = FALSE;
		
	if ((result = dscInitBoard(DSC_RMM1612, &dsccb, &dscb)) != DE_NONE)
	{
		dscGetLastError(&errparams);
		fprintf(stderr, "dscInitBoard failed: %s (%s)\n", dscGetErrorString(result), errparams.errstring);
		return result;
	}
	
	//Inicializacion del CDA: rango, polaridad.
	dscdasettings.range = RANGO_CDA;
	dscdasettings.polarity = BIPOLAR;	
	//dscdasettings.load_cal = TRUE;
	dscDASetSettings(dscb, &dscdasettings);
	
	//Si es necesario calibrar la TADQ:
	//calibra_adq();
	
	return 0;
}

//Funcion introducida si se hace necesaria la calibracion.
//Consultar manual de la TADQ.
void calibra_adq (void)
{
	DSCDACALPARAMS dscdacalparams;
	
	//El CDA se puede calibrar tambien pero no es recomendable, porque los valores de
	//salida pueden fluctuar y alterar mucho el estado de los dispositivos conectados.
	//dscdacalparams.polarity = TRUE;
	//dscdacalparams.offset = 5;
	dscdacalparams.ch0pol = 1;
	dscdacalparams.ch1pol = 1;
	dscdacalparams.ch0prog = 1;
	dscdacalparams.ch1prog = 1;
	dscdacalparams.ch0ext = 0;
	dscdacalparams.ch1ext = 0;
	dscdacalparams.darange = 10;
	dscDAAutoCal (dscb, &dscdacalparams);
	dscDACalVerify (dscb, &dscdacalparams);
	
	//dscdasettings.load_cal = TRUE;
}

//Funcion que escribe un valor en flotante en un canal del CDA
void cda_wr (BYTE canal, float valor_volt, BYTE polaridad)
{
	DSCDACODE output_code;
	
	//Hay que hacer una conversion del valor de tension deseado al output_code,
	//incluyendo el rango usado.
	switch (polaridad)
	{
		case UNIPOLAR:
			if (valor_volt < 0) valor_volt = 0;
			if (valor_volt > RANGO_CDA) valor_volt = RANGO_CDA;
			output_code = (DSCDACODE) (valor_volt  * 4096 / RANGO_CDA);
		break;
		case BIPOLAR:
			if (valor_volt < -RANGO_CDA) valor_volt = -RANGO_CDA;
			if (valor_volt > RANGO_CDA) valor_volt = RANGO_CDA;
			output_code = (DSCDACODE) (valor_volt  * 2048 / RANGO_CDA + 2048);
		break;
		default:
	}
	if (output_code > 4095) output_code = 4095;

	dscDAConvert(dscb, canal, output_code);
}

