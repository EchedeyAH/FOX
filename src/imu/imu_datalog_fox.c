/**************************************************************************
***									***
***	Fichero: imu_datalog_fox.c						***
***	Fecha: 26/11/2013						***
***	Autor: Elena Gonzalez						***
***	Descripción: Proceso de datalogging para la imu		***
***	Recibe datos de la imu por cola y los almacena en un fichero.			***
***									***
**************************************************************************/

// CABECERAS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include <hw/inout.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/neutrino.h>
#include <stdint.h>
#include <sys/mman.h>
#include <mqueue.h>
/* Include local */
#include "./include/imu_datalog_fox.h"

/**************************************************
***		  VARIABLES GLOBALES		***
**************************************************/
/* Nombre raiz del fichero para los logs */
char nombre_archivo[100];
/* Punteros a ficheros */
FILE *fp_imu;

/******************************************************************************************
***				DEFINICION FUNCION: main				***
*** Configura e inicia la cola de mensajes para la recepcion de datos de la imu.	 ***
*** Almacena los datos recibidos en un fichero.	***
******************************************************************************************/
int main(void)
{
	/**************************************************
	***		VARIABLES			***
	**************************************************/
	int i = 0;
	int errvalue = EOK;
	int fin = 0;
	int cont_datos = 0;
	int error_fichero = 0;
	struct timespec espera = {0, TIEMPO_BUCLE_NS};
	int cont_error_cola = 0;
	/* Variables para cola de mensajes */
	dato_cola_t dato_cola;
	mqd_t cola;
	struct mq_attr atrib_cola;
	int estado_cola = 0;
	struct timespec timeout_cola;
	int num_msgs_cola = 0;
	
	/**************************************************
	***		COLA DE MENSAJES						***
	**************************************************/
	/* Creacion de la cola de mensajes */
	cola = mq_open(NOMBRE_COLA, O_RDWR, S_IRWXU, NULL);
	printf("Creacion de cola, proceso imu. cola=%d - fin=%d\n", cola, fin);

	error_fichero = crea_nuevo_fichero();

	while(!fin)
	{
		/**************************************************
		***		RECIBE DATOS DE LA COLA DE MENSAJES		***
		**************************************************/
		/* Recibe datos de la cola de mensajes desde el proceso principal */
		/* Obtiene el tiempo actual */
		clock_gettime(CLOCK_REALTIME, &timeout_cola);
		/* Establece el tiempo absoluto para el timeout de la cola */
		timeout_cola.tv_sec += TIEMPO_EXP_COLA_S;

		estado_cola = mq_timedreceive(cola, (char *)&dato_cola, sizeof(dato_cola), NULL, &timeout_cola);
		
		if (estado_cola < 0)
		{
			errvalue = errno;
			printf("errno: %d\n", errvalue);
			if (errvalue == ETIMEDOUT)
			{
				printf("Error Timeout cola - proc imu\n");
				fin = 1;	/* Error de timeout - fin de programa */
			}
			else
			{
				printf("Error de cola - proc imu\n");
				cont_error_cola++;
			}
		}
		else
		{
			estado_cola = mq_getattr(cola, &atrib_cola);					// Actualiza los atributos de la cola creada en el otro extremo...
			num_msgs_cola = atrib_cola.mq_curmsgs;		// ... para conocer el numero de mensajes que tiene que recibir.
			/* Si hay datos que recibir */
			cont_error_cola = 0;
			for(i=0; i<num_msgs_cola; i++)
			{
				//printf("Dato recibido por cola imu - proc imu\n");
				/* Si alcanzamos el num maximo de datos por fichero, creamos un nuevo fichero */
				if (cont_datos > NUM_MAX_DATOS_FICHERO)
				{
					error_fichero = crea_nuevo_fichero();
					cont_datos = 0;
				}
				if (error_fichero == 0)	// Si no hay error de fichero
				{
					error_fichero = escribe_dato(&dato_cola); /* Cada vez que recibe un dato correcto, lo escribe en el fichero correspondiente */
					cont_datos++;
				}
				else
					printf("Error fichero - proc imu\n"); 
			} // Fin for - recorre mensajes cola
		}
		
		if (cont_error_cola > ER_CONT_COLA)
		{
			/* Error en la cola */
			printf("Error de cola - proc imu\n");
//			printf("cont_errores=%d\n", cont_error_cola);
			/* Salimos del programa */
			fin = 1;
		}
//		printf("fin = %d\n", fin);
		nanosleep(&espera, NULL);
	} // Fin bucle while
	
	/* Cierre ordenado del programa */
	mq_close(cola);			// Cierre de la cola de mensajes
	mq_unlink(NOMBRE_COLA);		// Elimina cola de mensajes
	
	return 0;
}

int crea_nuevo_fichero (void)
{
	/* Variables para recogida de la fecha actual */
	struct tm fecha, *act_tm;
	time_t momento;
	char nombre_fecha[20];
	int error = 0;
	
	/* Recogida de la fecha actual */
	momento = time(NULL);
	act_tm = localtime_r(&momento, &fecha);	// Recoge fecha y hora
	/* Creamos el nombre del fichero de logs */
	sprintf(nombre_fecha, "-%04d%02d%02d_%02d_%02d.csv", fecha.tm_year+1900, fecha.tm_mon+1, fecha.tm_mday, fecha.tm_hour, fecha.tm_min);
	strcpy (nombre_archivo, NOMBRE_RAIZ);
	
	/* Concatenamos con el nombre raiz del fichero para aniadirle la fecha */
	strcat (nombre_archivo, nombre_fecha);
	
	printf("Nombre inicio tras concatenar: %s\n", nombre_archivo);

	/* Escribe las cabeceras en el nuevo fichero */
	error = escribe_cabeceras();
	
	return error;

}

int escribe_cabeceras(void)
{
	int error = 0;		// Indica error con la apertura de ficheros
	
	/* Abre el fichero para la escritura */
	fp_imu = fopen(nombre_archivo, "a");
	if (fp_imu != NULL)
	{
		/* Imprime la cabecera del fichero */
		fprintf(fp_imu, "Timestamp\tAccel_1\tAccel_2\tAccel_3\tGyro_1\tGyro_2\tGyro_3\tLLHpos_1\tLLHpos_2\tLLHpos_3\tLLHpos_4\t");
		fprintf(fp_imu, "Euler_1\tEuler_2\tEuler_3\tNEDveloc_1\tNEDveloc_2\tNEDveloc_3\tNEDveloc_4\tNEDveloc_5\tNEDveloc_6\t");
		fprintf(fp_imu,	"ERROR_C\tERROR_G\tERROR_L\n");
		/* Cierra el fichero tras la escritura */
		fclose(fp_imu);
	}
	else
		error = -1;

	return error;	
}


int escribe_dato (dato_cola_t *dato)
{
	int error = 0;		// Indica error con la apertura de ficheros
	
	/* Abre el fichero para la escritura */
	fp_imu = fopen(nombre_archivo, "a");
	if (fp_imu != NULL)
	{
		/* Imprime los datos recibidos de la imu en el fichero */
		fprintf(fp_imu, "%u\t%.2f\t%.2f\t%.2f\t", dato->datosImu.iTimeStamp, dato->datosImu.pfAccelScal[0], dato->datosImu.pfAccelScal[1], dato->datosImu.pfAccelScal[2]);
		fprintf(fp_imu, "%.2f\t%.2f\t%.2f\t", dato->datosImu.pfGyroScal[0], dato->datosImu.pfGyroScal[1], dato->datosImu.pfGyroScal[2]);
		fprintf(fp_imu, "%.2f\t%.2f\t%.2f\t%.2f\t", dato->datosImu.pdLLHPos[0], dato->datosImu.pdLLHPos[1], dato->datosImu.pdLLHPos[2], dato->datosImu.pdLLHPos[3]);
		fprintf(fp_imu, "%.2f\t%.2f\t%.2f\t", dato->datosImu.pfEuler[0], dato->datosImu.pfEuler[1], dato->datosImu.pfEuler[2]);
		fprintf(fp_imu, "%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t", dato->datosImu.pfNEDVeloc[0], dato->datosImu.pfNEDVeloc[1], dato->datosImu.pfNEDVeloc[2], dato->datosImu.pfNEDVeloc[3], dato->datosImu.pfNEDVeloc[4], dato->datosImu.pfNEDVeloc[5]);
		fprintf(fp_imu, "%02X\t%04X\t%04X\n", dato->errores_criticos, dato->errores_graves, dato->errores_leves);
		/* Cierra el fichero tras la escritura */
		fclose(fp_imu);
	}
	else
		error = -1;

	return error;	
}
