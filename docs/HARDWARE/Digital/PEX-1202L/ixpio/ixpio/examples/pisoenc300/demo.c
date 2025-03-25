//-----------------------------------------------------------
// demo.cpp                                      Chao Chi-Mou
//
// compiled under large mode, Turbo C++
//
// v1.0   3/23/2001
// v2.0   1/1/2002
//-----------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>

#include "msw-enc600.h"

#define CARD1 0
//-----------------------------------------------------------------------

int main()
{
long  x1_value;
long  x2_value;
long  x3_value;
unsigned char x1_index;
unsigned char x2_index;
unsigned char x3_index;
int  dev_file = 1;

  clrscr();
  if (ENC6_REGISTRATION(CARD1, 0, dev_file)!=SUCCESS)
  {
     printf("PISO_ENC600 doesn't exist!\r\n");
     return 0;
  }

  ENC6_INIT_CARD(CARD1, ENC_CW_CCW  //ENC_QUADRANT
		      , ENC_CW_CCW
		      , ENC_CW_CCW
		      , ENC_CW_CCW
		      , ENC_CW_CCW
		      , ENC_CW_CCW);

  ENC6_RESET_ENCODER(CARD1,X1_axis);
  ENC6_RESET_ENCODER(CARD1,X2_axis);
  ENC6_RESET_ENCODER(CARD1,X3_axis);

  gotoxy(15,19);
  printf("Press <enter> to next, <Esc> to exit\n");
  do
  {

      x1_value = ENC6_GET_ENCODER(CARD1,X1_axis);
      x2_value = ENC6_GET_ENCODER(CARD1,X2_axis);
      x3_value = ENC6_GET_ENCODER(CARD1,X3_axis);

      x1_index = ENC6_GET_INDEX(CARD1,X1_axis);
      x2_index = ENC6_GET_INDEX(CARD1,X2_axis);
      x3_index = ENC6_GET_INDEX(CARD1,X3_axis);

      gotoxy(15,9);
      printf("  X1 : %12ld   HR,INDEX:%x\n", x1_value, x1_index);
      gotoxy(15,10);
      printf("  X2 : %12ld   HR,INDEX:%x\n", x2_value, x2_index);
      gotoxy(15,11);
      printf("  X3 : %12ld   HR,INDEX:%x\n", x3_value, x3_index);
      delay(100);
  } while (getchar()!=27);	
  closeDev();

  return 0;	
}

