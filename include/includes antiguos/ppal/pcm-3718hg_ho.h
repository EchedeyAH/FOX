#ifndef PCM3718HG_HO_H_
#define PCM3718HG_HO_H_
	
	
	// Libreria donde se definen las constantes de la tarjeta PCM-3718HG
	
	// Registros:
	
	#define AD_LOW_BYTE				0
	#define CHANNEL					0
	#define SW_AD_TRIGGER			0
	#define AD_HIGH_BYTE			1
	#define AD_RANGE_CONTROL		1
	#define MUX_SCAN				2
	#define MUX_SCAN_CHANNEL		2
	#define RANGE_CONTROL_POINTER	2
	#define DIO_LOW_BYTE			3
	#define DA_OUTPUT_DATA_0		4
	#define DA_OUTPUT_DATA_1		5
	#define FIFO_INTERRUPT_CONTROL	6
	#define STATUS					8
	#define CLEAR_INTERRUPT_REQUEST	8
	#define CONTROL					9
	#define COUNTER_ENABLE			10
	#define DIO_HIGH_BYTE			11
	#define COUNTER_0				12
	#define COUNTER_1				13
	#define COUNTER_2				14
	#define COUNTER_CONTROL			15
	#define AD_DATA_CHANNELS_FIFO_W	17
	#define AD_DATA_CHANNELS_FIFO_R	18
	#define FIFO_STATUS				19
	#define FIFO_CLEAR				20
	
	// Rangos de las entradas analogicas:
	
	
	#define RANGO_BIPOLAR_5V		0x00	//hg y ho
	#define RANGO_BIPOLAR_0_5V		0x01
	#define RANGO_BIPOLAR_2_5V		0x01		//ho
	#define RANGO_BIPOLAR_0_05V		0x02
	#define RANGO_BIPOLAR_1_25V		0x02	//ho
	#define RANGO_BIPOLAR_0_005V	0x03
	#define RANGO_BIPOLAR_0_625V	0x03	//ho
	#define RANGO_UNIPOLAR_10V		0x04	//hg y ho
	#define RANGO_UNIPOLAR_1V		0x05
	#define RANGO_UNIPOLAR_5V		0x05	//ho
	#define RANGO_UNIPOLAR_0_1V		0x06
	#define RANGO_UNIPOLAR_2_5V		0x06	//ho
	#define RANGO_UNIPOLAR_0_01V	0x07
	#define RANGO_UNIPOLAR_1_25V	0x07	//ho
	#define RANGO_BIPOLAR_10V		0x08	//hg y ho
	#define RANGO_BIPOLAR_1V		0x09
	#define RANGO_BIPOLAR_0_1V		0x0A
	#define RANGO_BIPOLAR_0_01V		0x0B
	
	// Byte de estado:
	
	#define EOC 					(1<<7)
	#define MUX						(1<<5)
	#define INT 					(1<<4)
	#define CN3 					(1<<2)
	#define CN2 					(1<<1)
	#define CN1 					(1<<0)
	
	// Byte de control:
	
	#define INTE 					(1<<7)
	#define I2 						(1<<6)
	#define I1 						(1<<5)
	#define I0 						(1<<4)
	#define DMAE 					(1<<2)
	#define ST1 					(1<<1)
	#define ST0 					(1<<0)
	
	#define IRQ2					2
	#define IRQ3					3
	#define IRQ4					4
	#define IRQ5					5
	#define IRQ6					6
	#define IRQ7					7
	
	#define SW_TRIGGER				(ST0)
	#define EXT_TRIGGER				(ST1)
	#define PACER_TRIGGER			(ST1|ST0)
	
	
	// Pacer Enable
	
	#define PACER_ENABLED			0
	#define PACER_DISABLED			1
	
	// FIFO int. control
	
	#define CTRL_ENABLED	1
	#define CTRL_DISABLED	0
	
	// Pacer
	
	// Selec. cnt
	#define SELEC_COUNTER_0				0
	#define SELEC_COUNTER_1				(1<<6)
	#define SELEC_COUNTER_2				(1<<7)
	#define READ_BACK				((1<<6)|(1<<7))
	//Selec. operacion lect/esc.
	#define COUNTER_LATCH			0
	#define RW_LSB					(1<<4)
	#define RW_MSB					(1<<5)
	#define RW_LSB_MSB				((1<<4)|(1<<5))
	//Selec. modo operacion 
	#define SQUARE_WAVE_RATE_GEN	((1<<1)|(1<<2)) //No ponemos el resto de modos porque no se usaran

#endif /*PCM3718HG_HO_H_*/

