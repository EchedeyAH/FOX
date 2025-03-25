/***************************************/
// Proyecto: FOX
// Nombre fichero:can.c (Adaptado para Linux Ubuntu 22.04) 
// Descripción: Proceso de adquisición de datos CAN desde la BMS.
//              Se lee un frame CAN, se procesa y se envía a una
//              cola de mensajes para el proceso principal de la ECU
// Autor: Echedey Aguilar Hernández
// email: eaguilar@us.es
// Fecha: 2025-03-20
// ***************************************/

// CABECERAS estándar de Linux y POSIX
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
#include <mqueue.h>
#include <pthread.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/ioctl.h>

// Incluimos nuestras cabeceras propias (adaptadas al nuevo proyecto)
#include "ecu_fox/constantes_fox.h"
#include "ecu_fox/estructuras_fox.h"
#include "ecu_fox/declaraciones_fox.h"
// Para la parte CAN, en vez de usar rutas relativas a "./include/", 
// se ha integrado la parte del fabricante en third_party/can/

/**************************************************
***           VARIABLES GLOBALES                ***
**************************************************/
// Estructura global para los datos a enviar por cola (dato_cola_t definido en can2_fox.h)
dato_cola_t dato_cola;

// Identificador del socket CAN (global, también declarado en canglob.h si es necesario)
int can_socket = -1;

// Cola de mensajes POSIX
mqd_t cola;

/**************************************************
***       FUNCIONES DE INICIALIZACIÓN           ***
**************************************************/
// Función para inicializar la interfaz CAN usando SocketCAN
static int init_can_interface(void) {
    struct ifreq ifr;
    struct sockaddr_can addr;

    // Crear el socket CAN
    can_socket = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (can_socket < 0) {
        perror("Error al crear el socket CAN");
        return ERROR_FAIL;
    }

    // Configurar la interfaz CAN (se utiliza la constante CAN_INTERFACE definida en constantes_fox.h)
    strncpy(ifr.ifr_name, CAN_INTERFACE, IFNAMSIZ-1);
    if (ioctl(can_socket, SIOCGIFINDEX, &ifr) < 0) {
        perror("Error al obtener el índice de la interfaz CAN");
        close(can_socket);
        return ERROR_FAIL;
    }

    // Enlazar el socket a la interfaz CAN
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    if (bind(can_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Error al enlazar el socket CAN");
        close(can_socket);
        return ERROR_FAIL;
    }
    
    printf("Socket CAN inicializado correctamente en la interfaz %s\n", CAN_INTERFACE);
    return ERROR_OK;
}

// Función para inicializar la cola de mensajes POSIX
static int init_message_queue(void) {
    struct mq_attr atrib_cola;
    atrib_cola.mq_maxmsg = TAMANHO_COLA_CAN2;  // Definido en can2_fox.h
    atrib_cola.mq_msgsize = sizeof(dato_cola_t);
    atrib_cola.mq_flags = 0;
    
    cola = mq_open(NOMBRE_COLA, O_CREAT | O_RDWR, 0644, &atrib_cola);
    if (cola == (mqd_t)-1) {
        perror("Error al crear la cola de mensajes");
        return ERROR_FAIL;
    }
    
    printf("Cola de mensajes '%s' creada correctamente\n", NOMBRE_COLA);
    return ERROR_OK;
}

/**************************************************
***          FUNCIÓN PRINCIPAL (main)           ***
**************************************************/
int main(void)
{
    int fin = 0;              // Bandera de finalización del bucle CAN
    int ret;
    struct can_frame msg_can_in;   // Estructura CAN de Linux para mensajes de entrada
    int estado_cola = 0;
    short contador_timeout = 0;
    short cont_error_cola = 0;

    // Variables para temporización (ajustadas a nanosleep)
    struct timespec espera = {0, TEMPO_CAN}; // TEMPO_CAN definido en can2_fox.h (en nanosegundos)

    // Inicializar la interfaz CAN
    ret = init_can_interface();
    if (ret != ERROR_OK) {
        fprintf(stderr, "Fallo en la inicialización de la interfaz CAN\n");
        exit(EXIT_FAILURE);
    }
    
    // Inicializar la cola de mensajes
    ret = init_message_queue();
    if (ret != ERROR_OK) {
        fprintf(stderr, "Fallo en la inicialización de la cola de mensajes\n");
        close(can_socket);
        exit(EXIT_FAILURE);
    }

    // Aquí, en lugar de crear canales y usar funciones de QNX, se usa el socket CAN.
    // Además, en vez de MsgReceivePulse se usará un bucle de lectura sobre el socket.
    
    while(!fin) {
        int nbytes = read(can_socket, &msg_can_in, sizeof(struct can_frame));
        if (nbytes < 0) {
            perror("Error al leer del socket CAN");
            contador_timeout++;
            printf("Error en lectura, contador: %d\n", contador_timeout);
            if (contador_timeout > 80)
                contador_timeout = 80;
        } else if (nbytes == 0) {
            // No se han recibido datos: podemos hacer un sleep breve
            usleep(100000); // 100 ms
            continue;
        } else {
            // Reiniciamos el contador de errores en caso de lectura exitosa
            contador_timeout = 0;
            
            // Procesamos el mensaje solo si la ID coincide con la BMS
            if (msg_can_in.can_id == ID_CAN_BMS) {
                // En este ejemplo, se llama a la función de interpretación para convertir el frame
                interpreta_can_entrada((struct can_object *)&msg_can_in);
                
                // Enviar los datos procesados a la cola de mensajes
                estado_cola = mq_send(cola, (char*)&dato_cola, sizeof(dato_cola_t), 0);
                if (estado_cola == -1) {
                    cont_error_cola++;
                    printf("Cola llena, contador: %d\n", (int)cont_error_cola);
                } else {
                    cont_error_cola = 0;
                }
                
                if (cont_error_cola > TIMEOUT_COLA) {
                    printf("Error en la comunicación de la cola, terminando proceso\n");
                    fin = 1;
                }
            }
        }
        
        // Si se superan ciertos errores de lectura, se termina el bucle
        if (contador_timeout > TIEMPO_EXP_CAN_X) {
            printf("Demasiados errores de lectura, terminando proceso\n");
            fin = 1;
        }
    }
    
    // Cierre ordenado: cerrar el socket CAN y la cola de mensajes
    // Suponiendo que DisConnectDriver se reemplace por close(can_socket)
    close(can_socket);
    mq_close(cola);
    mq_unlink(NOMBRE_COLA);
    return 0;
}

/******************************************************************************************
***         FUNCIÓN: interpreta_can_entrada            ***
*** Identifica y procesa el mensaje CAN recibido en función de su tipo y  ***
*** el identificador. Se asume que 'dato_cola' global se actualiza con los datos   ***
*** correspondientes.                                                    ***
******************************************************************************************/
void interpreta_can_entrada(struct can_object *p_can)
{
    // Variables auxiliares para convertir datos del mensaje (igual que en el código original)
    char c_aux_index[NUM_MAX_DIG_INDEX+3];
    char c_aux_value[NUM_MAX_DIG_VALUE+3];
    int i = 0;
    int index = 0;
    char c_param;
    int param = 0;
    int value = 0;
    
    // Inicialización de buffers
    c_aux_index[0] = '0';
    c_aux_index[1] = 'x';
    c_aux_index[NUM_MAX_DIG_INDEX+2] = '\0';
    c_aux_value[0] = '0';
    c_aux_value[1] = 'x';
    c_aux_value[NUM_MAX_DIG_VALUE+2] = '\0';
    
    // Leer el índice (convertir los primeros NUM_MAX_DIG_INDEX bytes a hexadecimal)
    for (i=0; i<NUM_MAX_DIG_INDEX; i++)
        c_aux_index[i+2] = (char)p_can->data[i];
    index = (int)strtol(c_aux_index, NULL, 0);
    
    // Leer el parámetro (tercer byte)
    c_param = p_can->data[2];
    param = c_param;
    
    // Leer el valor (siguientes NUM_MAX_DIG_VALUE bytes)
    for (i=3; i<(NUM_MAX_DIG_VALUE+3); i++)
        c_aux_value[i-1] = (char)p_can->data[i];
    value = (int)strtol(c_aux_value, NULL, 0);
    
    // Procesar el mensaje CAN según el parámetro
    switch(param) {
        case VOLTAJE_T:
            if((index > 0) && (index <= NUM_CEL_BAT))
                dato_cola.mv_cel[index-1] = (uint16_t)value;
            break;
        case TEMPERATURA_T:
            if((index > 0) && (index <= NUM_CEL_BAT))
                dato_cola.temp_cel[index-1] = (uint8_t)value;
            break;
        case ESTADO_T:
            switch(index) {
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
                    break;
            }
            break;
        case ALARMA_T:
            switch(index) {
                case 1:
                    dato_cola.nivel_alarma = (uint8_t) NO_ALARMA;
                    dato_cola.alarma = (uint8_t) PACK_OK;
                    break;
                case 2:
                    if(value != 0) {
                        dato_cola.nivel_alarma = (uint8_t) ALARMA;
                        dato_cola.alarma = (uint8_t) CELL_TEMP_HIGH;
                    }
                    break;
                // Otros casos para ALARMA_T según el original...
                default:
                    break;
            }
            break;
        default:
            printf("No se reconoce el mensaje de la BMS.\n");
            break;
    }
}
