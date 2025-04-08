/***************************************
// Proyecto: FOX
// Nombre fichero: can-rx_handler.c
// Descripción: Hilo de adquisición de datos CAN para motores.     
//              Recogida de datos CAN de los motores usando SocketCAN y
//              funciones POSIX.   
// Autor: Echedey Aguilar Hernández
// email: eaguilar@us.es
// Fecha: 2025-03-26
***************************************/

/**************************************************
***        CABECERAS                           ***
**************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/mman.h>
#include <pthread.h>
#include <string.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>

/* Incluye las cabeceras locales */
#include "./include/constantes_fox.h"
#include "./include/estructuras_fox.h"
#include "./include/declaraciones_fox.h"

/**************************************************
***        VARIABLES GLOBALES                  ***
**************************************************/
/* Variables globales (se asume que algunas se definen en otros módulos) */
extern uint16_t hilos_listos;
extern est_error_t errores;

/* Buffers CAN – se usa la estructura de SocketCAN: struct can_frame */
int cont_buff_rx = 0;  // Contador de mensajes en el buffer del supervisor
struct can_frame *buff_rx_superv;  // Buffer de mensajes del supervisor
struct can_frame buff_rx_mot1;     // Buffer para motor 1
struct can_frame buff_rx_mot2;     // Buffer para motor 2
struct can_frame buff_rx_mot3;     // Buffer para motor 3
struct can_frame buff_rx_mot4;     // Buffer para motor 4

/* Identificador de la conexión CAN (en QNX se usaba canhdl_t hdl; aquí usaremos el socket CAN) */
int hdl;  // Para compatibilidad, se conserva el nombre; en esta adaptación hdl equivale al socket

/* Mutex y condiciones (algunos se inicializan externamente) */
extern pthread_mutex_t mut_hilos_listos;
extern pthread_mutex_t mut_inicio;
extern pthread_cond_t inicio;
extern pthread_mutex_t mut_hdl;  // Utilizado para proteger el acceso al socket

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

/**************************************************
***        CONFIGURACIÓN DE SOCKET CAN         ***
**************************************************/
static int init_can_socket(void)
{
    int s;
    struct ifreq ifr;
    struct sockaddr_can addr;

    s = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (s < 0) {
        perror("Error al abrir socket CAN");
        return -1;
    }
    strcpy(ifr.ifr_name, CAN_INTERFACE);  // Por ejemplo, "can0"
    if (ioctl(s, SIOCGIFINDEX, &ifr) < 0) {
        perror("Error al obtener índice de interfaz CAN");
        close(s);
        return -1;
    }
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Error al hacer bind del socket CAN");
        close(s);
        return -1;
    }
    return s;
}

/******************************************************************************************
***         FUNCIÓN PRINCIPAL: can_rx (hilo)                                      ***
***  Recibe mensajes CAN desde el bus y los distribuye según su identificador.   ***
******************************************************************************************/
void *can_rx(void *pi)
{
    BOOL fin_local = 0;
    struct can_frame rxmsg;
    int dev_can;
    /* Variables para temporización en espera en caso de error (opcional) */
    struct timespec espera = {0, TEMPO_CAN};  
    short contador_timeout = 0;

    /* Inicialización del socket CAN */
    hdl = init_can_socket();
    if (hdl < 0) {
        pthread_exit(NULL);
    }

    /* Asignamos el socket al identificador global */
    // En esta adaptación, hdl es el socket CAN (usado por otras funciones para enviar mensajes)
    
    /* Reservar memoria para el buffer de recepción del supervisor */
    buff_rx_superv = malloc(TAM_BUFF_RX * sizeof(struct can_frame));
    if (!buff_rx_superv) {
        perror("Error al asignar memoria para buff_rx_superv");
        close(hdl);
        pthread_exit(NULL);
    }

    /* Indicar que el hilo de recepción CAN está listo */
    pthread_mutex_lock(&mut_hilos_listos);
    hilos_listos |= HILO_CANRX_LISTO;
    pthread_mutex_unlock(&mut_hilos_listos);
    printf("Hilo Rx CAN listo\n");

    /* Esperar la señal de inicio */
    pthread_mutex_lock(&mut_inicio);
    pthread_cond_wait(&inicio, &mut_inicio);
    pthread_mutex_unlock(&mut_inicio);

    while (!fin_local)
    {
        /* Leer el siguiente mensaje CAN (bloqueante) */
        dev_can = read(hdl, &rxmsg, sizeof(struct can_frame));
        if (dev_can < 0)
        {
            perror("Error al leer mensaje CAN");
            contador_timeout++;
            if (contador_timeout > TIEMPO_EXP_CAN_X) {
                printf("Error crítico de recepción CAN\n");
                pthread_mutex_lock(&mut_errores);
                errores.er_critico_1 |= ERC_ECU_COM_TCAN1;
                pthread_mutex_unlock(&mut_errores);
            }
        }
        else
        {
            contador_timeout = 0;
            /* Filtrado del mensaje según el identificador */
            switch (rxmsg.can_id)
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
                    if ((rxmsg.can_id == ID_SUPERV_HEARTBEAT) ||
                        (rxmsg.can_id == ID_SUPERV_OFF))
                    {
                        pthread_mutex_lock(&mut_rx_superv);
                        if (cont_buff_rx >= TAM_BUFF_RX) {
                            cont_buff_rx = TAM_BUFF_RX;
                            printf("Overflow buffer Rx supervisor\n");
                        }
                        buff_rx_superv[cont_buff_rx] = rxmsg;
                        cont_buff_rx++;
                        pthread_mutex_unlock(&mut_rx_superv);
                    }
                    break;
            }
        }

        /* Verificar si se debe finalizar el hilo */
        pthread_mutex_lock(&mut_hilos_listos);
        if ((hilos_listos & HILO_CANRX_LISTO) == 0)
            fin_local = 1;
        pthread_mutex_unlock(&mut_hilos_listos);
    }

    free(buff_rx_superv);
    close(hdl);
    pthread_exit(NULL);
    return NULL;
}
