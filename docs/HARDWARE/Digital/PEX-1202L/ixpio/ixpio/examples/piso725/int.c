/* Example of interrupt handle for PISO-725.

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
     Improves signal operating.

   v 0.0.0  4 Jun 2002 by Reed Lai
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
#define MY_SIG 33

void sig_handler(int sig)
{
	static unsigned sig_counter;
	printf("\rGot single %d for %u times\n ", sig, ++sig_counter);
}

int main()
{
	int fd;
	char *dev_file;
	ixpio_reg_t reg;
	ixpio_signal_t sig;
	ixpio_reg_t d_o, d_i;
	int index;

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
	reg.value = 0xff;  /* all channels */
	if (ioctl(fd, IXPIO_WRITE_REG, &reg)) {
		close(fd);
		sigaction(MY_SIG, &act_old, NULL);
		puts("Failure of configuring interrupt.");
		return FAILURE;
	}

	/* signal condiction */
	sig.sid = MY_SIG;
	sig.pid = getpid();
	sig.is = 0x02;				/* DI-0 */
	sig.bedge = 0x02;			/* signal for both edges */
	if (ioctl(fd, IXPIO_SET_SIG, &sig)) {
	//if (ioctl(11, IXPIO_SIG_ADD)) {

		close(fd);
		sigaction(MY_SIG, &act_old, NULL);
		puts("Failure of signal condiction.");
		return FAILURE;
	}

	/* wait for exit */
	d_o.id  = IXPIO_DO;
        d_i.id  = IXPIO_DI;
        d_o.value = 0;
	index = 10;

        puts("Enter to continue, ESC to exit.");
        while (index)
	{
	  ioctl(fd, IXPIO_WRITE_REG, &d_o);
	  sleep(1);
	  --d_o.value;			
	  index--;
          sleep(1);
        }

	close(fd);
	sigaction(MY_SIG, &act_old, NULL);
	puts("\nEnd of program.");
	return SUCCESS;
}
