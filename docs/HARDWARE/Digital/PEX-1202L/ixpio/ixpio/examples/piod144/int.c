/* Example of interrupt handling for PIO-D144. This program configures
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

   v 0.2.0  8 Aut 2003 by Reed Lai
     Custom signal.

   v 0.1.1  2 Dec 2002 by Reed Lai
     Improves signal operating.

   v 0.1.0 29 Apr 2002 by Reed Lai
     Rename PIO to IXPIO.

   v 0.0.0 5 Oct 2000 by Reed Lai
     create, blah blah... */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <signal.h>

#include "ixpio.h"

/* custom signal id, 33-63 */
#define MY_SIG 40

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

	struct sigaction act, act_old;

	dev_file = "/dev/ixpio1";

	/* open device file */
	fd = open(dev_file, O_RDWR);
	if (fd < 0) {
		printf("Failure of open device file \"%s.\"\n", dev_file);
		return FAILURE;
	}

	/* set action for signal */

	act.sa_handler = sig_handler;
	sigemptyset(&act.sa_mask);
	sigaddset(&act.sa_mask, MY_SIG);
	if (sigaction(MY_SIG, &act, &act_old)) {
		close(fd);
		puts("Failure of signal action.");
		return FAILURE;
	}

	/* configure ports */
	reg.id = IXPIO_AIOPCR;
	reg.value = 0x02;			/* port C/2 */
	if (ioctl(fd, IXPIO_WRITE_REG, &reg)) {
		close(fd);
		sigaction(MY_SIG, &act_old, NULL);
		puts("Failure of configuring ports.");
		return FAILURE;
	}

	/* configure board interrupt */
	reg.id = IXPIO_IMCR;
	reg.value = 0x1;  /* enable P2C0 */
	if (ioctl(fd, IXPIO_WRITE_REG, &reg)) {
		close(fd);
		sigaction(MY_SIG, &act_old, NULL);
		puts("Failure of configuring interrupt.");
		return FAILURE;
	}

	/* signal condiction */
	sig.sid = MY_SIG;
	sig.pid = getpid();
        printf("sig pid : %d\n",sig.pid);
	sig.is = 0x1;     /* signaling for the P2C0 channels */
	sig.edge = 1;  /* high level trigger */

	if (ioctl(fd, IXPIO_SIG, &sig)) {
		close(fd);
		sigaction(MY_SIG, &act_old, NULL);
		puts("Failure of signal condiction.");
		return FAILURE;
	}

	/* wait for exit */
	puts("press <enter> to exit\n");
	while (getchar() != 10);

	close(fd);
	sigaction(MY_SIG, &act_old, NULL);
	puts("\nEnd of program.");
	return SUCCESS;
}
