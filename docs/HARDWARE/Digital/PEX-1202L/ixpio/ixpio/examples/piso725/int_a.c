/* Example of interrupt handle for PISO-725 with PIO/PISO Static Libary.

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

   This example enable the P2C0 which is the port 2 bit 0 at CN1 pin 29
   as an interrupt source.

   v 0.0.0 6 Jul 2007 by Golden Wang
     create, */

#include "piodio.h"
/* custom signal id, 33-63 */
#define MY_SIG 40

void sig_handler(int sig)
{
	static unsigned sig_counter;
	printf("\rGot single %d for %u times \n", sig, ++sig_counter);
}

int main()
{
  int fd, index, do_data;
  char *dev_file;

  dev_file = "/dev/ixpio1";

  /* open device file */
  fd = PIODA_Open(dev_file);

  if (fd < 0)
  {
    printf("Can't open device\n");
    return FAILURE;
  }

  /* init device */
  if (PIODA_DriverInit(fd)) return FAILURE;

  /* use libary interrupt function to set interrupt argument */
  if (PIODA_IntInstall(fd, sig_handler, MY_SIG, PISO725_ALL_INT, 0)) return FAILURE;

  index = 8;
  do_data = 1;

  while (index)
  {
    printf("DI value : 0x%x\n", do_data);

    /* output data from port1 by using libary DO function */
    if (PIODA_Digital_Output(fd, 1, do_data)) return FAILURE;

    if (do_data == 8)
    {
      if ( index == 5 )
      {
        printf("\n======== run again ========\n");
      }
      else if ( index == 1 )
      {
        printf("\n========    end    ========\n");
      }

      do_data = 1;
    }
    else
    {
      do_data <<= 1;
    }

    index--;

    sleep(1);
  }

  /* remove interrupt by using libary interrupt function */
  PIODA_IntRemove(fd, MY_SIG);

  /* close device */
  PIODA_Close(fd);

  return 0;
}
