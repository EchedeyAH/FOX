/* Example of interrupt handling for PIO/PEX-D48. This program configures
   driver to send signals in same signal id for the four interrupt
   channels.

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

   v 0.0.0 28 Mar 2019 by Winson Chen
     Test interrupt by static lib */

#include "piodio.h"

/* custom signal id, 33-63 */
#define MY_SIG 34

#define DO_enable 0
#define DI_enable 1


void sig_handler(int sig)
{
	static unsigned sig_counter;
	printf("\rGot single %d for %u times ", sig, ++sig_counter);
}

int main()
{
	ixpio_reg_t reg;
	ixpio_signal_t sig;

	int fd, r;
	char *dev_file;
	WORD Int, opt, activemode;

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

	/* Set PC1-L as DI for INT0 */
	r = PIODA_PortDirCfs(fd, PIOD48_P2_LB, DI_enable);
        if(r)
        {
                printf("Port defined error\n");
                return FAILURE;
        }

	Int = 0;	//INT_CHAN_0
	opt = 0;	//0x0 PC3&!PC7, 0x1 disable Int, 0x2 PC3
	PIOD48_INT01_CONTROL(fd, Int, opt);

	activemode = 1;		//0 negative edges, 1 positive edges, 2 both edges
	// use libary interrupt function to set interrupt parameter
	if (PIODA_IntInstall(fd, sig_handler, MY_SIG, PIOD48_INT0, activemode)) return FAILURE;

	printf("Enable INT_CHAN0, external trigger by port2 PC3, signaling for both edge.\n");

	// wait for exit
	puts("press <enter> to exit\n");
	while (getchar() != 10);

	PIODA_IntRemove(fd, MY_SIG);
	PIODA_Close(fd);
	puts("\nEnd of program.");

	return SUCCESS;
}
