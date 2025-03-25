/*************************************************
 *
 *  enc.h
 * 
 *  v0.00 by Emmy Tsai
 * 
 *  Header for enc.c and enc600.c
 *
 ************************************************/ 
#ifndef _MSW_H
#define _MSW_H

#include "ixpio.h"
#include "_pisoenc600.h"

/* for register */
#define WR1 XR1
#define WR2 XR2
#define WR3 XR3
#define WR4 XR4
#define WR5 XR5
#define WR6 XR6
#define RD1 XR1
#define RD2 XR2
#define RD3 XR3
#define RD4 XR4
#define RD5 XR5
#define RD6 XR6
#define RD11 DI1
#define RD21 DI2
#define RD31 DI3
#define RD41 DI4
#define RD51 DI5
#define RD61 DI6

/* for tiger's register */
#define TJ_CNTL		0x00
#define TJ_AUXC		0x02
#define TJ_AUXD		0x03
#define TJ_MASK		0x05
#define TJ_AUX_STATUX	0x07
#define TJ_POLARITY	0x2a

#define CALLBACK
//#define EXPORTS
#define disable()
#define enable()

typedef struct
{
	unsigned int 	base;
	unsigned char	op;
	unsigned char	ctrl1;
	unsigned char	ctrl2;
	unsigned char	ctrl3;
	unsigned char	ctrl4;
	unsigned char	ctrl5;
	unsigned char 	ctrl6;
}cardtype;

extern cardtype card[20];	//0-19

void closeDev();


/* Macros to wrap DOS functions */
#define outportb(a, b)			(void)myoutb((unsigned int)(a), (unsigned char)(b))
#define outport(a, b)			outportb(a, b)	
#define inportb(a)			(unsigned int)myinb((unsigned int)(a))
/* FIXME - it will make some warning */
#define cprintf(x...)			printf(## x)
#define ENC6_REGISTRATION(a, b, c)	(int)ENC_INIT((a), (b), (c))
/* FIXME - gotoxy(), the fucntion of c++, not supported yet */
#define gotoxy(x, y)			(void)mymove(x, y) 
#define clrscr()			
#define delay(x)			usleep(x*1000)

#include "enc600.h"
#endif			/* _MSW_H*/
