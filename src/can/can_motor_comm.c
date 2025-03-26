/***************************************
    Proyecto: FOX
    Nombre fichero: can_motor_comm.c
    Descripción: Hilo de adquisición de datos CAN para motores.     ***
              Recogida de datos CAN de los motores desde el buffer de 
              recepción del hilo canRX (adaptado a SocketCAN y POSIX).  
    Autor: Echedey Aguilar Hernández
    email: eaguilar@us.es
    Fecha: 2025-03-26
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

/* Can includes – se asume que las definiciones locales se mantienen compatibles */
#include "./include/candef.h"
#include "./include/canstr.h"
#include "./include/canglob.h"

/* Include local */
#include "./include/constantes_fox.h"
#include "./include/estructuras_fox.h"
#include "./include/declaraciones_fox.h"

/**************************************************
***         VARIABLES GLOBALES                 ***
**************************************************/
/* Variables globales externas */
extern uint16_t hilos_listos;
/* Estructuras globales */
extern est_error_t errores;
extern est_motor_t motor1;   // Motor 1 – Trasero izquierdo
extern est_motor_t motor2;   // Motor 2 – Delantero izquierdo
extern est_motor_t motor3;   // Motor 3 – Trasero derecho
extern est_motor_t motor4;   // Motor 4 – Delantero derecho

/* Buffers CAN – En la adaptación se usa struct can_frame (SocketCAN) */
extern struct can_frame buff_rx_mot1;
extern struct can_frame buff_rx_mot2;
extern struct can_frame buff_rx_mot3;
extern struct can_frame buff_rx_mot4;

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
/* Mutex para bloquear el handler de TX CAN */
extern pthread_mutex_t mut_hdl;
/* En la versión original se usaba canhdl_t hdl; para Linux usaremos un socket CAN global */
extern int hdl;   // En esta adaptación, hdl se reemplaza por el socket
               // (Aunque se conserva el nombre para mantener consistencia)

/* Variable de control para indicar que todos los hilos se están ejecutando */
// (Se utiliza extern hilos_listos)

/**************************************************
***        DECLARACIÓN DE FUNCIONES           ***
**************************************************/
void interpreta_can_entrada(int tipo_msg, struct can_frame *p_can);
int envia_can(int tipo_msg, int id_can);

/* Función para enviar datos CAN mediante SocketCAN */
static int socket_can_write(struct can_frame *frame) {
    // Se utiliza el socket global hdl (o sock, según convención)
    return write(hdl, frame, sizeof(struct can_frame));
}

/******************************************************************************************************
***              FUNCIÓN PRINCIPAL: canMotor (hilo)                                                 ***
***   Gestiona el bus CAN para el controlador del motor n.                                         ***
***   Envía los mensajes de petición al controlador n y los recibe secuencialmente desde el buffer.  ***
*******************************************************************************************************/
void *canMotor(void *cm)
{
    struct timespec tiempo_exp_resp;
    struct can_frame rxmsg;
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
    struct can_frame *buff_rx_local;

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
            return NULL;
    }

    /* Indicar que el hilo de comunicación CAN del motor está listo */
    pthread_mutex_lock(&mut_hilos_listos);
    hilos_listos |= h_listo_local;
    pthread_mutex_unlock(&mut_hilos_listos);
    printf("Hilo CAN Motor %d listo\n", numMotor + 1);
    pthread_mutex_lock(&mut_inicio);
    pthread_cond_wait(&inicio, &mut_inicio);
    pthread_mutex_unlock(&mut_inicio);

    while (!fin_local)
    {
        for (tipo_mens = MSG_TIPO_01; tipo_mens < (MSG_TIPO_13 + 1); tipo_mens++)
        {
            if ((tipo_mens <= MSG_TIPO_02) && (cont_bucle % 200 != 0))
                continue;
            if ((tipo_mens > MSG_TIPO_02) && (tipo_mens <= MSG_TIPO_06) && (cont_bucle % 10 != 0))
                continue;
            do {
                /* Enviar mensaje de tipo "tipo_mens" */
                envia_can(tipo_mens, id_motor_local);
                /* Esperar la respuesta */
                clock_gettime(CLOCK_REALTIME, &tiempo_exp_resp);
                tiempo_exp_resp.tv_sec += TIEMPO_EXP_MOT_S;
                pthread_mutex_lock(pmut_hay_rx_mot);
                retval = pthread_cond_timedwait(phay_rx_mot, pmut_hay_rx_mot, &tiempo_exp_resp);
                pthread_mutex_unlock(pmut_hay_rx_mot);
                if (retval == ETIMEDOUT)
                    cont_intentos++;
                /* Si se supera el límite, se marca error */
                if (cont_intentos >= LIMITE_CONT_MOTOR_G)
                {
                    pthread_mutex_lock(&mut_errores);
                    errores.er_grave_1 |= er_grave_local;
                    pthread_mutex_unlock(&mut_errores);
                }
                if (cont_intentos > LIMITE_CONT_MOTOR_C)
                {
                    er_com_motor = TRUE;
                    pthread_mutex_lock(&mut_errores);
                    errores.er_critico_1 |= er_critico_local;
                    errores.er_grave_1 &= ~er_grave_local;
                    pthread_mutex_unlock(&mut_errores);
                }
            } while ((retval == ETIMEDOUT) && (er_com_motor == FALSE));
            /* Si se recibe el mensaje correctamente y no hay error crítico */
            if ((retval != ETIMEDOUT) && (er_com_motor == FALSE))
            {
                cont_intentos = 0;
                pthread_mutex_lock(&mut_errores);
                errores.er_critico_1 &= ~er_critico_local;
                errores.er_grave_1 &= ~er_grave_local;
                pthread_mutex_unlock(&mut_errores);
                pthread_mutex_lock(pmut_rx_mot);
                rxmsg = *buff_rx_local;
                pthread_mutex_unlock(pmut_rx_mot);
                interpreta_can_entrada(tipo_mens, &rxmsg);
            }
            nanosleep(&pausa_can_motores, NULL);
        }

        pthread_mutex_lock(&mut_hilos_listos);
        if ((hilos_listos & h_listo_local) == 0x0000)
            fin_local = 1;
        pthread_mutex_unlock(&mut_hilos_listos);

        cont_bucle++;
    }

    pthread_exit(NULL);
    return NULL;
}

/******************************************************************************************
***         FUNCIÓN: envia_can                                               ***
*** Envia los mensajes CAN al controlador del motor (según tipo de mensaje y ID CAN). ***
*** Parámetros:                                                            ***
***   - int tipo_msg: Número que identifica el tipo de mensaje.            ***
***   - int id_can: Identificador CAN para el mensaje a enviar.            ***
******************************************************************************************/
int envia_can(int tipo_msg, int id_can)
{
    struct can_frame msg_can_out;
    short dev_can = 0;
    
    memset(&msg_can_out, 0, sizeof(struct can_frame));

    /* Configuración del mensaje CAN:
       En SocketCAN se asigna el ID y la DLC; se omiten flags que en QNX se usaban para diferenciar
       modo estándar (StdFF) o extendido, por lo que se asume que id_can ya contiene la información adecuada. */
    msg_can_out.can_id = id_can;

    switch (tipo_msg)
    {
        case MSG_TIPO_01:
            msg_can_out.can_dlc = 3;
            msg_can_out.data[0] = (BYTE)(CCP_FLASH_READ);
            msg_can_out.data[1] = (BYTE)(INFO_MODULE_NAME);
            msg_can_out.data[2] = (BYTE)(0x08);
            break;
        case MSG_TIPO_02:
            msg_can_out.can_dlc = 3;
            msg_can_out.data[0] = (BYTE)(CCP_FLASH_READ);
            msg_can_out.data[1] = (BYTE)(INFO_SOFTWARE_VER);
            msg_can_out.data[2] = (BYTE)(0x02);
            break;
        case MSG_TIPO_03:
            msg_can_out.can_dlc = 3;
            msg_can_out.data[0] = (BYTE)(CCP_FLASH_READ);
            msg_can_out.data[1] = (BYTE)(CAL_TPS_DEAD_ZONE_LOW);
            msg_can_out.data[2] = (BYTE)(0x01);
            break;
        case MSG_TIPO_04:
            msg_can_out.can_dlc = 3;
            msg_can_out.data[0] = (BYTE)(CCP_FLASH_READ);
            msg_can_out.data[1] = (BYTE)(CAL_TPS_DEAD_ZONE_HIGH);
            msg_can_out.data[2] = (BYTE)(0x01);
            break;
        case MSG_TIPO_05:
            msg_can_out.can_dlc = 3;
            msg_can_out.data[0] = (BYTE)(CCP_FLASH_READ);
            msg_can_out.data[1] = (BYTE)(CAL_BRAKE_DEAD_ZONE_LOW);
            msg_can_out.data[2] = (BYTE)(0x01);
            break;
        case MSG_TIPO_06:
            msg_can_out.can_dlc = 3;
            msg_can_out.data[0] = (BYTE)(CCP_FLASH_READ);
            msg_can_out.data[1] = (BYTE)(CAL_BRAKE_DEAD_ZONE_HIGH);
            msg_can_out.data[2] = (BYTE)(0x01);
            break;
        case MSG_TIPO_07:
            msg_can_out.can_dlc = 1;
            msg_can_out.data[0] = (BYTE)(CCP_A2D_BATCH_READ1);
            break;
        case MSG_TIPO_08:
            msg_can_out.can_dlc = 1;
            msg_can_out.data[0] = (BYTE)(CCP_A2D_BATCH_READ2);
            break;
        case MSG_TIPO_09:
            msg_can_out.can_dlc = 1;
            msg_can_out.data[0] = (BYTE)(CCP_MONITOR1);
            break;
        case MSG_TIPO_10:
            msg_can_out.can_dlc = 1;
            msg_can_out.data[0] = (BYTE)(CCP_MONITOR2);
            break;
        case MSG_TIPO_11:
            msg_can_out.can_dlc = 2;
            msg_can_out.data[0] = (BYTE) COM_SW_ACC;
            msg_can_out.data[1] = (BYTE) COM_READING;
            break;
        case MSG_TIPO_12:
            msg_can_out.can_dlc = 2;
            msg_can_out.data[0] = (BYTE) COM_SW_BRK;
            msg_can_out.data[1] = (BYTE) COM_READING;
            break;
        case MSG_TIPO_13:
            msg_can_out.can_dlc = 2;
            msg_can_out.data[0] = (BYTE) COM_SW_REV;
            msg_can_out.data[1] = (BYTE) COM_READING;
            break;
        default:
            printf("Error. Tipo de mensaje CAN desconocido.\n");
            return -1;
    } // fin switch

    pthread_mutex_lock(&mut_hdl);
    dev_can = socket_can_write(&msg_can_out);
    pthread_mutex_unlock(&mut_hdl);

    return 0;
}

/******************************************************************************************
***         FUNCIÓN: interpreta_can_entrada                                    ***
*** Identifica el mensaje CAN recibido y lo procesa según el tipo y el identificador.***
*** Parámetros:                                                            ***
***  - int tipo_msg: Tipo de mensaje esperado (de 1 a 13).                  ***
***  - struct can_frame *p_can: Puntero al mensaje CAN recibido.            ***
******************************************************************************************/
int interpreta_can_entrada(int tipo_msg, struct can_frame *p_can)
{
    char buffer1[6], buffer2[6];
    int i;
    
    pthread_mutex_t *pmut_motor;
    est_motor_t *motor_local;
    int num_motor;
    
    switch(p_can->can_id)
    {
        case ID_CTRL_MOTOR_1_C_ECU:
            pmut_motor = &mut_motor1;
            motor_local = &motor1;
            num_motor = POS_M1 + 1;
            break;
        case ID_CTRL_MOTOR_2_C_ECU:
            pmut_motor = &mut_motor2;
            motor_local = &motor2;
            num_motor = POS_M2 + 1;
            break;
        case ID_CTRL_MOTOR_3_C_ECU:
            pmut_motor = &mut_motor3;
            motor_local = &motor3;
            num_motor = POS_M3 + 1;
            break;
        case ID_CTRL_MOTOR_4_C_ECU:
            pmut_motor = &mut_motor4;
            motor_local = &motor4;
            num_motor = POS_M4 + 1;
            break;
        default:
            printf("No se reconoce de qué motor proviene el mensaje.\n");
            return -1;
    }
    
    if (p_can->data[0] == CCP_INVALID_COMMAND)
    {
        printf("Respuesta de mensaje inválido.\n");
        return -1;
    }

    switch (tipo_msg)
    {
        case MSG_TIPO_01:
            pthread_mutex_lock(pmut_motor);
            for (i = 0; i < 8; i++)
                motor_local->modelo[i] = (char)p_can->data[i];
            motor_local->modelo[8] = '\0';
            pthread_mutex_unlock(pmut_motor);
            break;
        case MSG_TIPO_02:
            itoa((int)p_can->data[0], buffer1, 10);
            itoa((int)p_can->data[1], buffer2, 10);
            strcat(buffer1, buffer2);
            pthread_mutex_lock(pmut_motor);
            motor_local->version = (short)atoi(buffer1);
            pthread_mutex_unlock(pmut_motor);
            break;
        case MSG_TIPO_03:
            pthread_mutex_lock(pmut_motor);
            motor_local->zm_inf_acel = (BYTE)(p_can->data[0]) / 2;
            pthread_mutex_unlock(pmut_motor);
            break;
        case MSG_TIPO_04:
            pthread_mutex_lock(pmut_motor);
            motor_local->zm_sup_acel = (BYTE)(p_can->data[0]) / 2;
            pthread_mutex_unlock(pmut_motor);
            break;
        case MSG_TIPO_05:
            pthread_mutex_lock(pmut_motor);
            motor_local->zm_inf_fren = (BYTE)p_can->data[0];
            pthread_mutex_unlock(pmut_motor);
            break;
        case MSG_TIPO_06:
            pthread_mutex_lock(pmut_motor);
            motor_local->zm_sup_fren = (BYTE)p_can->data[0];
            pthread_mutex_unlock(pmut_motor);
            break;
        case MSG_TIPO_07:
            pthread_mutex_lock(pmut_motor);
            motor_local->freno = (float)p_can->data[0] / 51;
            motor_local->acel = (float)p_can->data[1] / 51;
            motor_local->v_pot = (float)p_can->data[2] / 2.71;
            motor_local->v_aux = (float)p_can->data[3] / 28 + 0.4643;
            motor_local->v_bat = (float)p_can->data[4] / 2.71;
            pthread_mutex_unlock(pmut_motor);
            break;
        case MSG_TIPO_08:
            pthread_mutex_lock(pmut_motor);
            motor_local->i_a = p_can->data[0];
            motor_local->i_b = p_can->data[1];
            motor_local->i_c = p_can->data[2];
            motor_local->v_a = (float)p_can->data[3] / 2.71;
            motor_local->v_b = (float)p_can->data[4] / 2.71;
            motor_local->v_c = (float)p_can->data[5] / 2.71;
            pthread_mutex_unlock(pmut_motor);
            break;
        case MSG_TIPO_09:
            pthread_mutex_lock(pmut_motor);
            motor_local->pwm = p_can->data[0];
            motor_local->en_motor_rot = (BOOL)p_can->data[1];
            motor_local->temp_motor = p_can->data[2];
            motor_local->temp_int_ctrl = p_can->data[3];
            motor_local->temp_sup_ctrl = p_can->data[4];
            motor_local->temp_inf_ctrl = p_can->data[5];
            pthread_mutex_unlock(pmut_motor);
            break;
        case MSG_TIPO_10:
            pthread_mutex_lock(pmut_motor);
            motor_local->rpm = (((uint16_t)p_can->data[1]) & MASK_BYTE_BAJO) |
                               ((((uint16_t)p_can->data[0]) << 8) & MASK_BYTE_ALTO);
            motor_local->i_porcent = p_can->data[2];
            pthread_mutex_unlock(pmut_motor);
            break;
        case MSG_TIPO_11:
            pthread_mutex_lock(pmut_motor);
            motor_local->acel_switch = (BOOL)p_can->data[0];
            pthread_mutex_unlock(pmut_motor);
            break;
        case MSG_TIPO_12:
            pthread_mutex_lock(pmut_motor);
            motor_local->freno_switch = (BOOL)p_can->data[0];
            pthread_mutex_unlock(pmut_motor);
            break;
        case MSG_TIPO_13:
            pthread_mutex_lock(pmut_motor);
            motor_local->reverse_switch = (BOOL)p_can->data[0];
            pthread_mutex_unlock(pmut_motor);
            break;
        default:
            printf("Tipo de mensaje no reconocido: no se puede recibir el mensaje.\n");
            return -1;
    }
    return 0;
}
