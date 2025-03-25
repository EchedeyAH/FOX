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

   v 0.0.0 5 Feb 2024 by Winson Chen
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
	int fd;
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
	if (PIODA_IntInstall(fd, sig_handler, MY_SIG, PISO730_ALL_INT, 1)) return FAILURE;

	puts("press <enter> to exit\n");
        	while (getchar() != 10);

	/* remove interrupt by using libary interrupt function */
	PIODA_IntRemove(fd, MY_SIG);

	/* close device */
	PIODA_Close(fd);

	return 0;
}
