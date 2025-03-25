/* Example of interrupt handling for PIO-D48. This program configures
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

   v 0.2.0 11 Aug 2003 by Reed Lai
     Custom signal.

   v 0.1.1  2 Dec 2002 by Reed Lai
     Improves signal operating.

   v 0.1.0 29 Apr 2002 by Reed Lai
     Renames PIO to IXPIO.

   v 0.0.0 30 Nov 2000 by Reed Lai
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
#define MY_SIG 34

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

	/* configure port */
	reg.id = IXPIO_82551CW;	//0xcc
	reg.value = 0x99;			/* PA and PC as input port */
	if (ioctl(fd, IXPIO_WRITE_REG, &reg)) {
		close(fd);
		sigaction(MY_SIG, &act_old, NULL);
		puts("Failure of configuring port.");
		return FAILURE;
	}

	/* configure board interrupt */
	reg.id = IXPIO_IMCR;	//0x05
	reg.value = 0xf;
		/* enable INT_CHAN_0/1/2/3 for interrupt */

	if (ioctl(fd, IXPIO_WRITE_REG, &reg)) {
		close(fd);
		sigaction(MY_SIG, &act_old, NULL);
		puts("Failure of configuring interrupt.");
		return FAILURE;
	}

	/* INT_CHAN_0 = PC3 of port 2 INT_CHAN_1 = * PC3&!PC& of port 2 */
	reg.id = IXPIO_CICR;	//0xf0
	reg.value = 0x8;

	if (ioctl(fd, IXPIO_WRITE_REG, &reg)) {
		close(fd);
		sigaction(MY_SIG, &act_old, NULL);
		puts("Failure of configuring interrupt.");
		return FAILURE;
	}

	/* signal condiction */
	sig.sid = MY_SIG;
	sig.pid = getpid();
	sig.is = 0xf;     /* signaling for the four channels */
	sig.bedge = 0x1;  /* signaling for both edges */

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
