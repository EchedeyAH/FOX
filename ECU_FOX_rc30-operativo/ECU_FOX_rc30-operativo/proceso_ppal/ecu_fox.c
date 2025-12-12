/**********************************************************************************
***													***
***	Fichero: ecu_fox.c								***
***	Fecha: 10/10/2013								***
***	Autor: Elena Gonzalez							***
***	Descripción: Fichero principal de la ECU. 		***
***	Se encarga de lanzar los demas hilos y recoger y adaptar los datos 	***
***													***
**********************************************************************************/

/* CABECERAS */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <hw/inout.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/neutrino.h>
#include <stdint.h>
#include <sys/mman.h>
#include <mqueue.h>
#include <pthread.h>
#include <process.h>
#include <sched.h>
/* Include local */
#include "./include/constantes_fox.h"
#include "./include/estructuras_fox.h"
#include "./include/declaraciones_fox.h"

/**************************************************
***		  VARIABLES GLOBALES		***
**************************************************/
/* Variables globales */
uint16_t hilos_listos;

/* Estructuras globales */
est_superv_t supervisor;
est_veh_t vehiculo;
est_bat_t bateria;
est_motor_t motor1;
est_motor_t motor2;
est_motor_t motor3;
est_motor_t motor4;
est_error_t errores;
est_pot_t potencias;
datosImu_t tDatosImu;

/* Mutex */
pthread_mutex_t mut_hilos_listos = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mut_inicio = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t inicio = PTHREAD_COND_INITIALIZER;

pthread_mutex_t mut_supervisor = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mut_vehiculo = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mut_bateria = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mut_motor1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mut_motor2 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mut_motor3 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mut_motor4 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mut_errores = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mut_potencias = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mDatosImu = PTHREAD_MUTEX_INITIALIZER;

// FUNCION PRINCIPAL
int main (int argc, char **argv)
{
	/**************************************************
	***			VARIABLES 					***
	**************************************************/
	uint16_t hilos_listos_local = 0;
	uint16_t cont_espera_hilos = 0;
	uint16_t cont_espera_fin = 0;
	uint16_t cont_bucle = 0;
	BOOL fin_local = FALSE;
	BOOL fin_main = FALSE;
	int i;
	time_t tiempo;
	/* Seleccion de procesos e hilos en funcion de los argumentos */
	uint16_t seleccion = OFF_TODOS;
	uint16_t selec_aux = OFF_TODOS;
	uint16_t imprimir = OFF_TODOS;
	char opcion[10];
	/* Variables relacionadas con los hilos */
	int er, im, ad, adao, cr, cs, gp, ct, ce;	// Variables para crear todos los hilos
	int cm1 = POS_M1, cm2 = POS_M2, cm3 = POS_M3, cm4 = POS_M4;
	
	pthread_t thr_errores;		// Hilo para comprobar errores
	pthread_t thr_imu;			// Hilo de adquisicion de datos de la IMU
	pthread_t thr_adq;			// Hilo de adquisicion de datos (ADQ)
	pthread_t thr_adq_AO;		// Hilo de adquisicion de datos de salidas analogicas
	pthread_t thr_can_rx;		// Hilo de recepcion de mensajes CAN por canal 1 a buffers
	pthread_t thr_can_superv;	// Hilo de envio y recepcion de mensajes CAN del supervisor
	pthread_t thr_canM1;		// Hilo de envio y recepcion de mensajes CAN del motor 1
	pthread_t thr_canM2;		// Hilo de envio y recepcion de mensajes CAN del motor 2
	pthread_t thr_canM3;		// Hilo de envio y recepcion de mensajes CAN del motor 3
	pthread_t thr_canM4;		// Hilo de envio y recepcion de mensajes CAN del motor 4
	pthread_t thr_gestion_potencia;	// Hilo para gestion de potencia
	pthread_t thr_ctrl_traccion;	// Hilo para control de traccion
	pthread_t thr_ctrl_estabilidad;	// Hilo para control de estabilidad
	struct timespec espera_hilos={0, TIEMPO_INI_HILOS_NS};	// Tiempo (500ms) para espera de hilos
	struct timespec espera_bucle={0, TIEMPO_BUCLE_NS};  	// Tiempo base para espera de bucle principal 50 ms
	struct timespec espera_fin={TIEMPO_FIN_S, 0};	// Espera 3 segundos antes de matar los otros procesos
	struct timespec espera_can={TIEMPO_DRV_CAN_S, 0};
	struct timespec espera_proc={TIEMPO_PROC_S, 0};
	/* Datos colas de mensajes - comunicacion con proceso can2 */
	mqd_t cola_can2;
	struct mq_attr atrib_cola_can2;
	/* Datos colas de mensajes - comunicacion con proceso imu datalog */
	mqd_t cola_imu;
	struct mq_attr atrib_cola_imu;
	
	/* VARIABLES DE MENSAJES DE INICIO */
	const char ini_mensaje[] = MENSAJE_INICIO;
	const char fin_mensaje[] = MENSAJE_FIN;
	const char version_mensaje[] = MENSAJE_VERSION;
	float version_ECU = (float) VERSION_ECU/100;
	/* VARIABLES PARA BLOQUEAR SENIALES */
	sigset_t cjto;
	
	/* Variables de control de procesos */
	pid_t pid_candual;	// Contiene el pid del proceso del can dual (driver can)
	pid_t pid_imu;		// Contiene el pid del proceso de datalogging de la imu
	pid_t pid_can2;		// Contiene el pid del proceso del can2
	
	/**************************************************
	***			INICIALIZACION 					***
	**************************************************/
	hilos_listos = HILOS_OFF;
	memset(&supervisor, 0, sizeof(est_superv_t));
	memset(&vehiculo, 0, sizeof(est_veh_t));
	memset(&bateria, 0, sizeof(est_bat_t));
	memset(&motor1, 0, sizeof(est_motor_t));
	memset(&motor2, 0, sizeof(est_motor_t));
	memset(&motor3, 0, sizeof(est_motor_t));
	memset(&motor4, 0, sizeof(est_motor_t));
	memset(&errores, 0, sizeof(est_error_t));
	memset(&potencias, 0, sizeof(est_pot_t));
	memset(&tDatosImu, 0, sizeof(datosImu_t));
	/* Si no hay Supervisor externo, la propia ECU levanta los motores */
	supervisor.heartbeat = TRUE;
	supervisor.on = TRUE;
	for (i = 0; i < NUM_MOTORES; ++i)
		supervisor.motor_on[i] = TRUE;
	
	/**************************************************
	***	BLOQUEA SEGNALES DEL PROCESO		***
	**************************************************/
	/* Crea la mascara de forma que se bloqueen SIGRTMIN, SIGRTMIN+1 Y SIGRTMIN+2 */
	/* De esta forma, cuando lleguen dichas seniales, se encolan. */
	sigemptyset(&cjto);
	sigaddset(&cjto, SIG_CAN_SUP);
	pthread_sigmask(SIG_BLOCK, &cjto, NULL);	
	
	/**************************************************
	***	SELECCION DE LOS PROCESOS E HILOS			***
	**************************************************/
	if (argc > 1)
		for (i=1;i<argc;i++)
		{
			if ((argv[i][0] == '-') || (argv[i][0] == 'p'))
				strcpy(opcion,argv[i]+1);
			else
				strcpy(opcion,argv[i]);

			if (!strcmp(opcion,"todo")) selec_aux = ON_TODOS;
			else if (!strcmp(opcion,"imudata")) selec_aux = ON_PROC_IMUDATA;
			else if (!strcmp(opcion,"imprime")) selec_aux = ON_IMPRIME;
			else if (!strcmp(opcion,"canbms")) selec_aux = ON_PROC_CANBMS;
			else if (!strcmp(opcion,"errores")) selec_aux = ON_HILO_ERRORES;
			else if (!strcmp(opcion,"imu")) selec_aux = ON_HILO_IMU;
			else if (!strcmp(opcion,"adq")) selec_aux = ON_HILO_ADQ;
			else if (!strcmp(opcion,"adqao")) selec_aux = ON_HILO_ADQAO;
			else if (!strcmp(opcion,"canrx")) selec_aux = ON_HILO_CANRX;
			else if (!strcmp(opcion,"cansuperv")) selec_aux = ON_HILO_CANSUPERV;
			else if (!strcmp(opcion,"canmotor1")) selec_aux = ON_HILO_CANMOTOR1;
			else if (!strcmp(opcion,"canmotor2")) selec_aux = ON_HILO_CANMOTOR2;
			else if (!strcmp(opcion,"canmotor3")) selec_aux = ON_HILO_CANMOTOR3;
			else if (!strcmp(opcion,"canmotor4")) selec_aux = ON_HILO_CANMOTOR4;
			else if (!strcmp(opcion,"gestpot")) selec_aux = ON_HILO_GESTPOT;
			else if (!strcmp(opcion,"ctrltrac")) selec_aux = ON_HILO_CTRLTRAC;
			else if (!strcmp(opcion,"ctrlest")) selec_aux = ON_HILO_CTRLEST;
			else
			{
				printf("Opcion incorrecta\n");
				return(-1);
			}
			
			if (argv[i][0] == '-')
			{
				seleccion &= ~selec_aux;
				imprimir &= ~selec_aux;
			}
			else if (argv[i][0] == 'p')
			{
				imprimir |= selec_aux;
				seleccion |= selec_aux;
			} else
				seleccion |= selec_aux;
		}
	else 
	{
		seleccion = ON_TODOS;
		imprimir = OFF_TODOS;
	}

	/**************************************************
	***		COLA DE MENSAJES						***
	**************************************************/
	/* Destruir las colas en caso de haberlas */
	mq_unlink(NOMBRE_COLA_CAN2);
	mq_unlink(NOMBRE_COLA_IMU);
		
	/* Configura los atributos para las colas */
	atrib_cola_can2.mq_maxmsg=TAMANHO_COLA_CAN2;
	atrib_cola_can2.mq_msgsize=sizeof(est_bat_t);
	atrib_cola_can2.mq_flags=0;
	atrib_cola_imu.mq_maxmsg=TAMANHO_COLA_IMU;
	atrib_cola_imu.mq_msgsize=sizeof(dato_cola_imu_t);
	atrib_cola_imu.mq_flags=0;
	
	/* Creacion de la cola de mensajes */
	if ((seleccion&ON_PROC_CANBMS) != OFF_TODOS)
	{
		cola_can2 = mq_open(NOMBRE_COLA_CAN2, O_RDWR|O_CREAT|O_NONBLOCK, S_IRWXU, &atrib_cola_can2);
		if (cola_can2 == (mqd_t)(-1))
		{
			printf("Error main: apertura de cola can2\n");
			pthread_mutex_lock(&mut_errores);
			errores.er_critico_1 |= ERC_CAN2_COM_COLA;
			pthread_mutex_unlock(&mut_errores);
		}
	}

	if ((seleccion&ON_PROC_IMUDATA) != OFF_TODOS)
	{	
		cola_imu = mq_open(NOMBRE_COLA_IMU, O_RDWR|O_CREAT|O_NONBLOCK, S_IRWXU, &atrib_cola_imu);
		if (cola_imu == (mqd_t)(-1))
		{
			printf("Error main: apertura de cola imu\n");
			printf("Error apertura cola: %d\n", errno);
			pthread_mutex_lock(&mut_errores);
			errores.er_grave_2 |= ERG_IMU_COM_COLA;
			pthread_mutex_unlock(&mut_errores);
		}
	}
	
	/**************************************************
	***			CREACION DE PROCESOS HIJO			***
	**************************************************/
	/* Creamos el proceso del driver can */
	if (((seleccion&ON_HILO_CANRX) != OFF_TODOS)||((seleccion&ON_PROC_CANBMS) != OFF_TODOS))
	{
		pid_candual = fork();
		if (pid_candual == 0)
		//Canal 1 (Superv. y motores): 1 Mbps.
		//Canal 2 (BMS): 500 Kbps (p. def.)
			execlp(PATH_DRIVER_CAN, PATH_DRIVER_CAN, "-B", "0x00", "-b", "0x14", NULL);
		/* Esperamos a que funcione el driver can */
		nanosleep(&espera_can, NULL);
	}
	
	/* Creamos el proceso hijo para el proceso can2 */
	if ((seleccion&ON_PROC_CANBMS) != OFF_TODOS)
	{
		pid_can2 = fork();
		if(pid_can2 == 0)
			execlp(PATH_PROC_CAN2, PATH_PROC_CAN2, NULL);		/* Proceso hijo - proceso de can2 */
		/* Establece la prioridad del hijo - Proceso imu */
		setprio(pid_imu, PRIO_CAN2);
		nanosleep(&espera_proc, NULL);
	}
	
	/* Creamos el proceso hijo para el proceso imu */
	if ((seleccion&ON_PROC_IMUDATA) != OFF_TODOS)
	{		
		pid_imu = fork();
		if (pid_imu == 0)
			execlp(PATH_PROC_IMU, PATH_PROC_IMU, NULL);		/* Proceso hijo - proceso de la imu */
		/* Establece la prioridad del hijo - Proceso imu */
		setprio(pid_imu, PRIO_IMU);
		nanosleep(&espera_proc, NULL);
	}
	
	/**************************************************
	***			CREACION Y ESPERA DE HILOS			***
	**************************************************/
	/* Primero los hilos basicos */
	if ((seleccion&ON_HILO_ADQ) != OFF_TODOS)
		pthread_create(&thr_adq, NULL, adq_main, (void *)&ad);
	if ((seleccion&ON_HILO_ADQAO) != OFF_TODOS)
		pthread_create(&thr_adq_AO, NULL, adq_ao, (void *)&adao);
	if ((seleccion&ON_HILO_CANRX) != OFF_TODOS)
		pthread_create(&thr_can_rx, NULL, can_rx, (void *)&cr);
	if ((seleccion&ON_HILO_IMU) != OFF_TODOS)
		pthread_create(&thr_imu, NULL, imuMain, (void *)&im);

	printf("Esperando hilos basicos...\n");
	while ((hilos_listos_local != (HILOS_BASE_OK&seleccion)) && (cont_espera_hilos <= TIEMPO_INI_HILOS_X))
	{
		pthread_mutex_lock(&mut_hilos_listos);
		hilos_listos_local = hilos_listos;
		pthread_mutex_unlock(&mut_hilos_listos);
		nanosleep(&espera_hilos,NULL);
		cont_espera_hilos++;
	}
	/* Si hay error en arranque de hilos basicos */
	if (cont_espera_hilos > TIEMPO_INI_HILOS_X)
	{
		pthread_mutex_lock(&mut_errores);
		errores.er_critico_1 |= ERC_ECU_HILOS;
		pthread_mutex_unlock(&mut_errores);
	}
	cont_espera_hilos = 0;
	
	if ((seleccion&ON_HILO_CANMOTOR1) != OFF_TODOS)
		pthread_create(&thr_canM1, NULL, canMotor, (void *)&cm1);
	if ((seleccion&ON_HILO_CANMOTOR2) != OFF_TODOS)
		pthread_create(&thr_canM2, NULL, canMotor, (void *)&cm2);
	if ((seleccion&ON_HILO_CANMOTOR3) != OFF_TODOS)
		pthread_create(&thr_canM3, NULL, canMotor, (void *)&cm3);
	if ((seleccion&ON_HILO_CANMOTOR4) != OFF_TODOS)
		pthread_create(&thr_canM4, NULL, canMotor, (void *)&cm4);	
	
	printf("Esperando hilos de los motores...\n");
	while ((hilos_listos_local != ((HILOS_BASE_OK|HILOS_MOTORES_OK)&seleccion)) && (cont_espera_hilos <= TIEMPO_INI_HILOS_X))
	{
		pthread_mutex_lock(&mut_hilos_listos);
		hilos_listos_local = hilos_listos;
		pthread_mutex_unlock(&mut_hilos_listos);
		nanosleep(&espera_hilos,NULL);
		cont_espera_hilos++;
	}
	/* Si hay error en arranque de hilos motores */
	if (cont_espera_hilos > TIEMPO_INI_HILOS_X)
	{
		pthread_mutex_lock(&mut_errores);
		errores.er_critico_1 |= ERC_ECU_HILOS;
		pthread_mutex_unlock(&mut_errores);
	}
	cont_espera_hilos = 0;
	
	if ((seleccion&ON_HILO_ERRORES) != OFF_TODOS)
		pthread_create(&thr_errores, NULL, errores_main, (void *)&er);
	if ((seleccion&ON_HILO_CANSUPERV) != OFF_TODOS)
		pthread_create(&thr_can_superv, NULL, can_superv, (void *)&cs);
	if ((seleccion&ON_HILO_GESTPOT) != OFF_TODOS)
		pthread_create(&thr_gestion_potencia, NULL, gestion_potencia_main, (void *)&gp);
	if ((seleccion&ON_HILO_CTRLTRAC) != OFF_TODOS)
		pthread_create(&thr_ctrl_traccion, NULL, ctrl_traccion_main, (void *)&ct);
	if ((seleccion&ON_HILO_CTRLEST) != OFF_TODOS)
		pthread_create(&thr_ctrl_estabilidad, NULL, ctrl_estabilidad_main, (void *)&ce);
	
	/* ESPERAMOS A QUE TODOS LOS HILOS ESTEN OPERATIVOS */
	printf("Esperando hilos secundarios...\n");
	while ((hilos_listos_local != (HILOS_OK&seleccion)) && (cont_espera_hilos <= TIEMPO_INI_HILOS_X))
	{
		pthread_mutex_lock(&mut_hilos_listos);
		hilos_listos_local = hilos_listos;
		pthread_mutex_unlock(&mut_hilos_listos);
		nanosleep(&espera_hilos,NULL);
		cont_espera_hilos++;
	}
	/* Si hay error en arranque de hilos */
	if (cont_espera_hilos > TIEMPO_INI_HILOS_X)
	{
		pthread_mutex_lock(&mut_errores);
		errores.er_critico_1 |= ERC_ECU_HILOS;
		pthread_mutex_unlock(&mut_errores);
	}
	cont_espera_hilos = 0;
	
	/* MENSAJE INICIO */
	printf("%s\n",ini_mensaje);
	printf("%s%.2f\n", version_mensaje, version_ECU);
	/* Notifico al resto de hilos que ya estan todos iniciados para que continuen */
	pthread_mutex_lock(&mut_inicio);
	pthread_cond_broadcast (&inicio);
	pthread_mutex_unlock(&mut_inicio);

	
	/* Comprobacion de error antes de iniciar el bucle */
//----------->darle valor a fin_local desde errores
//----------->hacerlo para todos los bucles de los hilos
	if (fin_local != FALSE)
		fin_main = TRUE;

	/* BUCLE PRINCIPAL */
	while (!fin_main)
	{
		//------------>Meter control de la espera
		/* RECIBE DATOS DE LA COLA DE MENSAJES */
		if ((seleccion&ON_PROC_CANBMS) != OFF_TODOS)
			recibe_datos_cola_can2(cola_can2);

		/* ENVIA DATOS A LA COLA DE MENSAJES */
		if ((seleccion&ON_PROC_IMUDATA) != OFF_TODOS)
			envia_datos_cola_imu(cola_imu);
	
		// Calculo velocidad lineal
		// Calculo hidrogeno
		// Comprobacion sentido marcha
		// Calculo estado borroso bateria

		/* Imprime fecha y hora cada minuto */
		if ((cont_bucle % 1200) == 0)
		{
			tiempo = time(NULL);
			printf("%s\n",asctime(localtime(&tiempo)));
		}
		
		/* Impresion de datos por pantalla */
		if (((seleccion&ON_IMPRIME) != OFF_TODOS)&&((cont_bucle % 20) == 0))		
			imprime_pantalla(imprimir);		
		
		// ESPERA DEL BUCLE PRINCIPAL
		nanosleep(&espera_bucle,NULL);
		
		/* Si ha habido un error critico que cierre el programa, salimos del bucle principal */
		
		if (fin_local != 0)
			cont_espera_fin++;
		
		if(cont_espera_fin > 100)
			fin_main = 1;
			
		cont_bucle ++;
	} // Fin bucle principal

	// Imprime mensaje de fin de programa
	printf("%s\n",fin_mensaje);

	// CIERRA TODOS LOS HILOS
	// (Devuelven error si los hilos no se han creado,
	// pero no detienen el programa)
	pthread_cancel(thr_errores);
	pthread_cancel(thr_imu);
	pthread_cancel(thr_adq);
	pthread_cancel(thr_adq_AO);
	pthread_cancel(thr_can_rx);
	pthread_cancel(thr_can_superv);
	pthread_cancel(thr_canM1);
	pthread_cancel(thr_canM2);
	pthread_cancel(thr_canM3);
	pthread_cancel(thr_canM4);	
	pthread_cancel(thr_gestion_potencia);
	pthread_cancel(thr_ctrl_traccion);
	pthread_cancel(thr_ctrl_estabilidad);
	
	/* Matamos los procesos */
	if ((seleccion&ON_HILO_CANRX) != OFF_TODOS)
		kill(pid_candual, SIGKILL);
	if ((seleccion&ON_PROC_IMUDATA) != OFF_TODOS)
		kill(pid_imu, SIGKILL);
	if ((seleccion&ON_PROC_CANBMS) != OFF_TODOS)
		kill(pid_can2, SIGKILL);

	nanosleep(&espera_fin, NULL);
	
	/* Cierre de la cola de mensajes */
	if ((seleccion&ON_PROC_CANBMS) != OFF_TODOS)
	{
		mq_close(cola_can2);			// Cierre de la cola de mensajes
		mq_unlink(NOMBRE_COLA_CAN2);		// Elimina cola de mensajes	
	}
	if ((seleccion&ON_PROC_IMUDATA) != OFF_TODOS)
	{
		mq_close(cola_imu);
		mq_unlink(NOMBRE_COLA_IMU);
	}

	/* FIN DE PROCESO PRINCIPAL */
	return(1);
}

//----------------->Esta funcion se puede mejorar, no tiene mucho sentido
//----------------->poner timeout si ya se sabe cuantos mensajes hay
//----------------->ni el copiarlos todos de golpe, solo interesaria el ultimo
void recibe_datos_cola_can2(mqd_t cola)
{
	/* Variables */
	int i;
	int num_msgs_cola = 0;	// Para contabilizar el numero de mensajes a recibir de la cola
	int estado_cola = 0;
	struct timespec timeout_cola;
	/* Para la cola de mensajes */
	struct mq_attr atributos_cola;
	est_bat_t dato_cola;
	
	/* Recibe datos de la cola de mensajes desde el proceso de recepcion CAN de la BMS */
	mq_getattr(cola, &atributos_cola);		// Actualiza los atributos de la cola creada en el otro extremo...
	num_msgs_cola = atributos_cola.mq_curmsgs;	// ... para conocer el numero de mensajes que tiene que recibir.
	/* Si hay datos que recibir */
	if (num_msgs_cola > 0)
	{
		for(i=0; i<num_msgs_cola; i++)
		{
			/* Obtiene el tiempo actual */
			clock_gettime(CLOCK_REALTIME, &timeout_cola);
			/* Establece el tiempo absoluto para el timeout de la cola */
			timeout_cola.tv_sec += TIEMPO_EXP_COLA_S;
			/* Recibimos los mensajes de la cola uno a uno */
			estado_cola = mq_timedreceive(cola, (char *)&dato_cola, sizeof(dato_cola), NULL, &timeout_cola);
			if (estado_cola == -1)
			{
				/* Error de recepcion de cola */
				printf("Error de cola can2 - proc ppal\n");
				pthread_mutex_lock(&mut_errores);
				errores.er_critico_1 |= ERC_CAN2_COM_COLA;
				pthread_mutex_unlock(&mut_errores);
			}
			else
			{
				//printf("Recibe datos cola bms - proc ppal\n");
				/* Cada vez que recibe un dato correcto, los vuelca en la estructura de bateria */
				pthread_mutex_lock(&mut_bateria);
				memcpy(&bateria, &dato_cola, sizeof(est_bat_t));
				pthread_mutex_unlock(&mut_bateria);
			}
		}
	}
}

void envia_datos_cola_imu(mqd_t cola)
{
	/* Variables */
	uint16_t erc_1_local = 0;
	uint16_t erg_1_local = 0;
	uint16_t erg_2_local = 0;
	uint16_t erl_1_local = 0;
	uint16_t erl_2_local = 0;
	int estado_cola = 0;
//-------------------->wow static!
	static int cont_error_cola = 0;
	/* Para la cola de mensajes */
	dato_cola_imu_t dato_cola;
	
	/* Prepara los datos a enviar a la cola */
	pthread_mutex_lock(&mDatosImu);
	memcpy(&(dato_cola.datosImu), &tDatosImu, sizeof(datosImu_t));
	pthread_mutex_unlock(&mDatosImu);
	pthread_mutex_lock(&mut_errores);
	erc_1_local = errores.er_critico_1;
	erg_1_local = errores.er_grave_1;
	erg_2_local = errores.er_grave_2;
	erl_1_local = errores.er_leve_1;
	erl_2_local = errores.er_leve_2;	
	pthread_mutex_unlock(&mut_errores);
	dato_cola.errores_criticos = erc_1_local;
	dato_cola.errores_graves = (erg_1_local & MASK_BYTE_BAJO) | ((erg_2_local << 8) & MASK_BYTE_ALTO);
	dato_cola.errores_leves = (erl_1_local & MASK_BYTE_BAJO) | ((erl_2_local << 8) & MASK_BYTE_ALTO);
	
	/* Envia datos a la cola de mensajes del proceso de datalogging de la imu */
	estado_cola = mq_send(cola, (char *)&dato_cola, sizeof(dato_cola), NULL);
	if(estado_cola == -1)
	{
		/* Error de envio de cola */
		printf("Error envio cola main\n");
		printf("Error envio: %d\n", errno);
		cont_error_cola++;
		if (cont_error_cola > NUM_ERR_MAX_COLA)
		{
			pthread_mutex_lock(&mut_errores);
			errores.er_grave_2 |= ERG_IMU_COM_COLA;
			pthread_mutex_unlock(&mut_errores);
		}
	}
	else
	{
		//printf("Envio de datos por cola imu correcto - proc ppal\n");
		cont_error_cola = 0;
	}
}

int imprime_pantalla (uint16_t imprimir)
{
	int i,j;
	est_bat_t bat;
	est_superv_t sup;
	est_veh_t veh;
	est_error_t err;
	est_pot_t pot;
	datosImu_t imu;
	est_motor_t mo1, mo2, mo3, mo4;
	
//El tamanio de pantalla en modo texto es de 80 caracteres
//El tabulador son 5 caracteres.
	
	printf("\e[1;1H\e[2J"); //Limpia la pantalla
	
	if ((imprimir&ON_PROC_CANBMS) != OFF_TODOS)
	{	
		pthread_mutex_lock(&mut_bateria);
		bat = bateria;
		pthread_mutex_unlock(&mut_bateria);
		
		printf("\n-------------------------------------BMS--------------------------------------\n");
		printf("Tiempo\tErrCan\tErrBMS\tNivAlrm\tAlarma\tNumCeld\tSOC\tVPack\tIPack\n");
		printf("%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",bat.timestamp,bat.error_com_tcan,bat.error_com_bms,
				bat.nivel_alarma,bat.alarma,bat.num_cel_scan,bat.soc,bat.v_pack,bat.i_pack);
		printf("TempMed\tTempMax\tCelTMax\tVMed\tVMax\tCelVMax\tVMin\tCelVMin\n");
		printf("%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",bat.temp_media,bat.temp_max,bat.cel_temp_max,
				bat.v_medio,bat.v_max,bat.cel_v_max,bat.v_min,bat.cel_v_min);
		printf("\n\n");
		
		for(j=0;j<((int)NUM_CEL_BAT/9);j++)
		{
			for(i=j*9;i<(j*9+9);i++)
				printf("mV%d\t",i+1);
			printf("\n");
			for(i=j*9;i<(j*9+9);i++)
				printf("%d\t",bat.mv_cel[i]);
			printf("\n");
		}
		
		j = i;
		
		for(i=j;i<(j+NUM_CEL_BAT%9);i++)
			printf("mV%d\t",i+1);
		printf("\n");
		for(i=j;i<(j+NUM_CEL_BAT%9);i++)
			printf("%d\t",bat.mv_cel[i]);
		printf("\n\n");

		
		for(j=0;j<((int)NUM_CEL_BAT/9);j++)
		{
			for(i=j*9;i<(j*9+9);i++)
				printf("T%d\t",i+1);
			printf("\n");
			for(i=j*9;i<(j*9+9);i++)
				printf("%d\t",bat.temp_cel[i]);
			printf("\n");
		}
		
		j = i;
		
		for(i=j;i<(j+NUM_CEL_BAT%9);i++)
			printf("T%d\t",i+1);
		printf("\n");
		for(i=j;i<(j+NUM_CEL_BAT%9);i++)
			printf("%d\t",bat.temp_cel[i]);
		printf("\n------------------------------------------------------------------------------\n");		
	}
	
	if ((imprimir&ON_HILO_ERRORES) != OFF_TODOS)
	{
		pthread_mutex_lock(&mut_errores);
		err = errores;
		pthread_mutex_unlock(&mut_errores);
		
		printf("\n--------------------------------ERRORES---------------------------------------\n");
		printf("ErrLeve\tErrGrav\tErrCrit\tErL1\tErL2\tErG1\tErG2\tErC1\n");
		printf("%x\t%x\t%x\t%x\t%x\t%x\t%x\t%x\n",err.error_leve,err.error_grave,err.error_critico,err.er_leve_1,err.er_leve_2,
				err.er_grave_1,err.er_grave_2,err.er_critico_1);
		printf("ErEm\tErWBMS\tErWM1\tErWM2\tErWM3\tErWM4\n");
		printf("%x\t%x\t%x\t%x\t%x\t%x",err.er_emergencia,err.er_watchdog_bms,err.er_watchdog_m1,err.er_watchdog_m2,
				err.er_watchdog_m3,err.er_watchdog_m4);
		printf("\n------------------------------------------------------------------------------\n");	
	}
	
	if ((imprimir&ON_HILO_IMU) != OFF_TODOS)
	{
		pthread_mutex_lock(&mDatosImu);
		imu = tDatosImu;
		pthread_mutex_unlock(&mDatosImu);		
		
		printf("\n-------------------------------------IMU--------------------------------------\n");
		printf("AccelX\tAccelY\tAccelZ\tGyroX\tGyroY\tGyroZ\n");
		for(i=0;i<3;i++)	
			printf("%#.3f\t",imu.pfAccelScal[i]);
		for(i=0;i<3;i++)	
			printf("%#.3f\t",imu.pfGyroScal[i]);
		printf("\nTime\tPosN\tPosE\tPosHe\tPosHsl\tEulerR\tEulerP\tEulerY\n");
		printf("%d\t",(imu.iTimeStamp)/62500);
		for(i=0;i<4;i++)	
			printf("%#.3f\t",imu.pdLLHPos[i]);
		for(i=0;i<3;i++)	
			printf("%#.3f\t",imu.pfEuler[i]);	
		printf("\nVelN\tVelE\tVelD\tVelMod\tVelModH\tVelPol\n");
		for(i=0;i<6;i++)	
			printf("%#.3f\t",imu.pfNEDVeloc[i]);
		printf("\n------------------------------------------------------------------------------\n");
	}

	if ((imprimir&ON_HILO_ADQ) != OFF_TODOS)
	{
		pthread_mutex_lock(&mut_vehiculo);
		veh = vehiculo;
		pthread_mutex_unlock(&mut_vehiculo);
		pthread_mutex_lock(&mut_potencias);
		pot = potencias;
		pthread_mutex_unlock(&mut_potencias);
		
		printf("\n-------------------------------------ADQ--------------------------------------\n");
		printf("Acelera\tFreno\tVolante\tSuspTI\tSuspTD\tSuspDI\tSuspDD\n");
		printf("%#.3f\t%#.3f\t%#.3f\t%#.3f\t%#.3f\t%#.3f\t%#.3f\n",veh.acelerador,veh.freno,veh.volante,
				veh.susp_ti,veh.susp_td,veh.susp_di,veh.susp_dd);
		printf("Rev\tAcl\tF1 F2 F3 F4\tIDel\tITras\n");
		printf("%d\t%d\t%d  %d  %d  %d\t%#.3f\t%#.3f",veh.marcha_atras,veh.act_acel,pot.act_freno[0],pot.act_freno[1],
				pot.act_freno[2],pot.act_freno[3],veh.i_eje_d,veh.i_eje_t);
		printf("\n------------------------------------------------------------------------------\n");
	}
	
	if ((imprimir&ON_HILO_ADQAO) != OFF_TODOS)
	{
		pthread_mutex_lock(&mut_potencias);
		pot = potencias;
		pthread_mutex_unlock(&mut_potencias);
		
		printf("\n-------------------------------------ADQAO------------------------------------\n");
		printf("AcelM1\tAcelM2\tAcelM3\tAcelM4\tFrenoM1\tFrenoM2\tFrenoM3\tFrenoM4\n");
		for(i=0;i<NUM_MOTORES;i++)
			printf("%#.3f\t",pot.acelerador[i]);
		for(i=0;i<NUM_MOTORES;i++)
			printf("%#.3f\t",pot.freno[i]);		
		printf("\n------------------------------------------------------------------------------\n");
	}
	
	if ((imprimir&ON_HILO_CANRX) != OFF_TODOS)
	{

	}
	
	if ((imprimir&ON_HILO_CANSUPERV) != OFF_TODOS)
	{
		pthread_mutex_lock(&mut_supervisor);
		sup = supervisor;
		pthread_mutex_unlock(&mut_supervisor);
		
		printf("\n---------------------------------SUPERVISOR-----------------------------------\n");
		printf("Hrtbt\tSendOff\tMustOff\tOn\tM1  M2  M3  M4\n");
		printf("%d\t%d\t%d\t%d\t%d  %d  %d  %d",sup.heartbeat,sup.enviado_off,sup.peticion_off,
				sup.on,sup.motor_on[0],sup.motor_on[1],sup.motor_on[2],sup.motor_on[3]);
		printf("\n------------------------------------------------------------------------------\n");
	}

	if ((imprimir&ON_HILO_CANMOTOR1) != OFF_TODOS)
	{
		pthread_mutex_lock(&mut_motor1);
		mo1 = motor1;
		pthread_mutex_unlock(&mut_motor1);
		
		printf("\n-----------------------------------MOTOR 1------------------------------------\n");
		printf("Modelo\tVersion\tDZInfAc\tDZSupAc\tDZInfFr\tDZSupFr\tI%%\n");
		printf("%s\t%d\t%d\t%d\t%d\t%d\t%d\n",mo1.modelo+3,mo1.version,mo1.zm_inf_acel,mo1.zm_sup_acel,
				mo1.zm_inf_fren,mo1.zm_sup_fren,mo1.i_porcent);
		printf("Ia\tIb\tIc\tVa\tVb\tVc\n");
		printf("%d\t%d\t%d\t%#.3f\t%#.3f\t%#.3f\n",mo1.i_a,mo1.i_b,mo1.i_c,mo1.v_a,mo1.v_b,mo1.v_c);		
		printf("Freno\tAcelera\tVPot\tVAux\tVBat\tPWM\tRPM\n");
		printf("%#.3f\t%#.3f\t%#.3f\t%#.3f\t%#.3f\t%d\t%d\n",mo1.freno,mo1.acel,mo1.v_pot,mo1.v_aux,
				mo1.v_bat,mo1.pwm,mo1.rpm);
		printf("TempCtl\tTempSup\tTempInf\tTempMot\tA  F  R  En\n");
		printf("%d\t%d\t%d\t%d\t%d  %d  %d  %d",mo1.temp_int_ctrl,mo1.temp_sup_ctrl,mo1.temp_inf_ctrl,mo1.temp_motor,
				mo1.acel_switch,mo1.freno_switch,mo1.reverse_switch,mo1.en_motor_rot);
		printf("\n------------------------------------------------------------------------------\n");
	}
	
	if ((imprimir&ON_HILO_CANMOTOR2) != OFF_TODOS)
	{
		pthread_mutex_lock(&mut_motor2);
		mo2 = motor2;
		pthread_mutex_unlock(&mut_motor2);
		
		printf("\n-----------------------------------MOTOR 2------------------------------------\n");
		printf("Modelo\tVersion\tDZInfAc\tDZSupAc\tDZInfFr\tDZSupFr\tI%%\n");
		printf("%s\t%d\t%d\t%d\t%d\t%d\t%d\n",mo2.modelo+3,mo2.version,mo2.zm_inf_acel,mo2.zm_sup_acel,
				mo2.zm_inf_fren,mo2.zm_sup_fren,mo2.i_porcent);
		printf("Ia\tIb\tIc\tVa\tVb\tVc\n");
		printf("%d\t%d\t%d\t%#.3f\t%#.3f\t%#.3f\n",mo2.i_a,mo2.i_b,mo2.i_c,mo2.v_a,mo2.v_b,mo2.v_c);		
		printf("Freno\tAcelera\tVPot\tVAux\tVBat\tPWM\tRPM\n");
		printf("%#.3f\t%#.3f\t%#.3f\t%#.3f\t%#.3f\t%d\t%d\n",mo2.freno,mo2.acel,mo2.v_pot,mo2.v_aux,
				mo2.v_bat,mo2.pwm,mo2.rpm);
		printf("TempCtl\tTempSup\tTempInf\tTempMot\tA  F  R  En\n");
		printf("%d\t%d\t%d\t%d\t%d  %d  %d  %d",mo2.temp_int_ctrl,mo2.temp_sup_ctrl,mo2.temp_inf_ctrl,mo2.temp_motor,
				mo2.acel_switch,mo2.freno_switch,mo2.reverse_switch,mo2.en_motor_rot);
		printf("\n------------------------------------------------------------------------------\n");
	}
	
	if ((imprimir&ON_HILO_CANMOTOR3) != OFF_TODOS)
	{
		pthread_mutex_lock(&mut_motor3);
		mo3 = motor3;
		pthread_mutex_unlock(&mut_motor3);
		
		printf("\n-----------------------------------MOTOR 3------------------------------------\n");
		printf("Modelo\tVersion\tDZInfAc\tDZSupAc\tDZInfFr\tDZSupFr\tI%%\n");
		printf("%s\t%d\t%d\t%d\t%d\t%d\t%d\n",mo3.modelo+3,mo3.version,mo3.zm_inf_acel,mo3.zm_sup_acel,
				mo3.zm_inf_fren,mo3.zm_sup_fren,mo3.i_porcent);
		printf("Ia\tIb\tIc\tVa\tVb\tVc\n");
		printf("%d\t%d\t%d\t%#.3f\t%#.3f\t%#.3f\n",mo3.i_a,mo3.i_b,mo3.i_c,mo3.v_a,mo3.v_b,mo3.v_c);		
		printf("Freno\tAcelera\tVPot\tVAux\tVBat\tPWM\tRPM\n");
		printf("%#.3f\t%#.3f\t%#.3f\t%#.3f\t%#.3f\t%d\t%d\n",mo3.freno,mo3.acel,mo3.v_pot,mo3.v_aux,
				mo3.v_bat,mo3.pwm,mo3.rpm);
		printf("TempCtl\tTempSup\tTempInf\tTempMot\tA  F  R  En\n");
		printf("%d\t%d\t%d\t%d\t%d  %d  %d  %d",mo3.temp_int_ctrl,mo3.temp_sup_ctrl,mo3.temp_inf_ctrl,mo3.temp_motor,
				mo3.acel_switch,mo3.freno_switch,mo3.reverse_switch,mo3.en_motor_rot);
		printf("\n------------------------------------------------------------------------------\n");
	}
	
	if ((imprimir&ON_HILO_CANMOTOR4) != OFF_TODOS)
	{
		pthread_mutex_lock(&mut_motor4);
		mo4 = motor4;
		pthread_mutex_unlock(&mut_motor4);
		
		printf("\n-----------------------------------MOTOR 4------------------------------------\n");
		printf("Modelo\tVersion\tDZInfAc\tDZSupAc\tDZInfFr\tDZSupFr\tI%%\n");
		printf("%s\t%d\t%d\t%d\t%d\t%d\t%d\n",mo4.modelo+3,mo4.version,mo4.zm_inf_acel,mo4.zm_sup_acel,
				mo4.zm_inf_fren,mo4.zm_sup_fren,mo4.i_porcent);
		printf("Ia\tIb\tIc\tVa\tVb\tVc\n");
		printf("%d\t%d\t%d\t%#.3f\t%#.3f\t%#.3f\n",mo4.i_a,mo4.i_b,mo4.i_c,mo4.v_a,mo4.v_b,mo4.v_c);		
		printf("Freno\tAcelera\tVPot\tVAux\tVBat\tPWM\tRPM\n");
		printf("%#.3f\t%#.3f\t%#.3f\t%#.3f\t%#.3f\t%d\t%d\n",mo4.freno,mo4.acel,mo4.v_pot,mo4.v_aux,
				mo4.v_bat,mo4.pwm,mo4.rpm);
		printf("TempCtl\tTempSup\tTempInf\tTempMot\tA  F  R  En\n");
		printf("%d\t%d\t%d\t%d\t%d  %d  %d  %d",mo4.temp_int_ctrl,mo4.temp_sup_ctrl,mo4.temp_inf_ctrl,mo4.temp_motor,
				mo4.acel_switch,mo4.freno_switch,mo4.reverse_switch,mo4.en_motor_rot);
		printf("\n------------------------------------------------------------------------------\n");
	}
	
	if ((imprimir&ON_HILO_GESTPOT) != OFF_TODOS)
	{

	}
	
	if ((imprimir&ON_HILO_CTRLTRAC) != OFF_TODOS)
	{

	}
	
	if ((imprimir&ON_HILO_CTRLEST) != OFF_TODOS)
	{

	}
	
	return(1);
}
