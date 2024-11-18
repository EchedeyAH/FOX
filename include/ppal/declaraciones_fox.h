/**********************************************************************************
***										***	
***	Fichero: declaraciones_fox.h						***
***	Fecha: 22/02/2013							***
***	Autor: Elena Gonzalez							***
***	Descripcion: Cabecera de declaraciones de funciones			***
***										***
**********************************************************************************/
#include <mqueue.h>

#ifndef DECLARACIONES_FOX_H_
#define DECLARACIONES_FOX_H_

	#ifndef _CAN_OBJECT_FOX_H
	#define _CAN_OBJECT_FOX_H
		struct frinf
		{
		   BYTE DLC:4;
		   BYTE res:2;
		   BYTE RTR:1;
		   BYTE  FF:1;
		};

		struct can_object
		{
			union
			{
			 struct frinf inf;
			   BYTE octet;
			} frame_inf;

			uint32_t id;
			BYTE data[24];
		};	
	#endif	// _CAN_OBJECT_FOX_H

	/* PROTOTIPOS DE FUNCIONES */
	/* Hilos */
	void *errores_main (void *);
	void *imuMain (void *);
	void *adq_main (void *);
	void *adq_ao (void *);
	void *can_rx (void *);
	void *canMotor (void *);
	void *can_superv (void *);
	void *gestion_potencia_main (void *);
	void *ctrl_traccion_main (void *);
	void *ctrl_estabilidad_main (void *);

	void recibe_datos_cola_can2(mqd_t);
	void envia_datos_cola_imu(mqd_t);
	int imprime_pantalla (uint16_t);

	int envia_can(int, int);
	int interpreta_can_entrada(int, struct can_object *);
	void can_in_superv(struct can_object);
	void manejador_envia_msgCAN(int, siginfo_t *, void *);

	void actuacion_critica();
	void actuacion_grave();
	void actuacion_leve();
	void bms_errores();
	void motor_errores(uint8_t num_motor);
	void sensores_errores(void);

	char inicia_tadq (void);
	void calibra_adq (void);
	void cda_wr (BYTE, float, BYTE);

	void lee_dato_analogico(float *);
	void interpreta_entrada_analog(float *);
	void modif_dato_digital(uint16_t *,uint16_t *);
	void comprueba_canales(void);
	void configura_rangos(void);
	void selec_canal(void);
	void config_byte_ctrl(void);
	void habilitar_pacer(uint8_t pacer);
	void habilitar_ctrl_int_fifo_da(uint8_t ctrl);
	int lee_fichero_cfg(void);
	uint8_t rango_canal(char *cadena);

	int abrirPuertoCom (void);
	int cerrarPuertoCom (void);
	int purgarPuertoCom (void);
	int leerPuertoCom (BYTE *, int);
	int escribirPuertoCom (BYTE *, int);
	int enviarComandoImu (mensajeImu_t *);
	int recibirComandoImu (mensajeImu_t *);
	int tratarRespuesta (mensajeImu_t *);

#endif	// DECLARACIONES_FOX_H_

