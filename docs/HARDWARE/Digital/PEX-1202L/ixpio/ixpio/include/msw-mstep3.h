/*************************************************
 *
 *  msw.h
 * 
 *  v0.00 by Emmy Tsai
 * 
 *  Header for msw.c and mstep3.c
 *
 ************************************************/ 
#ifndef _MSW_H
#define _MSW_H

#include "ixpio.h"
#include "_pisops300.h"

/* for register */
#define RDFF2	FIFO2
#define MSC	MR
#define WRFF1	FIFO1
#define RSTFF1	RR
#define CALLBACK
#define EXPORTS
#define disable()
#define enable()
#define PHASE1 1
#define sIDLE 0

typedef struct
{
	unsigned int 	base;
	unsigned char	exist;
	unsigned char	op;
	unsigned char	workNo;
	unsigned char	sync1;
	unsigned char	sync2;

	unsigned char 	sys;
	unsigned char 	limit;
	unsigned char 	p1;
	unsigned char 	InWorkNo;
	unsigned char 	X_state;
	unsigned char 	Y_state;
	unsigned char 	Z_state;
	long	 	command_X;
	long 		position_X;
	long 		command_Y;
	long		position_Y;
	long 		command_Z;
	long		position_Z;
}cardtype;

extern cardtype card[16];

typedef struct
{
	unsigned char 	command;
	unsigned char 	cardNo;
	unsigned char	plane;
	long		x;
	long 		y;
	long 		z;
	long 		R;
	unsigned char	dir;
	unsigned int 	speed;
	unsigned char	acc_mode;
}system_type;

extern system_type sys;
extern unsigned char MSTEP3_EXIST(unsigned char);
extern unsigned char WorkFlag;
extern unsigned char state;
void closeDev();


/* Macros to wrap DOS functions */
#define outportb(a, b)			(void)myoutb((unsigned int)(a), (unsigned char)(b))
#define outport(a, b)			outportb(a, b)	
#define inportb(a)			(unsigned int)myinb((unsigned int)(a))
/* FIXME - it will make some warning */
#define cprintf(x...)			printf(## x)
#define MSTEP3_REGISTRATION(a, b)	(int)MSTEP3_INIT((a), (b))
/* FIXME - gotoxy(), the fucntion of c++, not supported yet */
#define gotoxy(x, y)			(void)mymove(x, y) 
#define clrscr()			
#define delay(x)			usleep(x*1000)

#include "mstep3.h"
#endif			/* _MSW_H*/
