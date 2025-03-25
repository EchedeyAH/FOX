//--------------------------------------------------------
// DEMO1.cpp                                    12/11/99
//
// Function:
//    hand wheel(encoder) input from Z axis,
//    then move X axis
//--------------------------------------------------------
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
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
char GetPosition;

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
char ch;
  disable();
  clrscr();
  //------set card 1 parameters-------------------------------------
  card1.address      = 8;     //0~15
  card1.DDA          = 10;
  card1.AD           = 5;
  card1.LSP          = 5;
  card1.HSP          = 150;
  card1.home_speed   = 10;
  card1.search_speed = 2;
  card1.arc_speed    = 50;

  card1.x_mode       = DDA_CW_CCW|DDA_EN|DDA_OE|ENC_AB_PHASE|ENC_EXTERNAL;
  card1.x_dir        = NORMAL_DIR;
  card1.x_son        = ON;
  card1.y_mode       = DDA_CW_CCW|DDA_EN|DDA_OE|ENC_AB_PHASE|ENC_EXTERNAL;
  card1.y_dir        = NORMAL_DIR;
  card1.y_son        = OFF;
  card1.z_mode       = DDA_CW_CCW|DDA_EN|DDA_OE|ENC_AB_PHASE|ENC_EXTERNAL;
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
  GetPosition=0;

  do {} while (GetPosition=0);
  new_z=card1.ZP;
  old_z=new_z;
  puts("Enter to continue, EXC to exit.\n");
  do{
     card1.msc=MSTEP3_MSC(CARD1);       //get msc register
     if ((card1.msc & 0x01)== 0x00)     //check FIFO1 is empty or not!
     {
       new_z=card1.ZP;
       //output the difference of Z-axis to X-axis
       MSTEP3_INTP_PULSE(CARD1,new_z-old_z,0,0);
       old_z=new_z;
     }
     gotoxy(1,10);
     printf("HandWheel Z=%10ld, X output=%10ld", new_z, card1.XC);
  } while (getchar() != 27);		
  MSTEP3_RESET_SYSTEM(CARD1);
  closeDev();	// close device file
  puts("\n");
}
