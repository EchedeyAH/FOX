/* Example of interrupt handle for PISO-730.

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

   v 0.1.0 12 Aug 2003 by Reed Lai
     Custom signal.

   v 0.0.1  2 Dec 2002 by Reed Lai
     Improve signal operating.

   v 0.0.0 29 Apr 2002 by Reed Lai
     Create. */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <signal.h>

#include "ixpio.h"

/* custom signal, 33-63 */
#define MY_SIG 60

void sig_handler(int sig)
{
	static unsigned sig_counter;
	printf("\rGot single %d for %u times ", sig, ++sig_counter);
}

int main()
{
	int fd;
	char *dev_file;
	ixpio_reg_t reg;
	ixpio_signal_t sig;
	ixpio_reg_t id_o, id_i, d_o, d_i;

	static struct sigaction act, act_old;

	dev_file = "/dev/ixpio1";

	/* open device file */
	fd = open(dev_file, O_RDWR);
	if (fd < 0) {
		printf("Failure of open device file \"%s.\"\n", dev_file);
		return FAILURE;
	}

	/* signal action */
	act.sa_handler = sig_handler;
	sigemptyset(&act.sa_mask);
	sigaddset(&act.sa_mask, MY_SIG);
	if (sigaction(MY_SIG, &act, &act_old)) {
		close(fd);
		puts("Failure of signal action.");
		return FAILURE;
	}

	/* configure interrupt */
	reg.id = IXPIO_IMCR;
	reg.value = 3;  /* DI 0/1 */
	if (ioctl(fd, IXPIO_WRITE_REG, &reg)) {
		close(fd);
		sigaction(MY_SIG, &act_old, NULL);
		puts("Failure of configuring interrupt.");
		return FAILURE;
	}

	/* signal condiction */
	sig.sid = MY_SIG;
	sig.pid = getpid();
	sig.is = 1;				/* DI-0 */
	sig.bedge = 1;			/* signal for both edges */
	if (ioctl(fd, IXPIO_SET_SIG, &sig)) {
		close(fd);
		sigaction(MY_SIG, &act_old, NULL);
		puts("Failure of signal condiction.");
		return FAILURE;
	}

        d_o.id  = d_i.id = IXPIO_DIO;

        puts("Enter to continue, ESC to exit.");
        while (getchar() != 27) 
	{
        	d_o.value = 0x0;
                if (ioctl(fd, IXPIO_WRITE_REG, &d_o)) {
                        puts("Failure of IXPIO_WRITE_REG: DIO.");
                        break;
                }
		printf("d_o.value is 0x%x\n",d_o.value);

		if (ioctl(fd, IXPIO_READ_REG, &d_i)) 
		{
                        puts("Failure of IXPIO_READ_REG: DIO.");
                        break;
                }
                printf("DI[0:15]=0x%04x\n",d_i.value);

		usleep(800);

		d_o.value = 0xffff;

                if (ioctl(fd, IXPIO_WRITE_REG, &d_o)) {
                        puts("Failure of IXPIO_WRITE_REG: DIO.");
                        break;
		}
		printf("d_o.value is 0x%x\n",d_o.value);

		if (ioctl(fd, IXPIO_READ_REG, &d_i)) 
		{
                        puts("Failure of IXPIO_READ_REG: DIO.");
                        break;
                }
                printf("DI[0:15]=0x%04x\n",d_i.value);

        }


/*
	puts("press <enter> to exit\n");
	while (getchar() != 10);
*/
	close(fd);
	sigaction(MY_SIG, &act_old, NULL);
	puts("\nEnd of program.");
	return SUCCESS;
}
