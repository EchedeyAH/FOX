/* Example of analog input for PISO-813 with PIO/PISO Dynamic Libary..

   Author: Golden Wang

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. */

/* File level history (record changes for this file here.)

   Analog input range -10V ~ +10V, giain x1

   JP1 : 20V

       +---+ 10V
       |   |
       | o |
       |+-+|
       ||o||
       || ||
       ||o||
       |+-+|
       +---+ 20V

   JP2 : Bipolar

       +---+ UNI
       |   |
       | o |
       |+-+|
       ||o||
       || ||
       ||o||
       |+-+|
       +---+ BI

   v 0.0.0  25 Nov 2009 by Golden Wang
     
     1.Before running the demo "aiso", user must set the environment variable of 
       ixpio dynamic library. Please refer to below command:

       #export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:../../lib	 


*/

#include <stdio.h>
#include "piodio.h"

int main()
{
  int fd;
  char *dev_file;
  float av;

  dev_file = "/dev/ixpio1";

  fd = PIODA_Open(dev_file);

  if (fd < 0)
  {
    printf("Can't open device\n");
    return FAILURE;
  }

  if (PIODA_DriverInit(fd)) return FAILURE;

  float ai_data;
  WORD channel = 0;
  WORD gain = 0;

  while (getchar() != 27)
  {
    PIODA_AnalogInputVoltage(fd, channel, gain, &ai_data);
    av = 20 * ai_data / 0xfff - 10;     //Default JP2(Bipolar),JP1(20V) 
    //av = 10 * (ai_data / 0xfff) - 0;  //JP2(Unipolar),JP1(10V)
    printf("ADC read = 0x%04x ==> %fV\t ESC exit\n", (WORD)ai_data, av);
  }

  PIODA_Close(fd);
  return SUCCESS;
}
