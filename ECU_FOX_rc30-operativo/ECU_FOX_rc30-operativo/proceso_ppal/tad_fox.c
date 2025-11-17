/**************************************************************************
***									***
***	Fichero: tad_fox.c						***
***	Fecha: 02/04/2013						***
***	Autor: Elena Gonzalez. Modificacion al codigo de David Marcos.	***
***	Descripcion: Hilo de adquisicion de datos 			***
***	Recogida de datos de la DAQ					***
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
/* Tarjeta de adquisicion */
#include "./include/pcm-3718hg_ho.h"
/* Include local */
#include "./include/constantes_fox.h"
#include "./include/estructuras_fox.h"
#include "./include/declaraciones_fox.h"

/* INICIALIZACION DE VARIABLES GLOBALES */
config_pcm3718 parametros_tarjeta;
uintptr_t base;		// Direccion del registro 0 de la tarjeta de adq.

/* Variables globales */
extern uint16_t hilos_listos;

/* Estructuras globales */
extern est_veh_t vehiculo;
extern est_error_t errores;
extern est_pot_t potencias;

/* Mutex globales */
extern pthread_mutex_t mut_vehiculo;
extern pthread_mutex_t mut_errores;
extern pthread_mutex_t mut_potencias;
extern pthread_mutex_t mut_hilos_listos;
extern pthread_mutex_t mut_inicio;
extern pthread_cond_t inicio;


// FUNCION PRINCIPAL
void *adq_main(void *pi)
{
	/******************************************
	***		VARIABLES		***
	******************************************/
	BOOL fin_local = 0;
//	short i = 0;		// Indice para bucle for
	int privlg = 0;		// Indica si se han concedido privilegios de E/S
	int lee_ok = 0;		// Indica si ha ocurrido algun error en la lectura del fichero de config de la TAD
	float valor_analog_in[NUM_ANALOG_IN];	// Almacena de forma local las entradas analogicas
//	uint16_t valor_analog_out;	// En principio no da ninguna salida
	uint16_t aux_lectura_dig;	// Variable auxiliar para lectura de segnales digitales
	uint16_t aux_escritura_dig;
	
	memset(valor_analog_in, 0, sizeof(valor_analog_in));
	
	/**************************************************
	***	CONFIGURA TARJETA DE ADQUISICION	***
	**************************************************/
	/* Lee fichero de configuracion de la tarjeta de adq. */
	lee_ok = lee_fichero_cfg();
	if(lee_ok < 0)
	{
		printf("Error de lectura de fichero de configuracion - TAD.\n");
		/* Si da error, salimos del programa */
		pthread_mutex_lock(&mut_errores);
		errores.er_critico_1 |= ERC_ECU_COM_TAD;
		pthread_mutex_unlock(&mut_errores);
	}	
	/* Pide privilegios de E/S */
	privlg=ThreadCtl(_NTO_TCTL_IO, 0);
	if(privlg == -1)
	{
		printf("Error. Denegados privilegios de E/S\n");
		pthread_mutex_lock(&mut_errores);
		errores.er_critico_1 |= ERC_ECU_COM_TAD;
		pthread_mutex_unlock(&mut_errores);
	}
	/* Mapeo de la tarjeta de adquisicion */
	/* Necesita 16 bytes a partir de la direccion. */
	base = mmap_device_io((size_t)16, parametros_tarjeta.direccion);
	if (base == MAP_DEVICE_FAILED)
	{
		printf("Error al mapear el dispositivo.\n");
		pthread_mutex_lock(&mut_errores);
		errores.er_critico_1 |= ERC_ECU_COM_TAD;
		pthread_mutex_unlock(&mut_errores);
	}
	/* Hasta que no mapeamos la tarjeta, no sabemos si el jumper JP2 
	se encuentra en modo 8 canales diferenciales o 16 canales absolutos.
	Para corroborarlo, comprobamos los canales intentando leer en modo
	diferencial una entrada mayor que la 7. */
	comprueba_canales();
	/* Configura los rangos de las entradas. */
	configura_rangos();
	/* Seleccion de los canales que se van a leer.
	Hay que hacerlo despues de configurar los rangos porque se usa
	el mismo registro. */
//	selec_canal();
	/* Configuracion del byte de control (ver manual, pg 31): */
	config_byte_ctrl();	
	/* Deshabilitar pacer  */
	habilitar_pacer(PACER_DISABLED);
	/* Deshabilitar ctrl int. FIFO da */
	habilitar_ctrl_int_fifo_da(CTRL_DISABLED);

	
	/* Espera al resto de hilos */
	/* Indica que el hilo de tarjeta de adquisicion esta listo */
	pthread_mutex_lock(&mut_hilos_listos);
	hilos_listos |= HILO_TAD_LISTO;
	pthread_mutex_unlock(&mut_hilos_listos);
	printf("Hilo TAD listo\n");
	pthread_mutex_lock(&mut_inicio);
	pthread_cond_wait(&inicio, &mut_inicio);
	pthread_mutex_unlock(&mut_inicio);
	
	/**************************************************
	***	RECOGIDA DE DATOS ***
	**************************************************/
	/* No entra en el bucle si ha ocurrido un error de mapeo de la tarjeta de adq. */
	while(!fin_local)
	{
		aux_lectura_dig = 0x0000;
		aux_escritura_dig = 0x0000;
		
		/* Obtiene la marca de tiempo de la recogida del dato */
//		clock_gettime(CLOCK_REALTIME, &local_timestamp);
	
		/* Escribe y lee entrada digital ************
		********************************************/
		// Recoge las entradas y salidas digitales en una variable auxiliar
		pthread_mutex_lock(&mut_potencias);
//		aux_escritura_dig |= (uint16_t) (potencias.act_acel[POS_M1]<<DIG_OUT_ACL_M1);
//		aux_escritura_dig |= (uint16_t) (potencias.act_acel[POS_M2]<<DIG_OUT_ACL_M2);
//		aux_escritura_dig |= (uint16_t) (potencias.act_acel[POS_M3]<<DIG_OUT_ACL_M3);
//		aux_escritura_dig |= (uint16_t) (potencias.act_acel[POS_M4]<<DIG_OUT_ACL_M4);
		aux_escritura_dig |= (uint16_t) (potencias.act_freno[POS_M1]<<DIG_OUT_FRN_M1);
		aux_escritura_dig |= (uint16_t) (potencias.act_freno[POS_M2]<<DIG_OUT_FRN_M2);
		aux_escritura_dig |= (uint16_t) (potencias.act_freno[POS_M3]<<DIG_OUT_FRN_M3);
		aux_escritura_dig |= (uint16_t) (potencias.act_freno[POS_M4]<<DIG_OUT_FRN_M4);
		pthread_mutex_unlock(&mut_potencias);
		
		modif_dato_digital(&aux_lectura_dig, &aux_escritura_dig);
		
		// Carga los valores leidos en las variables locales
		pthread_mutex_lock(&mut_vehiculo);
		vehiculo.marcha_atras = (BOOL) ((aux_lectura_dig & (1<<DIG_IN_REV))>>DIG_IN_REV);
		vehiculo.act_acel = (BOOL) ((aux_lectura_dig & (1<<DIG_IN_ACL))>>DIG_IN_ACL);
		pthread_mutex_unlock(&mut_vehiculo);
	
		/* Lee entrada analogica */
		// Recoge las entradas analogicas de la tarjeta de adq. recorriendo los canales uno a uno
		lee_dato_analogico(valor_analog_in);
		/* Interpreta seniales y actualiza el valor en las variables globales correspondientes */
		interpreta_entrada_analog(valor_analog_in);
	
		nanosleep(&(parametros_tarjeta.periodo), NULL); //50 ms
		
		pthread_mutex_lock(&mut_hilos_listos);
		if ((hilos_listos&HILO_TAD_LISTO)==0x0000)
			fin_local = 1;
		pthread_mutex_unlock(&mut_hilos_listos);
		
	}
	
	/**************************************************
	***	CIERRE ORDENADO DEL HILO		***
	**************************************************/
	munmap_device_io(base, 16);
	pthread_exit(NULL);
	return(NULL);
}

/**************************************************
***	   LEE DATO ANALOGICO DE LA TAD  	***
**************************************************/
/* Se le pasa como parametro la direccion de la variable que recogera el dato */
void lee_dato_analogico(float *valor_analog_leido)
{
	uint8_t i = 0;
	uint8_t octeto = 0x00;				// Para trigger de lectura
//	uint8_t aux_byte_low = 0;		// Para lectura del registro de entrada en 2 bytes: LSB
//	uint8_t aux_byte_high = 0;		// MSB
	int num_canal = 0;				// Para recoger el numero de canal leido
	uint16_t dato_low = 0x0000;	// Para recoger el valor del dato leido en 2 bytes: LSB
	uint16_t dato_high = 0x0000;	// MSB
	uint16_t dato = 0x0000;
	
	/* Recojo datos de los canales uno a uno, cada lectura lee un canal distinto */ 
	for (i=0; i<NUM_ANALOG_IN; i++)
	{
		//Seleccion de canal
		out8(base+MUX_SCAN_CHANNEL,i);
		delay(1);
		/* Peticion de dato */
		out8(base+SW_AD_TRIGGER,1);

		/* Espero hasta que el dato este listo */
		do
			octeto=in8(base+STATUS);
//		while(!(octeto&INT));
		while((octeto&0x80)==0x80);
		
		/* Leo los registros de la entrada analogica de un canal */
//		aux_byte_low = in8(base+AD_LOW_BYTE);
//		aux_byte_high = in8(base+AD_HIGH_BYTE);
		
		dato_low = in8(base+AD_LOW_BYTE);
		delay(1);
		dato_high = in8(base+AD_HIGH_BYTE);
	
		/* Falseo de la senial de entrada a la TAD */
		//aux_byte_low = (0x00 | (0x0F & i));	// El canal va cambiando automaticamente con i
		//aux_byte_high = 0x80;					// El valor para cada entrada es fijo y es 2048 que es la mitad del maximo (2,5v o 5v)
		
		/* Recojo el numero de canal leido */
//		num_canal = (int)(aux_byte_low & 0x0F);
		num_canal = dato_low & 0x0F;
		if (num_canal != i) printf("TAD Error de lectura en canal %d",i);
		/* Recojo el valor del dato leido en 2 bytes */
//		dato_low = ((uint16_t) aux_byte_low) & 0x00F0;
//		dato_high = ((uint16_t) aux_byte_high) & 0x00FF;
//		dato = ((dato_high << 4)+(dato_low >> 4)) & 0x0FFF;
		dato = (dato_high << 4)+(dato_low >> 4);
		
		// Proceso el dato para adaptarlo al formato final y lo almaceno en la posicion correspondiente al canal leido
		//UNIPOLAR
		if (parametros_tarjeta.rango_ad_f[i] > 0)
		{
			valor_analog_leido[i] = (float) dato / 4096.0 * parametros_tarjeta.rango_ad_f[i];
			if (valor_analog_leido[i] < 0) valor_analog_leido[i] = 0;
			if (valor_analog_leido[i] > parametros_tarjeta.rango_ad_f[i]) valor_analog_leido[i] = parametros_tarjeta.rango_ad_f[i];	
		}
		//BIPOLAR
		else if (parametros_tarjeta.rango_ad_f[i] < 0)
		{
			valor_analog_leido[i] = ((float) dato / 4096.0 * (-2) + 1) * parametros_tarjeta.rango_ad_f[i];
			if (valor_analog_leido[i] < parametros_tarjeta.rango_ad_f[i]) valor_analog_leido[i] = parametros_tarjeta.rango_ad_f[i];
			if (valor_analog_leido[i] > -parametros_tarjeta.rango_ad_f[i]) valor_analog_leido[i] = -parametros_tarjeta.rango_ad_f[i];	
		}
		// Reseteo el bit INT del registro 8
//		out8(base+CLEAR_INTERRUPT_REQUEST, 0);
	}
}

/**************************************************
***	   MODIFICA DATO DIGITAL DE LA TAD 	 	***
**************************************************/
void modif_dato_digital(uint16_t *aux_lectura, uint16_t *aux_escritura)
{
	uint8_t MSByte, LSByte;
	uint16_t mascara, dato;

	LSByte = in8(base+DIO_LOW_BYTE);
	MSByte = in8(base+DIO_HIGH_BYTE);

	dato = (((uint16_t) MSByte << 8) & MASK_BYTE_ALTO)|((uint16_t) LSByte & MASK_BYTE_BAJO);
	*aux_lectura = dato;
	
	mascara = (((uint16_t) parametros_tarjeta.dio_1 << 8) & MASK_BYTE_ALTO)|((uint16_t) parametros_tarjeta.dio_0 & MASK_BYTE_BAJO);
	//los bits de entrada los deja igual y cambia los de salida
	dato = mascara & *aux_escritura;
	LSByte = (uint8_t) (dato & MASK_BYTE_BAJO);
	MSByte = (uint8_t) ((dato & MASK_BYTE_ALTO) >> 8);

	out8(base+DIO_LOW_BYTE,LSByte);
	out8(base+DIO_HIGH_BYTE,MSByte);
}

void interpreta_entrada_analog(float *valor_analog_in)
{

	pthread_mutex_lock(&mut_vehiculo);	
	vehiculo.acelerador = valor_analog_in[ANLG_IN_ACL];
	vehiculo.freno = valor_analog_in[ANLG_IN_FRN];
	vehiculo.volante = valor_analog_in[ANLG_IN_VLT];	
	vehiculo.susp_di = valor_analog_in[ANLG_IN_SDI];			
	vehiculo.susp_dd = valor_analog_in[ANLG_IN_SDD];		
	vehiculo.susp_ti = valor_analog_in[ANLG_IN_STI];
	vehiculo.susp_td = valor_analog_in[ANLG_IN_STD];
	vehiculo.i_eje_d = FACTOR_CONV_I * valor_analog_in[ANLG_IN_ID];		
	vehiculo.i_eje_t = FACTOR_CONV_I * valor_analog_in[ANLG_IN_IT];				
	pthread_mutex_unlock(&mut_vehiculo);	

}

void comprueba_canales(void)
{	
	uint8_t octeto;
	
	octeto=in8(base+STATUS);
	if((!(octeto&MUX))&&(parametros_tarjeta.canal_fin_ad>7))
	{
		printf("Error. Tarjeta en modo 8 entradas analogicas. No se puede leer hasta la entrada %d\n",(int)parametros_tarjeta.canal_fin_ad);
		pthread_mutex_lock(&mut_errores);
		errores.er_critico_1 |= ERC_ECU_COM_TAD;
		pthread_mutex_unlock(&mut_errores);
	}
}

void configura_rangos(void)
{
	int i;
	int dig;
	
	for (i=0;i<NUM_ANALOG_IN; i++)
	{
		dig=(i<<4)+i;		
		out8(base+RANGE_CONTROL_POINTER, dig);
		out8(base+AD_RANGE_CONTROL, parametros_tarjeta.rango_ad[i]);
		
		switch (parametros_tarjeta.rango_ad[i])
		{
		/* Si es PCM-3718HG */
		/*
			case RANGO_BIPOLAR_0_5V:
				parametros_tarjeta.rango_ad_f[i] = -0.5;
			break;
			case RANGO_BIPOLAR_0_05V:
				parametros_tarjeta.rango_ad_f[i] = -0.05;
			break;
			case RANGO_BIPOLAR_0_005V:
				parametros_tarjeta.rango_ad_f[i] = -0.005;
			break;
			case RANGO_UNIPOLAR_1V:
				parametros_tarjeta.rango_ad_f[i] = 1;
			break;
			case RANGO_UNIPOLAR_0_1V:
				parametros_tarjeta.rango_ad_f[i] = 0.1;
			break;
			case RANGO_UNIPOLAR_0_01V:
				parametros_tarjeta.rango_ad_f[i] = 0.01;
			break;			
		*/
			case RANGO_BIPOLAR_5V:
				parametros_tarjeta.rango_ad_f[i] = -5;
			break;
			case RANGO_BIPOLAR_2_5V:
				parametros_tarjeta.rango_ad_f[i] = -2.5;
			break;
			case RANGO_BIPOLAR_1_25V:
				parametros_tarjeta.rango_ad_f[i] = -1.25;
			break;
			case RANGO_BIPOLAR_0_625V:
				parametros_tarjeta.rango_ad_f[i] = -0.625;
			break;
			case RANGO_UNIPOLAR_10V:
				parametros_tarjeta.rango_ad_f[i] = 10;
			break;
			case RANGO_UNIPOLAR_5V:
				parametros_tarjeta.rango_ad_f[i] = 5;
			break;
			case RANGO_UNIPOLAR_2_5V:
				parametros_tarjeta.rango_ad_f[i] = 2.5;
			break;			
			case RANGO_UNIPOLAR_1_25V:
				parametros_tarjeta.rango_ad_f[i] = 1.25;
			break;
			case RANGO_BIPOLAR_10V:
				parametros_tarjeta.rango_ad_f[i] = -10;
			break;
			case RANGO_BIPOLAR_1V:
				parametros_tarjeta.rango_ad_f[i] = -1;
			break;
			case RANGO_BIPOLAR_0_1V:
				parametros_tarjeta.rango_ad_f[i] = -0.1;
			break;
			case RANGO_BIPOLAR_0_01V:
				parametros_tarjeta.rango_ad_f[i] = -0.01;
			break;
			default:
		}
	}
}	

void selec_canal(void)
{
	uint8_t octeto;
	
	//Ponemos el ultimo canal en los cuatro bits mas significativos de octeto...
	octeto=parametros_tarjeta.canal_fin_ad;
	octeto<<=4;
	octeto&=0xf0;
	//... y el primero en los cuatro menos significativos.
	octeto|=(parametros_tarjeta.canal_inicio_ad&0x0f);
	
	out8(base+MUX_SCAN_CHANNEL, octeto);
}

void config_byte_ctrl(void)
{
	uint8_t octeto=0;
	
	parametros_tarjeta.trigger = SW_TRIGGER;

	octeto=(parametros_tarjeta.irq<<4)&0xF0;
	if (parametros_tarjeta.trigger==PACER_TRIGGER)
		octeto|=(INTE|PACER_TRIGGER);
	else if (parametros_tarjeta.trigger==SW_TRIGGER)
		octeto|=SW_TRIGGER;
	
	out8(base+CONTROL, octeto);
}

void habilitar_pacer(uint8_t pacer)
{
	out8(base+COUNTER_ENABLE, pacer);
}

void habilitar_ctrl_int_fifo_da(uint8_t ctrl)
{
	out8(base+FIFO_INTERRUPT_CONTROL, ctrl);
}

int lee_fichero_cfg(void)
{
	FILE *fp;
	char buffer[100], campo1[50], campo2[50];
	int flag=0, temp=0, i=0, j, mascara=0, temp2=0;
	long temp_long;
	
	fp=fopen(PATH_CONFIG_TAD, "r");
	
	if(fp==NULL)
	{
		printf("Error. No se encuentra pcm3718hg_ho.cfg\n");
		return(-1);
	}
	
	while(!feof(fp))
	{
		i++;
		
		fgets(buffer, sizeof(buffer), fp);
		
		if (buffer[0]=='#') //Si el primer caracter de la linea es una almohadilla esa linea no se lee.
			continue;
		else 
			temp=sscanf(buffer, "%s %s", campo1, campo2);
		if(temp>0&&temp!=2)
		{
			printf("Error. En la linea %d de pcm3718hg_ho.cfg\n", i);
			return(-1);
		}
		
		if (!strcmp(campo1, "DireccionTarjeta:"))
		{
			sscanf(campo2, "%x", &temp2);
			parametros_tarjeta.direccion=(uint64_t)temp2;
			if((parametros_tarjeta.direccion>=0x3f0)||(parametros_tarjeta.direccion%0x10))
			{
				printf("Error. En el campo %s, linea %d del archivo pcm3718hg_ho.cfg\n", campo1, i);
				return(-1);
			}
			flag|=(1<<0);
		}
		else if (!strcmp(campo1, "NumeroIrq:"))
		{
			sscanf(campo2, "%d", &temp2);
			parametros_tarjeta.irq=(uint8_t)temp2;
			if((parametros_tarjeta.irq<2)||(parametros_tarjeta.irq>7))
			{
				printf("Error. En el campo %s, linea %d del archivo pcm3718hg_ho.cfg\n", campo1, i);
				return(-1);
			}
			flag|=(1<<1);
		}
		else if (!strcmp(campo1, "RangoCanal0:"))
		{
			parametros_tarjeta.rango_ad[0]=rango_canal(campo2);
			if(parametros_tarjeta.rango_ad[0]>11)
			{
				printf("Error. En el campo %s, linea %d del archivo pcm3718hg_ho.cfg\n", campo1, i);
				return(-1);
			}
			flag|=(1<<2);
		}
		else if (!strcmp(campo1, "RangoCanal1:"))
		{
			parametros_tarjeta.rango_ad[1]=rango_canal(campo2);
			if(parametros_tarjeta.rango_ad[1]>11)
			{
				printf("Error. En el campo %s, linea %d del archivo pcm3718hg_ho.cfg\n", campo1, i);
				return(-1);
			}
			flag|=(1<<3);
		}
		else if (!strcmp(campo1, "RangoCanal2:"))
		{
			parametros_tarjeta.rango_ad[2]=rango_canal(campo2);
			if(parametros_tarjeta.rango_ad[2]>11)
			{
				printf("Error. En el campo %s, linea %d del archivo pcm3718hg_ho.cfg\n", campo1, i);
				return(-1);
			}
			flag|=(1<<4);
		}
		else if (!strcmp(campo1, "RangoCanal3:"))
		{
			parametros_tarjeta.rango_ad[3]=rango_canal(campo2);
			if(parametros_tarjeta.rango_ad[3]>11)
			{
				printf("Error. En el campo %s, linea %d del archivo pcm3718hg_ho.cfg\n", campo1, i);
				return(-1);
			}
			flag|=(1<<5);
		}
		else if (!strcmp(campo1, "RangoCanal4:"))
		{
			parametros_tarjeta.rango_ad[4]=rango_canal(campo2);
			if(parametros_tarjeta.rango_ad[4]>11)
			{
				printf("Error. En el campo %s, linea %d del archivo pcm3718hg_ho.cfg\n", campo1, i);
				return(-1);
			}
			flag|=(1<<6);
		}
		else if (!strcmp(campo1, "RangoCanal5:"))
		{
			parametros_tarjeta.rango_ad[5]=rango_canal(campo2);
			if(parametros_tarjeta.rango_ad[5]>11)
			{
				printf("Error. En el campo %s, linea %d del archivo pcm3718hg_ho.cfg\n", campo1, i);
				return(-1);
			}
			flag|=(1<<7);
		}
		else if (!strcmp(campo1, "RangoCanal6:"))
		{
			parametros_tarjeta.rango_ad[6]=rango_canal(campo2);
			if(parametros_tarjeta.rango_ad[6]>11)
			{
				printf("Error. En el campo %s, linea %d del archivo pcm3718hg_ho.cfg\n", campo1, i);
				return(-1);
			}
			flag|=(1<<8);
		}
		else if (!strcmp(campo1, "RangoCanal7:"))
		{
			parametros_tarjeta.rango_ad[7]=rango_canal(campo2);
			if(parametros_tarjeta.rango_ad[7]>11)
			{
				printf("Error. En el campo %s, linea %d del archivo pcm3718hg_ho.cfg\n", campo1, i);
				return(-1);
			}
			flag|=(1<<9);
		}
		else if (!strcmp(campo1, "RangoCanal8:"))
		{
			parametros_tarjeta.rango_ad[8]=rango_canal(campo2);
			if(parametros_tarjeta.rango_ad[8]>11)
			{
				printf("Error. En el campo %s, linea %d del archivo pcm3718hg_ho.cfg\n", campo1, i);
				return(-1);
			}
			flag|=(1<<10);
		}
		else if (!strcmp(campo1, "RangoCanal9:"))
		{
			parametros_tarjeta.rango_ad[9]=rango_canal(campo2);
			if(parametros_tarjeta.rango_ad[9]>11)
			{
				printf("Error. En el campo %s, linea %d del archivo pcm3718hg_ho.cfg\n", campo1, i);
				return(-1);
			}
			flag|=(1<<11);
		}
		else if (!strcmp(campo1, "RangoCanal10:"))
		{
			parametros_tarjeta.rango_ad[10]=rango_canal(campo2);
			if(parametros_tarjeta.rango_ad[10]>11)
			{
				printf("Error. En el campo %s, linea %d del archivo pcm3718hg_ho.cfg\n", campo1, i);
				return(-1);
			}
			flag|=(1<<12);
		}
		else if (!strcmp(campo1, "RangoCanal11:"))
		{
			parametros_tarjeta.rango_ad[11]=rango_canal(campo2);
			if(parametros_tarjeta.rango_ad[11]>11)
			{
				printf("Error. En el campo %s, linea %d del archivo pcm3718hg_ho.cfg\n", campo1, i);
				return(-1);
			}
			flag|=(1<<13);
		}
		else if (!strcmp(campo1, "RangoCanal12:"))
		{
			parametros_tarjeta.rango_ad[12]=rango_canal(campo2);
			if(parametros_tarjeta.rango_ad[12]>11)
			{
				printf("Error. En el campo %s, linea %d del archivo pcm3718hg_ho.cfg\n", campo1, i);
				return(-1);
			}
			flag|=(1<<14);
		}
		else if (!strcmp(campo1, "RangoCanal13:"))
		{
			parametros_tarjeta.rango_ad[13]=rango_canal(campo2);
			if(parametros_tarjeta.rango_ad[13]>11)
			{
				printf("Error. En el campo %s, linea %d del archivo pcm3718hg_ho.cfg\n", campo1, i);
				return(-1);
			}
			flag|=(1<<15);
		}
		else if (!strcmp(campo1, "RangoCanal14:"))
		{
			parametros_tarjeta.rango_ad[14]=rango_canal(campo2);
			if(parametros_tarjeta.rango_ad[14]>11)
			{
				printf("Error. En el campo %s, linea %d del archivo pcm3718hg_ho.cfg\n", campo1, i);
				return(-1);
			}
			flag|=(1<<16);
		}
		else if (!strcmp(campo1, "RangoCanal15:"))
		{
			parametros_tarjeta.rango_ad[15]=rango_canal(campo2);
			if(parametros_tarjeta.rango_ad[15]>11)
			{
				printf("Error. En el campo %s, linea %d del archivo pcm3718hg_ho.cfg\n", campo1, i);
				return(-1);
			}
			flag|=(1<<17);
		}
		else if (!strcmp(campo1, "CanalInicioLectura:"))
		{
			sscanf(campo2, "%d", &temp2);
			if(temp2>15||temp2<0)
			{
				printf("Error. En el campo %s, linea %d del archivo pcm3718hg_ho.cfg\n", campo1, i);
				return(-1);
			}
			parametros_tarjeta.canal_inicio_ad=(uint8_t)temp2;
			flag|=(1<<18);
		}
		else if (!strcmp(campo1, "CanalFinLectura:"))
		{
			sscanf(campo2, "%d", &temp2);
			if(temp2>15||temp2<parametros_tarjeta.canal_inicio_ad)
			{
				printf("Error. En el campo %s, linea %d del archivo pcm3718hg_ho.cfg\n", campo1, i);
				return(-1);
			}
			parametros_tarjeta.canal_fin_ad=(uint8_t)temp2;
			flag|=(1<<19);
		}
		else if (!strcmp(campo1, "DigByte0:"))
		{
			sscanf(campo2, "%x", &temp2);
			parametros_tarjeta.dio_0=(uint8_t)temp2;
			flag|=(1<<20);
		}
		else if (!strcmp(campo1, "DigByte1:"))
		{
			sscanf(campo2, "%x", &temp2);
			parametros_tarjeta.dio_1=(uint8_t)temp2;
			flag|=(1<<21);
		}
		else if (!strcmp(campo1, "Periodo:"))
		{
			sscanf(campo2, "%d", &temp2);
			temp_long=(long)temp2;
			if(temp_long>=1000000||temp_long<0)
			{
				printf("Error. En el campo %s, linea %d del archivo pcm3718hg_ho.cfg\n", campo1, i);
				return(-1);
			}
			parametros_tarjeta.periodo.tv_nsec=1000*temp_long;
			parametros_tarjeta.periodo.tv_sec=0;
			flag|=(1<<22);
		}
		
		for(j=0;j<23;j++)
			mascara|=1<<j;
		if((~mascara)&flag)
		{
			printf("Error. Faltan campos por completar en el archivo pcm3718hg_ho.cfg\n");
			return(-1);
		}
	}
	
	return(0);
	
}

uint8_t rango_canal(char *cadena)
{
	if(!strcmp(cadena,"B5"))
		return (RANGO_BIPOLAR_5V);
	if(!strcmp(cadena,"B05"))
		return (RANGO_BIPOLAR_0_5V);
	if(!strcmp(cadena,"B25"))
		return (RANGO_BIPOLAR_2_5V);		
	if(!strcmp(cadena,"B005"))
		return (RANGO_BIPOLAR_0_05V);
	if(!strcmp(cadena,"B125"))
		return (RANGO_BIPOLAR_1_25V);
	if(!strcmp(cadena,"B0005"))
		return (RANGO_BIPOLAR_0_005V);
	if(!strcmp(cadena,"B0625"))
		return (RANGO_BIPOLAR_0_625V);
	if(!strcmp(cadena,"U10"))
		return (RANGO_UNIPOLAR_10V);
	if(!strcmp(cadena,"U1"))
		return (RANGO_UNIPOLAR_1V);
	if(!strcmp(cadena,"U5"))
		return (RANGO_UNIPOLAR_5V);
	if(!strcmp(cadena,"U01"))
		return (RANGO_UNIPOLAR_0_1V);
	if(!strcmp(cadena,"U25"))
		return (RANGO_UNIPOLAR_2_5V);
	if(!strcmp(cadena,"U001"))
		return (RANGO_UNIPOLAR_0_01V);
	if(!strcmp(cadena,"U125"))
		return (RANGO_UNIPOLAR_1_25V);
	if(!strcmp(cadena,"B10"))
		return (RANGO_BIPOLAR_10V);	
	if(!strcmp(cadena,"B1"))
		return (RANGO_BIPOLAR_1V);
	if(!strcmp(cadena,"B01"))
		return (RANGO_BIPOLAR_0_1V);
	if(!strcmp(cadena,"B001"))
		return (RANGO_BIPOLAR_0_01V);
	
	return(12);	
}

