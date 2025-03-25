/* Example of counter for PIO-DA16/DA8/DA4 with PIO/PISO Static Libary.

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

   v 0.0.0  2 Nov 2006 by Reed Lai
     create */

#include "piodio.h"

/* custom signal id, 33-63 */
#define MY_SIG 34

int fd;
WORD dout_data = 1;
WORD din_data = 0;

void sig_handler(int sig)
{
  static unsigned int beat;
  PIODA_Digital_Output(fd, 0, dout_data);
  PIODA_Digital_Input(fd, 0, &din_data);

  printf("\rTime beat: %d    Digital Output: 0x%04x    Input: 0x%04x    <enter> exit ", ++beat, dout_data, din_data);

  if (dout_data == 0x8000) dout_data = 1;
  else dout_data <<= 1;
}

int main()
{
  char *dev_file = "/dev/ixpio1";

  /* open device file */
  fd = PIODA_Open(dev_file);

  if (fd < 0)
  {
    printf("Can't open device\n");
    return FAILURE;
  }

  if (PIODA_DriverInit(fd)) return FAILURE;

  if (PIODA_IntInstall(fd, sig_handler, MY_SIG, PIODA16_INT1, 0)) return FAILURE;

  /* press <enter> to get out */
  while (getchar() != 10);

  PIODA_IntRemove(fd, MY_SIG);
  PIODA_Close(fd);
  puts("End of program.");
  return SUCCESS;
}
