/***************************************
// Proyecto: FOX
// Nombre fichero: can_supervisor_comm.c
// Descripción: Hilo de adquisición de datos CAN para motores.    
//              Recogida de datos CAN de los motores usando SocketCAN y temporizadores POSIX.
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

/* Incluye las cabeceras locales (se supone que se han adaptado para Linux) */
#include "./include/constantes_fox.h"
#include "./include/estructuras_fox.h"
#include "./include/declaraciones_fox.h"

/**************************************************
***        VARIABLES GLOBALES                  ***
**************************************************/
/* Variables globales externas */
extern uint16_t hilos_listos;
extern int cont_buff_rx;  // Contador de mensajes en el buffer de recepción del supervisor

extern est_veh_t vehiculo;
extern est_bat_t bateria;
extern est_motor_t motor1;
extern est_motor_t motor2;
extern est_motor_t motor3;
extern est_motor_t motor4;
extern est_error_t errores;
extern est_pot_t potencias;
extern est_superv_t supervisor;
extern datosImu_t tDatosImu;

/* Buffers CAN (se asume que en la adaptación la estructura "can_object" se transforma en "struct can_frame") */
extern struct can_frame *buff_rx_superv;    // Buffer de recepción de mensajes del supervisor
extern struct can_frame buff_rx_mot1;       // Buffer de recepción de mensajes del motor 1
extern struct can_frame buff_rx_mot2;       // Buffer de recepción de mensajes del motor 2
extern struct can_frame buff_rx_mot3;       // Buffer de recepción de mensajes del motor 3
extern struct can_frame buff_rx_mot4;       // Buffer de recepción de mensajes del motor 4

/* En la versión original se utiliza "canhdl_t hdl". Para Linux usaremos un socket CAN global */
static int sock;

/* Mutex y condición externos */
extern pthread_mutex_t mDatosImu;
extern pthread_mutex_t mut_supervisor;
extern pthread_mutex_t mut_vehiculo;
extern pthread_mutex_t mut_bateria;
extern pthread_mutex_t mut_motor1;
extern pthread_mutex_t mut_motor2;
extern pthread_mutex_t mut_motor3;
extern pthread_mutex_t mut_motor4;
extern pthread_mutex_t mut_errores;
extern pthread_mutex_t mut_potencias;
extern pthread_mutex_t mut_hdl;
extern pthread_mutex_t mut_hilos_listos;
extern pthread_mutex_t mut_inicio;
extern pthread_cond_t inicio;
extern pthread_mutex_t mut_rx_superv;

/**************************************************
***        DECLARACIÓN DE FUNCIONES           ***
**************************************************/
void can_in_superv(struct can_frame rxmsg);
void manejador_envia_msgCAN(int signo, siginfo_t *info, void *nada);

/* Función para enviar un mensaje CAN mediante SocketCAN */
static int socket_can_write(struct can_frame *frame) {
    return write(sock, frame, sizeof(struct can_frame));
}

/**************************************************
***        FUNCIÓN PRINCIPAL: can_superv        ***
**************************************************/
void *can_superv(void *pi)
{
    int i;
    BOOL fin_local = 0;
    struct can_frame superv_rx_msg;
    struct can_frame *buff_rx_superv_local;
    int cont_buff_rx_local = 0;

    /* Variables para el temporizador POSIX */
    timer_t temporizador;
    struct sigaction accion;
    struct sigevent eventos;
    struct itimerspec programacion;
    struct timespec periodo;
    sigset_t segnales;

    /* Configuración del período del temporizador (definido en TIEMPO_CAN_SUP_NS) */
    periodo.tv_sec = 0;
    periodo.tv_nsec = TIEMPO_CAN_SUP_NS;
    programacion.it_value = periodo;
    programacion.it_interval = periodo;

    /* Configuración de la señal para el temporizador */
    eventos.sigev_signo = SIG_CAN_SUP;  // Se define SIG_CAN_SUP en constantes_fox.h (puede asignarse, por ejemplo, a SIGRTMIN)
    eventos.sigev_notify = SIGEV_SIGNAL;
    eventos.sigev_value.sival_ptr = &temporizador;
    memset(&accion, 0, sizeof(accion));
    accion.sa_sigaction = manejador_envia_msgCAN;  // Manejador de la señal
    accion.sa_flags = SA_SIGINFO;
    sigemptyset(&accion.sa_mask);
    sigaction(SIG_CAN_SUP, &accion, NULL);

    /* Desbloquear la señal SIG_CAN_SUP */
    sigemptyset(&segnales);
    sigaddset(&segnales, SIG_CAN_SUP);
    pthread_sigmask(SIG_UNBLOCK, &segnales, NULL);

    /* Reservar memoria para el buffer local de mensajes CAN */
    buff_rx_superv_local = malloc(TAM_BUFF_RX * sizeof(struct can_frame));
    if (!buff_rx_superv_local) {
        perror("Error al asignar memoria para buff_rx_superv_local");
        pthread_exit(NULL);
    }

    /* Activación del temporizador POSIX */
    timer_create(CLOCK_REALTIME, &eventos, &temporizador);
    timer_settime(temporizador, 0, &programacion, NULL);

    /* Inicializar el socket CAN */
    sock = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (sock < 0) {
        perror("Error al abrir socket CAN");
        pthread_exit(NULL);
    }
    struct ifreq ifr;
    strcpy(ifr.ifr_name, CAN_INTERFACE);  // Por ejemplo: "can0"
    if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
        perror("Error al obtener el índice de la interfaz CAN");
        close(sock);
        pthread_exit(NULL);
    }
    struct sockaddr_can addr;
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Error al hacer bind del socket CAN");
        close(sock);
        pthread_exit(NULL);
    }

    /* Indicar que el hilo CAN del supervisor está listo */
    pthread_mutex_lock(&mut_hilos_listos);
    hilos_listos |= HILO_SUPERV_LISTO;
    pthread_mutex_unlock(&mut_hilos_listos);
    printf("Hilo SUPERV listo\n");
    pthread_mutex_lock(&mut_inicio);
    pthread_cond_wait(&inicio, &mut_inicio);
    pthread_mutex_unlock(&mut_inicio);

    while (!fin_local)
    {
        /* Copiar el contenido del buffer compartido de recepción */
        pthread_mutex_lock(&mut_rx_superv);
        cont_buff_rx_local = cont_buff_rx;
        cont_buff_rx = 0;
        memcpy(buff_rx_superv_local, buff_rx_superv, cont_buff_rx_local * sizeof(struct can_frame));
        pthread_mutex_unlock(&mut_rx_superv);

        /* Procesar cada mensaje recibido */
        for (i = cont_buff_rx_local; i > 0; i--) {
            superv_rx_msg = buff_rx_superv_local[cont_buff_rx_local - i];
            can_in_superv(superv_rx_msg);
        }

        /* Esperar al próximo período */
        nanosleep(&periodo, NULL);

        pthread_mutex_lock(&mut_hilos_listos);
        if ((hilos_listos & HILO_SUPERV_LISTO) == 0)
            fin_local = 1;
        pthread_mutex_unlock(&mut_hilos_listos);
    }

    timer_delete(temporizador);
    free(buff_rx_superv_local);
    close(sock);
    pthread_exit(NULL);
    return NULL;
}

/**************************************************
***        FUNCIÓN: can_in_superv               ***
**************************************************/
void can_in_superv(struct can_frame rxmsg)
{
    switch (rxmsg.can_id)
    {
        case ID_SUPERV_HEARTBEAT:
            pthread_mutex_lock(&mut_supervisor);
            supervisor.heartbeat = (BOOL) rxmsg.data[0];
            supervisor.motor_on[POS_M1] = (BOOL) rxmsg.data[1];
            supervisor.motor_on[POS_M2] = (BOOL) rxmsg.data[2];
            supervisor.motor_on[POS_M3] = (BOOL) rxmsg.data[3];
            supervisor.motor_on[POS_M4] = (BOOL) rxmsg.data[4];
            supervisor.on = TRUE;
            pthread_mutex_unlock(&mut_supervisor);
            break;
        case ID_SUPERV_OFF:
            // Procesar mensaje de apagado si es necesario
            break;
        default:
            // Se puede registrar un mensaje de error o ignorar
            break;
    }
}

/**************************************************
***        FUNCIÓN: manejador_envia_msgCAN       ***
***  Manejador para el envío periódico de mensajes  ***
***  CAN al supervisor.                            ***
**************************************************/
void manejador_envia_msgCAN(int signo, siginfo_t *info, void *nada)
{
    struct can_frame msg_can_out;
    BOOL heartbeat_superv_local = FALSE;
    BYTE uc_aux1 = 0x00, uc_aux2 = 0x00, uc_aux3 = 0x00, uc_aux4 = 0x00;
    uint16_t us_aux1 = 0, us_aux2 = 0, us_aux3 = 0, us_aux4 = 0;
    int16_t s_aux1 = 0, s_aux2 = 0, s_aux3 = 0, s_aux4 = 0;
    uint32_t local_timestamp;
    BYTE local_temp_cel[NUM_CEL_BAT];
    uint16_t local_mv_cel[NUM_CEL_BAT];
    int i, j;
    static uint16_t cnt_ciclos_can = 0;
    static int cont_heartbeat_superv = 0;
    char peticion_off_local = 0;
    static BOOL superv_encendido = FALSE;

    /* Configuración inicial del mensaje CAN.
       En SocketCAN se utiliza msg_can_out.can_id, msg_can_out.can_dlc y msg_can_out.data[].
       Se omiten flags como FF y RTR (se deben incluir en el ID si se requieren, por ejemplo con CAN_EFF_FLAG). */
    
    /* Si es la primera activación, se marca que el supervisor ya está encendido */
    pthread_mutex_lock(&mut_supervisor);
    if ((supervisor.on == TRUE) && (superv_encendido == FALSE))
        superv_encendido = TRUE;
    pthread_mutex_unlock(&mut_supervisor);
    if (superv_encendido == TRUE)
        superv_encendido = 2; // Sólo una vez

    if (cnt_ciclos_can == 0) {
        // (Opcional) Enviar mensajes de inicialización
    }
    cnt_ciclos_can++;
    if (cnt_ciclos_can == 0) cnt_ciclos_can = 1;

    /**************** PERIODO = 1 x TIEMPO_CAN_SUP_NS ****************/
    /* Envío de mensaje: Errores a Supervisor - Críticos */
    msg_can_out.can_id = ID_ECU_SUPERV_ERC;
    msg_can_out.can_dlc = 2;
    pthread_mutex_lock(&mut_errores);
    msg_can_out.data[0] = (BYTE)((MASK_BYTE_ALTO & errores.er_critico_1) >> 8);
    msg_can_out.data[1] = (BYTE)(MASK_BYTE_BAJO & errores.er_critico_1);
    pthread_mutex_unlock(&mut_errores);
    pthread_mutex_lock(&mut_hdl);
    socket_can_write(&msg_can_out);
    pthread_mutex_unlock(&mut_hdl);

    /* Envío de mensaje: Actuaciones del vehículo */
    msg_can_out.can_id = ID_VEHICULO_ACTUACION;
    msg_can_out.can_dlc = 6;
    pthread_mutex_lock(&mut_vehiculo);
    us_aux1 = (uint16_t) roundf(100 * vehiculo.freno);
    us_aux2 = (uint16_t) roundf(100 * vehiculo.acelerador);
    us_aux3 = (uint16_t) roundf(100 * vehiculo.volante);
    pthread_mutex_unlock(&mut_vehiculo);
    msg_can_out.data[0] = (BYTE)((us_aux1 & MASK_BYTE_ALTO) >> 8);
    msg_can_out.data[1] = (BYTE)(us_aux1 & MASK_BYTE_BAJO);
    msg_can_out.data[2] = (BYTE)((us_aux2 & MASK_BYTE_ALTO) >> 8);
    msg_can_out.data[3] = (BYTE)(us_aux2 & MASK_BYTE_BAJO);
    msg_can_out.data[4] = (BYTE)((us_aux3 & MASK_BYTE_ALTO) >> 8);
    msg_can_out.data[5] = (BYTE)(us_aux3 & MASK_BYTE_BAJO);
    pthread_mutex_lock(&mut_hdl);
    socket_can_write(&msg_can_out);
    pthread_mutex_unlock(&mut_hdl);

    /* Envío de mensaje: Lectura de sensores de corriente del vehículo */
    msg_can_out.can_id = ID_VEHICULO_CORRIENTE;
    msg_can_out.can_dlc = 4;
    pthread_mutex_lock(&mut_vehiculo);
    s_aux1 = (int16_t) roundf(100 * vehiculo.i_eje_d);
    s_aux2 = (int16_t) roundf(100 * vehiculo.i_eje_t);
    pthread_mutex_unlock(&mut_vehiculo);
    msg_can_out.data[0] = (BYTE)((s_aux1 & MASK_BYTE_ALTO) >> 8);
    msg_can_out.data[1] = (BYTE)(s_aux1 & MASK_BYTE_BAJO);
    msg_can_out.data[2] = (BYTE)((s_aux2 & MASK_BYTE_ALTO) >> 8);
    msg_can_out.data[3] = (BYTE)(s_aux2 & MASK_BYTE_BAJO);
    pthread_mutex_lock(&mut_hdl);
    socket_can_write(&msg_can_out);
    pthread_mutex_unlock(&mut_hdl);

    /* Envío de mensaje: Estado general batería */
    msg_can_out.can_id = ID_BMS_ESTADO;
    msg_can_out.can_dlc = 7;
    pthread_mutex_lock(&mut_bateria);
    us_aux1 = (uint16_t) roundf((float) bateria.v_pack / 10);
    msg_can_out.data[0] = (BYTE) bateria.num_cel_scan;
    msg_can_out.data[1] = (BYTE) bateria.temp_media;
    msg_can_out.data[2] = (BYTE)((us_aux1 & MASK_BYTE_ALTO) >> 8);
    msg_can_out.data[3] = (BYTE)(us_aux1 & MASK_BYTE_BAJO);
    msg_can_out.data[4] = (BYTE)((bateria.i_pack & MASK_BYTE_ALTO) >> 8);
    msg_can_out.data[5] = (BYTE)(bateria.i_pack & MASK_BYTE_BAJO);
    msg_can_out.data[6] = (BYTE) bateria.soc;
    pthread_mutex_unlock(&mut_bateria);
    pthread_mutex_lock(&mut_hdl);
    socket_can_write(&msg_can_out);
    pthread_mutex_unlock(&mut_hdl);

    /* Envío de mensaje: Alarmas BMS */
    msg_can_out.can_id = ID_BMS_ESTADO_T;
    msg_can_out.can_dlc = 2;  // Se usa DLC=2 según el código original
    pthread_mutex_lock(&mut_bateria);
    msg_can_out.data[0] = (BYTE) bateria.temp_max;
    msg_can_out.data[1] = (BYTE) bateria.cel_temp_max;
    pthread_mutex_unlock(&mut_bateria);
    pthread_mutex_lock(&mut_hdl);
    socket_can_write(&msg_can_out);
    pthread_mutex_unlock(&mut_hdl);

    /**************** PERIODO = 2 x TIEMPO_CAN_SUP_NS ****************/
    if ((cnt_ciclos_can % 2) == 0)
    {
        /* Envío de mensaje: Errores a Supervisor - Graves */
        msg_can_out.can_id = ID_ECU_SUPERV_ERG;
        msg_can_out.can_dlc = 4;
        pthread_mutex_lock(&mut_errores);
        msg_can_out.data[0] = (BYTE)((MASK_BYTE_ALTO & errores.er_grave_1) >> 8);
        msg_can_out.data[1] = (BYTE)(MASK_BYTE_BAJO & errores.er_grave_1);
        msg_can_out.data[2] = (BYTE)((MASK_BYTE_ALTO & errores.er_grave_2) >> 8);
        msg_can_out.data[3] = (BYTE)(MASK_BYTE_BAJO & errores.er_grave_2);
        pthread_mutex_unlock(&mut_errores);
        pthread_mutex_lock(&mut_hdl);
        socket_can_write(&msg_can_out);
        pthread_mutex_unlock(&mut_hdl);

        /* Envío de mensaje: Suspensiones del vehículo */
        msg_can_out.can_id = ID_VEHICULO_SUSPENSION;
        msg_can_out.can_dlc = 8;
        pthread_mutex_lock(&mut_vehiculo);
        us_aux1 = (uint16_t) roundf(100 * vehiculo.susp_ti);
        us_aux2 = (uint16_t) roundf(100 * vehiculo.susp_td);
        us_aux3 = (uint16_t) roundf(100 * vehiculo.susp_di);
        us_aux4 = (uint16_t) roundf(100 * vehiculo.susp_dd);
        pthread_mutex_unlock(&mut_vehiculo);
        msg_can_out.data[0] = (BYTE)((us_aux1 & MASK_BYTE_ALTO) >> 8);
        msg_can_out.data[1] = (BYTE)(us_aux1 & MASK_BYTE_BAJO);
        msg_can_out.data[2] = (BYTE)((us_aux2 & MASK_BYTE_ALTO) >> 8);
        msg_can_out.data[3] = (BYTE)(us_aux2 & MASK_BYTE_BAJO);
        msg_can_out.data[4] = (BYTE)((us_aux3 & MASK_BYTE_ALTO) >> 8);
        msg_can_out.data[5] = (BYTE)(us_aux3 & MASK_BYTE_BAJO);
        msg_can_out.data[6] = (BYTE)((us_aux4 & MASK_BYTE_ALTO) >> 8);
        msg_can_out.data[7] = (BYTE)(us_aux4 & MASK_BYTE_BAJO);
        pthread_mutex_lock(&mut_hdl);
        socket_can_write(&msg_can_out);
        pthread_mutex_unlock(&mut_hdl);

        /* Envío de mensaje: Medidas de la IMU - Acelerómetros */
        msg_can_out.can_id = ID_IMU_DATOS_1;
        msg_can_out.can_dlc = 6;
        pthread_mutex_lock(&mDatosImu);
        s_aux1 = (int16_t) roundf(tDatosImu.pfAccelScal[0] * 100);
        s_aux2 = (int16_t) roundf(tDatosImu.pfAccelScal[1] * 100);
        s_aux3 = (int16_t) roundf(tDatosImu.pfAccelScal[2] * 100);
        pthread_mutex_unlock(&mDatosImu);
        msg_can_out.data[0] = (BYTE)((s_aux1 & MASK_BYTE_ALTO) >> 8);
        msg_can_out.data[1] = (BYTE)(s_aux1 & MASK_BYTE_BAJO);
        msg_can_out.data[2] = (BYTE)((s_aux2 & MASK_BYTE_ALTO) >> 8);
        msg_can_out.data[3] = (BYTE)(s_aux2 & MASK_BYTE_BAJO);
        msg_can_out.data[4] = (BYTE)((s_aux3 & MASK_BYTE_ALTO) >> 8);
        msg_can_out.data[5] = (BYTE)(s_aux3 & MASK_BYTE_BAJO);
        pthread_mutex_lock(&mut_hdl);
        socket_can_write(&msg_can_out);
        pthread_mutex_unlock(&mut_hdl);

        /* Envío de mensaje: Medidas de la IMU - Giroscopios */
        msg_can_out.can_id = ID_IMU_DATOS_2;
        msg_can_out.can_dlc = 6;
        pthread_mutex_lock(&mDatosImu);
        s_aux1 = (int16_t) roundf(tDatosImu.pfGyroScal[0] * 100);
        s_aux2 = (int16_t) roundf(tDatosImu.pfGyroScal[1] * 100);
        s_aux3 = (int16_t) roundf(tDatosImu.pfGyroScal[2] * 100);
        pthread_mutex_unlock(&mDatosImu);
        msg_can_out.data[0] = (BYTE)((s_aux1 & MASK_BYTE_ALTO) >> 8);
        msg_can_out.data[1] = (BYTE)(s_aux1 & MASK_BYTE_BAJO);
        msg_can_out.data[2] = (BYTE)((s_aux2 & MASK_BYTE_ALTO) >> 8);
        msg_can_out.data[3] = (BYTE)(s_aux2 & MASK_BYTE_BAJO);
        msg_can_out.data[4] = (BYTE)((s_aux3 & MASK_BYTE_ALTO) >> 8);
        msg_can_out.data[5] = (BYTE)(s_aux3 & MASK_BYTE_BAJO);
        pthread_mutex_lock(&mut_hdl);
        socket_can_write(&msg_can_out);
        pthread_mutex_unlock(&mut_hdl);

        /* Envío de mensaje: Medidas de la IMU - Timestamp */
        msg_can_out.can_id = ID_IMU_DATOS_3;
        msg_can_out.can_dlc = 4;
        pthread_mutex_lock(&mDatosImu);
        local_timestamp = (uint32_t) roundf((float)tDatosImu.iTimeStamp / 62500);
        pthread_mutex_unlock(&mDatosImu);
        us_aux1 = (uint16_t)((local_timestamp & 0xFFFF0000) >> 16);
        us_aux2 = (uint16_t)(local_timestamp & 0x0000FFFF);
        msg_can_out.data[0] = (BYTE)((us_aux1 & MASK_BYTE_ALTO) >> 8);
        msg_can_out.data[1] = (BYTE)(us_aux1 & MASK_BYTE_BAJO);
        msg_can_out.data[2] = (BYTE)((us_aux2 & MASK_BYTE_ALTO) >> 8);
        msg_can_out.data[3] = (BYTE)(us_aux2 & MASK_BYTE_BAJO);
        pthread_mutex_lock(&mut_hdl);
        socket_can_write(&msg_can_out);
        pthread_mutex_unlock(&mut_hdl);

        /* Envío de mensaje: Medidas de la IMU - Posición LLH */
        msg_can_out.can_id = ID_IMU_DATOS_4;
        msg_can_out.can_dlc = 6;
        pthread_mutex_lock(&mDatosImu);
        s_aux1 = (int16_t) round(tDatosImu.pdLLHPos[0] * 100);
        s_aux2 = (int16_t) round(tDatosImu.pdLLHPos[1] * 100);
        s_aux3 = (int16_t) round(tDatosImu.pdLLHPos[3] * 100);
        pthread_mutex_unlock(&mDatosImu);
        msg_can_out.data[0] = (BYTE)((s_aux1 & MASK_BYTE_ALTO) >> 8);
        msg_can_out.data[1] = (BYTE)(s_aux1 & MASK_BYTE_BAJO);
        msg_can_out.data[2] = (BYTE)((s_aux2 & MASK_BYTE_ALTO) >> 8);
        msg_can_out.data[3] = (BYTE)(s_aux2 & MASK_BYTE_BAJO);
        msg_can_out.data[4] = (BYTE)((s_aux3 & MASK_BYTE_ALTO) >> 8);
        msg_can_out.data[5] = (BYTE)(s_aux3 & MASK_BYTE_BAJO);
        pthread_mutex_lock(&mut_hdl);
        socket_can_write(&msg_can_out);
        pthread_mutex_unlock(&mut_hdl);

        /* Envío de mensaje: Medidas de la IMU - Módulo velocidad NED y vector Euler */
        msg_can_out.can_id = ID_IMU_DATOS_5;
        msg_can_out.can_dlc = 8;
        pthread_mutex_lock(&mDatosImu);
        s_aux1 = (int16_t) roundf(tDatosImu.pfNEDVeloc[3] * 100);
        s_aux2 = (int16_t) roundf(tDatosImu.pfEuler[0] * 100);
        s_aux3 = (int16_t) roundf(tDatosImu.pfEuler[1] * 100);
        s_aux4 = (int16_t) roundf(tDatosImu.pfEuler[2] * 100);
        pthread_mutex_unlock(&mDatosImu);
        msg_can_out.data[0] = (BYTE)((s_aux1 & MASK_BYTE_ALTO) >> 8);
        msg_can_out.data[1] = (BYTE)(s_aux1 & MASK_BYTE_BAJO);
        msg_can_out.data[2] = (BYTE)((s_aux2 & MASK_BYTE_ALTO) >> 8);
        msg_can_out.data[3] = (BYTE)(s_aux2 & MASK_BYTE_BAJO);
        msg_can_out.data[4] = (BYTE)((s_aux3 & MASK_BYTE_ALTO) >> 8);
        msg_can_out.data[5] = (BYTE)(s_aux3 & MASK_BYTE_BAJO);
        msg_can_out.data[6] = (BYTE)((s_aux4 & MASK_BYTE_ALTO) >> 8);
        msg_can_out.data[7] = (BYTE)(s_aux4 & MASK_BYTE_BAJO);
        pthread_mutex_lock(&mut_hdl);
        socket_can_write(&msg_can_out);
        pthread_mutex_unlock(&mut_hdl);
    }

    /**************** PERIODO = 4 x TIEMPO_CAN_SUP_NS ****************/
    if ((cnt_ciclos_can % 4) == 0)
    {
        /* Envío de señal de apagado al supervisor si procede */
        pthread_mutex_lock(&mut_supervisor);
        peticion_off_local = supervisor.peticion_off;
        pthread_mutex_unlock(&mut_supervisor);
        if (peticion_off_local == TRUE)
        {
            msg_can_out.can_id = ID_SUPERV_OFF;
            msg_can_out.can_dlc = 1;
            msg_can_out.data[0] = (BYTE) peticion_off_local;
            pthread_mutex_lock(&mut_hdl);
            socket_can_write(&msg_can_out);
            pthread_mutex_unlock(&mut_hdl);
            pthread_mutex_lock(&mut_supervisor);
            supervisor.enviado_off = TRUE;
            pthread_mutex_unlock(&mut_supervisor);
        }

        /* Envío de mensajes de conversion A/D para cada motor (ejemplo para motor 1) */
        msg_can_out.can_id = ID_MOTOR1_CONV_AD1;
        msg_can_out.can_dlc = 5;
        pthread_mutex_lock(&mut_motor1);
        msg_can_out.data[0] = (BYTE) roundf(motor1.freno * 51);
        msg_can_out.data[1] = (BYTE) roundf(motor1.acel * 51);
        msg_can_out.data[2] = (BYTE) roundf(motor1.v_pot * 3);
        msg_can_out.data[3] = (BYTE) roundf(motor1.v_aux * 6);
        msg_can_out.data[4] = (BYTE) roundf(motor1.v_bat * 3);
        pthread_mutex_unlock(&mut_motor1);
        pthread_mutex_lock(&mut_hdl);
        socket_can_write(&msg_can_out);
        pthread_mutex_unlock(&mut_hdl);

        /* Se debe continuar de forma análoga para el resto de motores y conversiones.
           Cada bloque se adapta configurando:
             - msg_can_out.can_id
             - msg_can_out.can_dlc
             - msg_can_out.data[] (mediante cálculos y uso de mutex)
             - Enviar con socket_can_write()
        */
    }

    /**************** PERIODO = 5, 6, 8, 10, 120 x TIEMPO_CAN_SUP_NS ****************/
    if ((cnt_ciclos_can % 10) == 0)
    {
        /* Envío de mensaje: Heartbeat */
        msg_can_out.can_id = ID_WATCHDOG;
        msg_can_out.can_dlc = 1;
        msg_can_out.data[0] = 0x00;
        pthread_mutex_lock(&mut_hdl);
        socket_can_write(&msg_can_out);
        pthread_mutex_unlock(&mut_hdl);

        /* Comprobación del watchdog del supervisor */
        pthread_mutex_lock(&mut_supervisor);
        heartbeat_superv_local = supervisor.heartbeat;
        pthread_mutex_unlock(&mut_supervisor);
        if (heartbeat_superv_local == FALSE)
        {
            cont_heartbeat_superv++;
            if (cont_heartbeat_superv >= LIMITE_ERL_HEARTBEAT)
            {
                pthread_mutex_lock(&mut_errores);
                errores.er_leve_1 |= ERL_ECU_COM_SUPERV;
                pthread_mutex_unlock(&mut_errores);
            }
            if (cont_heartbeat_superv >= LIMITE_ERG_HEARTBEAT)
            {
                pthread_mutex_lock(&mut_errores);
                errores.er_leve_1 &= ~ERL_ECU_COM_SUPERV;
                errores.er_grave_1 |= ERG_ECU_COM_SUPERV;
                pthread_mutex_unlock(&mut_errores);
            }
            if (cont_heartbeat_superv > 30)
                cont_heartbeat_superv = 30;
        }
        else
        {
            cont_heartbeat_superv = 0;
            pthread_mutex_lock(&mut_errores);
            errores.er_leve_1 &= ~ERL_ECU_COM_SUPERV;
            errores.er_grave_1 &= ~ERG_ECU_COM_SUPERV;
            pthread_mutex_unlock(&mut_errores);
            pthread_mutex_lock(&mut_supervisor);
            supervisor.heartbeat = FALSE;
            pthread_mutex_unlock(&mut_supervisor);
        }
    }

    if ((cnt_ciclos_can % 120) == 0)
    {
        /* Envío de mensaje: Versión de la ECU */
        msg_can_out.can_id = ID_SUPERV_VER;
        msg_can_out.can_dlc = 1;
        msg_can_out.data[0] = (BYTE)(VERSION_ECU / 10);
        pthread_mutex_lock(&mut_hdl);
        socket_can_write(&msg_can_out);
        pthread_mutex_unlock(&mut_hdl);

        /* Envío de mensajes: Zonas muertas para cada motor.
           Adaptar cada bloque similarmente, configurando can_id, can_dlc, data[] y enviando mediante socket_can_write(). */
    }
}
