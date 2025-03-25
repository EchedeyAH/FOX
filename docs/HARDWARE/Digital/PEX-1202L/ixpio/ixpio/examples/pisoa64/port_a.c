/* Example of I/O port for PISO-A64 with with PIO/PISO Static Libary.

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


   v 0.0.0  27 Oct 2006 by Reed Lai
     Create. */
#include <stdio.h>
#include "piodio.h"

int main()
{
   int fd;
   char *dev_file;

   dev_file = "/dev/ixpio1";

   /* Open device file */
   fd = PIODA_Open(dev_file);

   if (fd < 0)
   {
     printf("Can't open device\n");
     return FAILURE;
   }

   if (PIODA_DriverInit(fd)) return FAILURE;

   WORD do_data = 0;

   puts("Enter to continue, ESC to exit");
   while(getchar()!=27)
   {
     if (PIODA_Digital_Output(fd, PISOA64_DOB, do_data)) return FAILURE;
     printf("DO_A=0x%02x ", do_data);
     ++do_data;
   }

   PIODA_Close(fd);
   return SUCCESS;
}
