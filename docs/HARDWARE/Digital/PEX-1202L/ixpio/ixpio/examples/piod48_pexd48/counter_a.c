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

void sig_handler(int sig)
{
	static unsigned sig_counter;
	printf("time beat %u, press <enter> to exit\n", ++sig_counter);
}

int main()
{
	int fd;
        char *dev_file;
        ixpio_reg_t reg;
        ixpio_signal_t sig;
        int Counter1, Counter2;
	unsigned int HZ;
	int clk, r;
        static struct sigaction act, act_old;
	WORD activemode, c1hb, c1lb, c2hb, c2lb;

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

	clk = 1;			//0 -> 2MHz, 1 -> 32.768KHz
	PIOD48_8254Clk(fd, clk);
	if(clk)
		HZ = 32768;
	else
		HZ = 2000000;
	
	activemode = 1;		//0 negative edges, 1 positive edges, 2 both edges
	if (PIODA_IntInstall(fd, sig_handler, SIGALRM, PIOD48_INT3, activemode)) return FAILURE;
	

	c1hb = 0x40;
	c1lb = 0x0;
	Counter1 = (c1hb << 8 | c1lb);
	PIODA_8254CW(fd, 0, 3, 3, 1);		//bcd, mode, rl, sc
	if(PIODA_8254C1(fd, c1hb, c1lb))	//hbyte, lbyte,	Notice Counter1 can't be 1
		return FAILURE;

	c2hb = 0x0;
	c2lb = 0x2;
	Counter2 = (c2hb << 8 | c2lb);
	PIODA_8254CW(fd, 0, 3, 3, 2);
	if(PIODA_8254C2(fd, c2hb, c2lb))	//hbyte, lbyte, Notice Counter2 can't be 1
		return FAILURE;

	/* Count counter HZ */
	HZ = (HZ/Counter1)/Counter2;
	printf("INT_CHAN_3 is %d HZ\n",HZ);

	// wait for exit
	puts("press <enter> to exit\n");
	while (getchar() != 10);

	PIODA_IntRemove(fd, SIGALRM);
	PIODA_Close(fd);
	puts("\nEnd of program.");

	return SUCCESS;
}
