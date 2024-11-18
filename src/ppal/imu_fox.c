///////////////////////////////////////////////////////////////////////////////
//Hilo de la IMU
//Realizado por Alejandro J. Oliva Torres
//Ref: 3DM-GX3-35-Data-Communications-Protocol.pdf
///////////////////////////////////////////////////////////////////////////////
//Estructura de un mensaje:
//CABU,CABE: dos bytes de valor fijo
//DESC: descriptor del tipo de mensaje
//TAMT: tamagno total en bytes de todos los campos
//CAMPOS: siempre hay uno o mas
//CKS1, CKS2: dos bytes de checksum
//  |-1B-|-1B-|-1B-|-1B-|-----NB----|-1B-|-1B-|  
//  |CABU|CABE|DESC|TAMT|___CAMPOS__|CKS1|CKS2|  
//Estructura de cada campo:
//TAMC: tamgno en bytes del campo
//DESC: descriptor del tipo de campo
//DATOS: su tamagno puede ser 0
//  |-1B-|-1B-|----NB----|                       
//  |TAMC|DESC|___DATOS__|                       
//
//Se ha optado por la obtención de datos a partir de "polling".
//Los campos de las respuestas de la IMU se determinan mediante la 
//configuracion de la misma.
//Esta configuracion puede hacerse mediante mensajes pero, por cuestiones de
//tiempo y simplicidad, se ha optado por hacer esta configuracion aparte,
//en Windows, y dejarla por defecto.
//Este codigo permite la adicion de nuevos mensajes si se desea.
///////////////////////////////////////////////////////////////////////////////

/**************************************************
***		CABECERAS			***
**************************************************/
#include <termios.h> //interfaz puerto serie
#include <fcntl.h>  //control de ficheros (open, close, read, write...)
#include <stdio.h>	//entrada-salida estandar (printf, scanf...)
#include <stdlib.h>	//memoria dinamica (malloc...), exit, system...
#include <stdint.h>	//tipos enteros: (u)int8_t (u)int16_t (u)int32_t (u)int64_t
#include <unistd.h>	//constantes y tipos UNIX
#include <errno.h>	//constantes error
#include <time.h> 	//tiempos y temporizadores
#include <pthread.h> //hilos POSIX
#include <signal.h>  //segnales
#include <sys/types.h>
#include <sys/stat.h>

#include "./include/constantes_fox.h"
#include "./include/estructuras_fox.h"
#include "./include/declaraciones_fox.h"
///////////////////////////////////////////////////////////////////////////////

////////////////Variables globales al programa y sus mutex/////////////////////
extern datosImu_t tDatosImu;
extern pthread_mutex_t mDatosImu;
extern uint16_t hilos_listos;

extern pthread_mutex_t mut_hilos_listos;
extern pthread_mutex_t mut_inicio;
extern pthread_cond_t inicio;
extern est_error_t errores;
extern pthread_mutex_t mut_errores;

////////////////Variables globales al archivo//////////////////////////////////
int iManejadorCom;

////////////////Rutina principal del hilo//////////////////////////////////////
//Abre puerto COM, manda ping, hace poll continuo de los datos del AHRS y GPS.
//Cuando sale del bucle por interrupcion cierra el puerto COM y sale del hilo.
void *imuMain (void *pi)
{
	//Variables locales
	int i;
	char fin_local = 0;
	mensajeImu_t tMensajeImuTx;
	mensajeImu_t tMensajeImuRx;
	BYTE bErrorCode;
	
	//Reserva de mensaje minimo
	tMensajeImuRx.bNumCampos = 1;
	tMensajeImuRx.ppbDatos = (BYTE **) calloc (1,sizeof (BYTE *));
	tMensajeImuRx.ppbDatos[0] = (BYTE *) calloc (2,sizeof (BYTE));
	
	tMensajeImuTx.bNumCampos = 1;
	tMensajeImuTx.ppbDatos = (BYTE **) calloc (1,sizeof (BYTE *));
	tMensajeImuTx.ppbDatos[0] = (BYTE *) calloc (2,sizeof (BYTE));
	
	//Inicializacion del puerto COM
	bErrorCode = abrirPuertoCom();
	if (bErrorCode != ERROK)
	{
		//salir o repetir?
	}

	//Mandamos primero un ping
	////////////////////PING///////////////////////
	tMensajeImuTx.bDescTipo = CABDESCBASE;
	tMensajeImuTx.bNumCampos = 1;
	tMensajeImuTx.ppbDatos[0] = (BYTE *) realloc (tMensajeImuTx.ppbDatos[0], 2 * sizeof (BYTE));
	tMensajeImuTx.ppbDatos[0][0] = 2;
	tMensajeImuTx.ppbDatos[0][1] = DESCPING;
	
	bErrorCode = enviarComandoImu(&tMensajeImuTx);
	if (bErrorCode != ERROK)
	{
		//salir o repetir?
	}

	bErrorCode = recibirComandoImu(&tMensajeImuRx);
	if (bErrorCode != ERROK)
	{
		//salir o repetir?
	}
	
	bErrorCode = tratarRespuesta(&tMensajeImuRx);
	if (bErrorCode == ERRIMUESTADO)
	{
		//La IMU esta en estado de error
	}
	///////////////////////////////////////////////

	/* Indica que el hilo de la IMU esta listo */
	pthread_mutex_lock(&mut_hilos_listos);
	hilos_listos |= HILO_IMU_LISTO;
	pthread_mutex_unlock(&mut_hilos_listos);
	printf("Hilo IMU listo\n");
	pthread_mutex_lock(&mut_inicio);
	pthread_cond_wait(&inicio, &mut_inicio);
	pthread_mutex_unlock(&mut_inicio);
	
	//Bucle de envio-respuesta
	while (!fin_local)
	{
		////////////////////POLL AHRS///////////////////////
		tMensajeImuTx.bDescTipo = CABDESCCOM;
		tMensajeImuTx.bNumCampos = 1;
		tMensajeImuTx.ppbDatos[0] = (BYTE *) realloc (tMensajeImuTx.ppbDatos[0], 4 * sizeof (BYTE));
		tMensajeImuTx.ppbDatos[0][0] = 4;
		tMensajeImuTx.ppbDatos[0][1] = DESCPOLLAHRS;
		tMensajeImuTx.ppbDatos[0][2] = 0x01;
		tMensajeImuTx.ppbDatos[0][3] = 0x00;

		bErrorCode = enviarComandoImu(&tMensajeImuTx);
		if (bErrorCode != ERROK) 
		{
			printf("Fallo en el envio. Error: %#X\n",bErrorCode);
			//salir o repetir?
		}

		bErrorCode = recibirComandoImu(&tMensajeImuRx);
		if (bErrorCode != ERROK) 
		{
			printf("Fallo en la recepcion. Error: %#X\n",bErrorCode);
			//salir o repetir?
		}
		
		bErrorCode = tratarRespuesta(&tMensajeImuRx);
		if (bErrorCode != ERROK) 
		{
			printf("Fallo en la respuesta. Error: %#X\n",bErrorCode);
			//salir o repetir?
		}
		////////////////////////////////////////////////////

		////////////////////POLL GPS////////////////////////
		tMensajeImuTx.bDescTipo = CABDESCCOM;
		tMensajeImuTx.bNumCampos = 1;
		tMensajeImuTx.ppbDatos[0] = (BYTE *) realloc (tMensajeImuTx.ppbDatos[0], 4 * sizeof (BYTE));
		tMensajeImuTx.ppbDatos[0][0] = 4;
		tMensajeImuTx.ppbDatos[0][1] = DESCPOLLGPS;
		tMensajeImuTx.ppbDatos[0][2] = 0x01;
		tMensajeImuTx.ppbDatos[0][3] = 0x00;

		bErrorCode = enviarComandoImu(&tMensajeImuTx);
		if (bErrorCode != ERROK) 
		{
			printf("Fallo en el envio. Error: %#X\n",bErrorCode);
			//salir o repetir?
		}

		bErrorCode = recibirComandoImu(&tMensajeImuRx);
		if (bErrorCode != ERROK) 
		{
			printf("Fallo en la recepcion. Error: %#X\n",bErrorCode);
			//salir o repetir?
		}
		
		bErrorCode = tratarRespuesta(&tMensajeImuRx);
		if (bErrorCode != ERROK) 
		{
			printf("Fallo en la respuesta. Error: %#X\n",bErrorCode);
			//salir o repetir?
		}
		////////////////////////////////////////////////////
		pthread_mutex_lock(&mut_hilos_listos);
		if ((hilos_listos&HILO_IMU_LISTO)==0x0000)
			fin_local = 1;
		pthread_mutex_unlock(&mut_hilos_listos);
	}

	//Cierre del puerto COM
	bErrorCode = cerrarPuertoCom();
	if (bErrorCode != ERROK)
	{
		//salir o repetir?
	}
	
	//Liberacion de las reservas. Al ser ppbDatos una tabla, hay
	//que liberar todos sus elementos
	for(i=0;i<tMensajeImuRx.bNumCampos;i++)
		free(tMensajeImuRx.ppbDatos[i]);
	free(tMensajeImuRx.ppbDatos);
	
	for(i=0;i<tMensajeImuTx.bNumCampos;i++)
		free(tMensajeImuTx.ppbDatos[i]);
	free(tMensajeImuTx.ppbDatos);

	pthread_exit (NULL);
	return(NULL);
}
///////////////////////////////////////////////////////////////////////////////

////////////////abrirPuertoCom/////////////////////////////////////////////////
//Abre y configura el puerto serie.
int abrirPuertoCom(void)
{
	struct termios tOpciones;
	
	iManejadorCom = open(PUERTOCOM, O_RDWR | O_NOCTTY);
	if (iManejadorCom == -1)
		return ERRPCOMOPEN;
		
	//Obtiene opciones actuales del puerto
	tcgetattr(iManejadorCom, &tOpciones);

	//Establece velocidad del puerto
	cfsetospeed(&tOpciones, BAUDRATE);
	cfsetispeed(&tOpciones, BAUDRATE);

	//Numero de bits de datos a 8 por caracter
	tOpciones.c_cflag &= ~CSIZE;
	tOpciones.c_cflag |= CS8;

	//Un bit de parada
	tOpciones.c_cflag &= ~CSTOPB;

	//Sin paridad
	tOpciones.c_cflag &=~PARENB;

	//Modo no canonico (procesado raw, no eco, etc.)
	tOpciones.c_iflag = IGNPAR; // ignora paridad
	tOpciones.c_oflag = 0;
	tOpciones.c_lflag = 0;

	//Tiempos de expiracion
	tOpciones.c_cc[VMIN] = 0;	//Sin bloqueo de lectura
	tOpciones.c_cc[VTIME] = 1;  //Tiempo exp. entre caract.= x*0.1 s.

	//Modo local y habilitar receptor
	tOpciones.c_cflag |= (CLOCAL | CREAD);

	//Purgar el buffer del puerto
	if (purgarPuertoCom() != ERROK)
		return ERRPCOMPURGA;

	//Establecer las nuevas opciones
	if (tcsetattr(iManejadorCom, TCSANOW, &tOpciones) == -1)
		return ERRPCOMCONFIG;

	//Purgar el buffer del puerto
	if (purgarPuertoCom() != ERROK)
		return ERRPCOMPURGA;

	return ERROK;
}
///////////////////////////////////////////////////////////////////////////////

////////////////cerrarPuertoCom////////////////////////////////////////////////
//Cierra el puerto COM.
int cerrarPuertoCom(void)
{
	if (close(iManejadorCom) ==-1)
		return ERRPCOMCLOSE;
	return ERROK;
}
///////////////////////////////////////////////////////////////////////////////

////////////////purgarPuertoCom////////////////////////////////////////////////
//Borra los buffers de entrada y salida del puerto COM.
int purgarPuertoCom(void)
{
	if (tcflush(iManejadorCom,TCIOFLUSH) == -1)
		return ERRPCOMPURGA;
	return ERROK;
}
///////////////////////////////////////////////////////////////////////////////

////////////////leerPuertoCom//////////////////////////////////////////////////
//Lee del puerto COM el numero de bytes especificado.
int leerPuertoCom(BYTE* pbBytesLeidos, int iNumeroBytes)
{	
	int iTamRx;
	
	iTamRx = read(iManejadorCom, pbBytesLeidos, iNumeroBytes);
	
	if (iTamRx == 0)
		return ERRPCOMVACIO;
	
	if (iTamRx < 0)
		return ERRPCOMREAD;
		
	return ERROK;
}
///////////////////////////////////////////////////////////////////////////////

////////////////escribirPuertoCom//////////////////////////////////////////////
//Envia el numero de bytes especificado por el puerto COM.
int escribirPuertoCom(BYTE* pbBytesAEscribir, int iNumeroBytes)
{
	if (write(iManejadorCom, pbBytesAEscribir, iNumeroBytes) == -1)
		return ERRPCOMWRITE;
	return ERROK;
}
///////////////////////////////////////////////////////////////////////////////

////////////////enviarComando//////////////////////////////////////////////////
//Prepara y envia un comando.
int enviarComandoImu (mensajeImu_t *ptMensajeTx)
{
	int i;
	BYTE bChecksum1, bChecksum2; 
	BYTE *pbMensajeTx; 	//Mensaje completo.
	BYTE bNumCampos; 	//Numero de campos.
	BYTE bDescCampo; 	//Descriptor del mensaje.
	BYTE bLongCampo; 	//Long. de cada campo.
	BYTE bIndice; 		//Long. de todos los campos.
	BYTE bLongTotal; 	//Long. del mensaje completo.
		
	//Long. total de partida: cabecera+cola
	bLongTotal = 4 + 2;
	pbMensajeTx = (BYTE *) malloc (sizeof (BYTE) * bLongTotal);
	bNumCampos = ptMensajeTx->bNumCampos;
	
	bIndice = 0;
	for (i=0;i<bNumCampos;i++)
	{
		bLongCampo = ptMensajeTx->ppbDatos[i][0];
		bDescCampo = ptMensajeTx->ppbDatos[i][1];
		
		bLongTotal += bLongCampo;
		pbMensajeTx = (BYTE *) realloc (pbMensajeTx, sizeof (BYTE) * bLongTotal);
		
		memcpy(pbMensajeTx+4+bIndice,ptMensajeTx->ppbDatos[i],sizeof (BYTE) * bLongCampo);

		bIndice = bLongTotal - 6;
	}
	
	//bIndice sirve para saltar de campo a campo pero su valor final
	//tambien coincide con la longitud total de los campos
	pbMensajeTx[0] = CABU;
	pbMensajeTx[1] = CABE;
	pbMensajeTx[2] = ptMensajeTx->bDescTipo;
	pbMensajeTx[3] = bIndice;
	
	//Agnadido del checksum
	bChecksum1=0; bChecksum2=0;
	for (i=0;i<(bLongTotal - 2);i++)
	{
		bChecksum1 += pbMensajeTx[i];
		bChecksum2 += bChecksum1;
	}
	pbMensajeTx[bLongTotal - 2] = bChecksum1;
	pbMensajeTx[bLongTotal - 1] = bChecksum2;

	if (escribirPuertoCom(pbMensajeTx, bLongTotal) != ERROK)
	{
		free(pbMensajeTx);
		return ERRIMUTX;		
	}

	free(pbMensajeTx);
	return ERROK;
}
///////////////////////////////////////////////////////////////////////////////

////////////////recibirComando/////////////////////////////////////////////////
//Lee lo recibido por el puerto y fabrica el mensaje recibido.
int recibirComandoImu(mensajeImu_t *ptMensajeRx)
{

	int i;
	uint16_t iChecksumRx, iChecksum;
	BYTE bChecksumRx1, bChecksumRx2;
	BYTE bChecksum1, bChecksum2;
	BYTE *pbMensajeRx;
	BYTE bNumCampos;
	BYTE bLongCampo;
	BYTE bLongCamposT; //Long. total de los campos.
	BYTE bLongTotal;
	BYTE bIndice;
	struct termios tOpcionesIni, tOpcionesNuevas;
	struct timespec espera = {0,ESPERA};
	
	pbMensajeRx = (BYTE *) calloc (TAMBUFFRX, sizeof (BYTE));

	tcgetattr(0,&tOpcionesIni);
	tOpcionesNuevas = tOpcionesIni;
	tOpcionesNuevas.c_lflag &= ~ICANON;
	tOpcionesNuevas.c_cc[VMIN] = 0;
	tOpcionesNuevas.c_cc[VTIME] = 0;
	tcsetattr(0, TCSANOW, &tOpcionesNuevas);
	
	nanosleep(&espera,NULL);  //Espera a que llegue el mensaje
	
	if (leerPuertoCom (pbMensajeRx, TAMBUFFRX) != ERROK)
	{
		free (pbMensajeRx);
		return ERRIMURX;
	}
	
	tcsetattr(0, TCSANOW, &tOpcionesIni);

	bLongCamposT = pbMensajeRx[3];
	bLongTotal = bLongCamposT + 6;
	
	//Comprobacion del mensaje
	if ((pbMensajeRx[0] != CABU)||(pbMensajeRx[1] != CABE)
		|| (bLongCamposT < 2))
	{
		free (pbMensajeRx);
		return ERRIMURX;
	}

	//Comprobacion del checksum
	bChecksum1 = 0; bChecksum2 = 0;
	for (i=0; i<(bLongTotal - 2); i++)
	{
		bChecksum1 += pbMensajeRx[i];
		bChecksum2 += bChecksum1;
	}
	iChecksum = ((uint16_t) bChecksum1 << 8) + (uint16_t) bChecksum2;
	
	bChecksumRx1 = pbMensajeRx[bLongTotal - 2];
	bChecksumRx2 = pbMensajeRx[bLongTotal - 1];
	iChecksumRx = ((uint16_t) bChecksumRx1 << 8) + (uint16_t) bChecksumRx2;

	if (iChecksumRx != iChecksum)
	{
		free (pbMensajeRx);
		return ERRIMURX;
	}

	bIndice = 0;
	bNumCampos = 0;
	while (bIndice < bLongCamposT)
	{
		bNumCampos++;
		bLongCampo = pbMensajeRx[4 + bIndice];					
		ptMensajeRx->ppbDatos = (BYTE **) realloc (ptMensajeRx->ppbDatos, bNumCampos * sizeof (BYTE *));
		ptMensajeRx->ppbDatos[bNumCampos-1] = (BYTE *) realloc (ptMensajeRx->ppbDatos[bNumCampos-1], bLongCampo * sizeof (BYTE));
		memcpy (ptMensajeRx->ppbDatos[bNumCampos-1], pbMensajeRx+4+bIndice, bLongCampo * sizeof (BYTE));
		bIndice += bLongCampo;
	}

	if (bIndice != bLongCamposT)
	{
		free (pbMensajeRx);
		return ERRIMURX;
	}
	
	ptMensajeRx->bDescTipo = pbMensajeRx[2];
	ptMensajeRx->bNumCampos = bNumCampos;
	
	free (pbMensajeRx);
	return ERROK;
}
///////////////////////////////////////////////////////////////////////////////

////////////////tratarRespuesta////////////////////////////////////////////////
//Comprueba el contenido del mensaje recibido y guarda las variables.
int tratarRespuesta (mensajeImu_t *ptMensajeRx)
{
	enum tipado_t {NADA,OCT,ENTERO16,
	ENTERO32,FLOTANTE,DOBLE
	} eTipado;
	int i,j,k;
	BYTE bDescTipo,bNumCampos,bTipoDato;
	BYTE bTamDatoT,bTamDato,bDimDato;
	void *pvDatoFinal;
	BYTE *pbAuxDato;

	pbAuxDato = (BYTE *) calloc (1,sizeof (BYTE));
	bDescTipo = ptMensajeRx->bDescTipo;
	bNumCampos = ptMensajeRx->bNumCampos;

	for (i=0;i<bNumCampos;i++)
	{
		bTamDatoT = (ptMensajeRx->ppbDatos[i][0])-2;
		bTipoDato = ptMensajeRx->ppbDatos[i][1];
		
		switch (bDescTipo)
		{
			case CABDESCBASE:
				switch (bTipoDato)
				{
					case DESCACK:
						if (ptMensajeRx->ppbDatos[i][3] != ERROK)
							return ERRIMUESTADO;
						return ERROK;
					break;
					default:
						return ERRIMUMENSAJE;
				}
			break;
			case CABDESCAHRS:
				switch (bTipoDato)
				{
					case DESCSCALACCEL:
						bDimDato = 3;
						bTamDato = 4;
						eTipado = FLOTANTE;
						pthread_mutex_lock (&mDatosImu);
						pvDatoFinal = tDatosImu.pfAccelScal;
						pthread_mutex_unlock (&mDatosImu);
					break;
					case DESCSCALGYRO:
						bDimDato = 3;
						bTamDato = 4;
						eTipado = FLOTANTE;
						pthread_mutex_lock (&mDatosImu);
						pvDatoFinal = tDatosImu.pfGyroScal;
						pthread_mutex_unlock (&mDatosImu);
					break;
					case DESCEULER:
						bDimDato = 3;
						bTamDato = 4;
						eTipado = FLOTANTE;
						pthread_mutex_lock (&mDatosImu);
						pvDatoFinal = tDatosImu.pfEuler;
						pthread_mutex_unlock (&mDatosImu);
					break;					
					case DESCINTTIME:
						bDimDato = 1;
						bTamDato = 4;
						eTipado = ENTERO32;
						pthread_mutex_lock (&mDatosImu);
						pvDatoFinal = &(tDatosImu.iTimeStamp);
						pthread_mutex_unlock (&mDatosImu);
					break;
					default:
						return ERRIMUMENSAJE;
				}
			break;
			case CABDESCGPS:
				switch (bTipoDato)
				{
					case DESCLLHPOS:
						bDimDato = 4;
						bTamDato = 8;
						eTipado = DOBLE;
						pthread_mutex_lock (&mDatosImu);
						pvDatoFinal = tDatosImu.pdLLHPos;
						pthread_mutex_unlock (&mDatosImu);
					break;
					case DESCNEDVELO:
						bDimDato = 6;
						bTamDato = 4;
						eTipado = FLOTANTE;
						pthread_mutex_lock (&mDatosImu);
						pvDatoFinal = tDatosImu.pfNEDVeloc;
						pthread_mutex_unlock (&mDatosImu);
					break;
					default:
						return ERRIMUMENSAJE;
				}
			break;
			default:
				return ERRIMUMENSAJE;
		}

		pbAuxDato = (BYTE *) realloc (pbAuxDato, bTamDato * sizeof (BYTE));
		
		for (j=0;j<bDimDato;j++)
		{
			//Invierte el orden de los bytes del dato
			for (k=0;k<bTamDato;k++)
				pbAuxDato[k] = ptMensajeRx->ppbDatos[i][2+(bTamDato*j)+(bTamDato-1-k)];

			switch (eTipado)
			{
				case ENTERO32:
					pthread_mutex_lock (&mDatosImu);
					*(((uint32_t *)pvDatoFinal)+j) = *(uint32_t *) pbAuxDato;
					pthread_mutex_unlock (&mDatosImu);
				break;
				case FLOTANTE:
					pthread_mutex_lock (&mDatosImu);
				 	*(((float *)pvDatoFinal)+j) = *(float *) pbAuxDato;
					pthread_mutex_unlock (&mDatosImu);
				break;
				case DOBLE:
					pthread_mutex_lock (&mDatosImu);
					*(((double *)pvDatoFinal)+j) = *(double *) pbAuxDato;
					pthread_mutex_unlock (&mDatosImu);
				break;
				default:
			}
		}
	}

	free (pbAuxDato);
	return(ERROK);
}
///////////////////////////////////////////////////////////////////////////////
