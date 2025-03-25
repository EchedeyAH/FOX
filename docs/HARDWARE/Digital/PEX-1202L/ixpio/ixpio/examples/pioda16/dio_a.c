/* Example of Digital I/O for PIO-DA16/DA8/DA4 with with PIO/PISO Static Libary.

   Author: Reed Lai

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

   This example shows the digital output and input by ioctl digital
   commands.  You can connect the CON1 and CON2 with a 20-pin cable
   to examine the output.

   v 0.0.0  25 Oct 2006 by Reed Lai
     create */

#include <stdio.h>
#include "piodio.h"

int main()
{
  int fd;
  char *dev_file;

  dev_file = "/dev/ixpio1";

  fd = PIODA_Open(dev_file);

  if (fd < 0)
  {
    printf("Can't open device\n");
    return FAILURE;
  }

  if (PIODA_DriverInit(fd)) return FAILURE;

  WORD dout_data = 0x1;
  WORD din_data = 0;

  puts("Press <enter> for next, ESC to exit.");
 
  while (getchar() != 27)
  {
    if (PIODA_Digital_Output(fd, 1, dout_data)) return FAILURE;
    if (PIODA_Digital_Input(fd, 0, &din_data)) return FAILURE;

    printf("Digital Output: 0x%04x    Input: 0x%04x    <Enter> next, ESC exit ", dout_data, din_data);

    (dout_data == 0x8000) ? (dout_data = 1) : (dout_data <<= 1); 
  }

  PIODA_Close(fd);
  puts("\nEnd of program.");

  return SUCCESS;
}
