//--------------------------------------------------------
// DEMO2.cpp                                    2/24/2001
//                   
// CPU version 2.0
//
// Function:
//    test WAIT_X_STOP(),WAIT_IS_X_STOP()
//    round robin test
//--------------------------------------------------------
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

//#include "mstep3.h"
#include "msw-mstep3.h"

//----- define --------------------------------------------
#define CARD1 0

//----- structure -----------------------------------------
typedef struct
  {
	 //---- parameter ---------
	 unsigned int  address;
	 unsigned char exist;
	 unsigned char DDA;
	 unsigned char AD;
	 unsigned int  LSP;
	 unsigned int  HSP;
	 unsigned char home_speed;
	 unsigned char search_speed;
	 unsigned int  arc_speed;

	 unsigned char x_mode;
	 unsigned char x_dir;
	 unsigned char x_son;
	 unsigned char y_mode;
	 unsigned char y_dir;
	 unsigned char y_son;
	 unsigned char z_mode;
	 unsigned char z_dir;
	 unsigned char z_son;

	 //---- information -------
	 unsigned char op;
	 unsigned char ip;
	 unsigned char msc;
	 unsigned char ls;
	 unsigned char p1;
	 unsigned char x_state;
	 unsigned char y_state;
	 unsigned char z_state;
	 long          XC;
	 long          XP;
	 long          YC;
	 long          YP;
	 long          ZC;
	 long          ZP;
  }CardParameter;

//----- variable ------------------------------------------
CardParameter card1;
long new_z,old_z;
int  wait_count=0;
//--------------------------------------------------------------------
// set the operation parameter of PISO-PS300
//--------------------------------------------------------------------
void set_parameter()
{
  MSTEP3_SET_NC(CARD1,NO);
  MSTEP3_SET_CONTROL_MODE(CARD1, card1.x_mode, card1.y_mode, card1.z_mode);
  MSTEP3_SET_VAR(CARD1, card1.DDA, card1.AD, card1.LSP,
		 card1.HSP, card1.arc_speed);
  MSTEP3_SET_DEFDIR(CARD1, card1.x_dir, card1.y_dir, card1.z_dir);
  MSTEP3_SET_SERVO_ON(CARD1, card1.x_son, card1.y_son, card1.z_son);
}
//####################################################################
int main()
{
int j,error,error_count;
  disable();
  clrscr();

  //------set card 1 parameters-------------------------------------
  card1.address      = 8;     //0~15
  card1.DDA          = 1;
  card1.AD           = 50;
  card1.LSP          = 50;
  card1.HSP          = 2040;
  card1.home_speed   = 50;
  card1.search_speed = 5;
  card1.arc_speed    = 500;

  card1.x_mode       = DDA_CW_CCW|DDA_EN|ENC_CW_CCW|ENC_INTERNAL;
  card1.x_dir        = NORMAL_DIR;
  card1.x_son        = OFF;
  card1.y_mode       = DDA_CW_CCW|DDA_EN|ENC_CW_CCW|ENC_INTERNAL;
  card1.y_dir        = NORMAL_DIR;
  card1.y_son        = OFF;
  card1.z_mode       = DDA_CW_CCW|DDA_EN|ENC_CW_CCW|ENC_INTERNAL;
  card1.z_dir        = NORMAL_DIR;
  card1.z_son        = OFF;

  //---- check PISO-PS300/S300 is exist or not ------------------------
  card1.exist=MSTEP3_REGISTRATION(CARD1, card1.address);
  if (card1.exist!=YES)
  {
     printf("There is not exist any PISO-PS300/S300 card !\n");
     return;
  }
  MSTEP3_RESET_SYSTEM(CARD1);
  clrscr();

  //-------------------------------------------------------------------
  set_parameter();
  enable();
  printf("*** PISO-PS300 ROUND-ROBIN test ***\r\n");

  //----motion command------------------------
  error=0;
  error_count=0;
  printf("\r\n");

  printf("MSTEP3_INTP_LINE02\r\n");
  for (j=0; j<400; j++)
  {
     //------------------------------------------------
     MSTEP3_INTP_LINE02(CARD1,XY_plane, 20000,-20000, 2040, 0);
     do
     { printf("%d full\r\n",j);
     } while (MSTEP3_INTP_STOP()!=READY);
     do {} while (MSTEP3_IS_X_STOP(CARD1)==NO);
     //---------------------------------------------
     wait_count=2;
     while (wait_count) {}; //wait for correct position input in ISR
     if ( (card1.XP!=20000) || (card1.YP!=-20000))
     {
	error=1;
	error_count++;
	printf("error x=%ld, y=%ld N=%d\r\n",card1.XP,card1.YP,j);
     };
     MSTEP3_PRESET_POSITION(CARD1,X_axis,0);
     MSTEP3_PRESET_POSITION(CARD1,Y_axis,0);
  }

  printf("MSTEP3_INTP_ARC02\r\n");
  for (j=0; j<400; j++)
  {
     //------------------------------------------------
     MSTEP3_INTP_ARC02(CARD1,XY_plane,20000,-20000, 100000, CW, 2040,0);
     do {} while (MSTEP3_INTP_STOP()!=READY);
     do {} while (MSTEP3_IS_X_STOP(CARD1)==NO);
     //---------------------------------------------
     wait_count=2;
     while (wait_count) {}; //wait for correct position input in ISR
     if ( (card1.XP!=20000) || (card1.YP!=-20000))
     {
	error=1;
	error_count++;
	printf("error x=%ld, y=%ld N=%d\r\n",card1.XP,card1.YP,j);
     };
     MSTEP3_PRESET_POSITION(CARD1,X_axis,0);
     MSTEP3_PRESET_POSITION(CARD1,Y_axis,0);
  }

  printf("MSTEP3_INTP_CIRCLE02\r\n");
  for (j=0; j<400; j++)
  {
     //------------------------------------------------
     MSTEP3_INTP_CIRCLE02(CARD1,XY_plane,10000,-10000, CW, 2040,0);
     do {} while (MSTEP3_INTP_STOP()!=READY);
     do {} while (MSTEP3_IS_X_STOP(CARD1)==NO);
     //---------------------------------------------
     wait_count=2;
     while (wait_count) {}; //wait for correct position input in ISR
     if ( (card1.XP!=0) || (card1.YP!=0))
     {
	error=1;
	error_count++;
	printf("error x=%ld, y=%ld N=%d\r\n",card1.XP,card1.YP,j);
     };
     MSTEP3_PRESET_POSITION(CARD1,X_axis,0);
     MSTEP3_PRESET_POSITION(CARD1,Y_axis,0);
  }

  MSTEP3_RESET_SYSTEM(CARD1);
  closeDev();
}
