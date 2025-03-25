/* Example of counter and interrupt for PIO-D64.

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

   v 0.0.0 11 Jul 2003 by Reed Lai
     She did nothing else smile. */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <signal.h>

#include "ixpio.h"

/* custom signal id , 33-63 */
#define MY_SIG 33

void sig_handler(int sig)
{
	static unsigned sig_counter;
	printf("time beat %u, press <enter> to exit\r", ++sig_counter);
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

	/* set action for signal */
	act.sa_handler = sig_handler;
	sigemptyset(&act.sa_mask);
	sigaddset(&act.sa_mask, MY_SIG);
	sigaction(MY_SIG, &act, &act_old);

	/* config board interrupt */
	reg.id = IXPIO_IMCR;
	reg.value = 0x4;
		/* enable INT_CHAN_2 (TMRIRQ) as an interrupt source */

	if (ioctl(fd, IXPIO_WRITE_REG, &reg)) {
		close(fd);
		sigaction(MY_SIG, &act_old, NULL);
		puts("Failure of configuring interrupt.");
		return FAILURE;
	}

	/* condiction to receive signal */
	sig.sid = MY_SIG;
	sig.pid = getpid();
	sig.is = 0x4; /* INT_CHAN_2 */
	sig.edge = 0x4; /* signal for positive edge */
	sig.bedge = 0;

	if (ioctl(fd, IXPIO_SET_SIG, &sig)) {
		close(fd);
		sigaction(MY_SIG, &act_old, NULL);
		puts("Failure of signal condiction.");
		return FAILURE;
	}

	/***************************************************
	 * setup 8254 chip 2 (counter 4, 5) for INT_CHAN_2 *
	 *                                                 *
	 * 4MHz / (0x07d0 * 0x7d0) = 1Hz                    *
	 ***************************************************/
	reg.id = IXPIO_8254CW2;
	reg.value = 0x76;			/* counter-1, mode 3 */
	if (ioctl(fd, IXPIO_WRITE_REG, &reg)) {
		close(fd);
		sigaction(MY_SIG, &act_old, NULL);
		puts("Failure of 8254 counter-1 mode.");
		return FAILURE;
	}
	reg.id = IXPIO_8254C4;
	reg.value = 0xd0;
	if (ioctl(fd, IXPIO_WRITE_REG, &reg)) {
		close(fd);
		sigaction(MY_SIG, &act_old, NULL);
		puts("Failure of 8254 counter 1 LSB.");
		return FAILURE;
	}
	reg.value = 0x07;
	if (ioctl(fd, IXPIO_WRITE_REG, &reg)) {
		close(fd);
		sigaction(MY_SIG, &act_old, NULL);
		puts("Failure of 8254 counter 1 MSB.");
		return FAILURE;
	}
	reg.id = IXPIO_8254CW2;
	reg.value = 0xb6;			/* counter-2, mode 3 */
	if (ioctl(fd, IXPIO_WRITE_REG, &reg)) {
		close(fd);
		sigaction(MY_SIG, &act_old, NULL);
		puts("Failure of 8254 counter 2 mode.");
		return FAILURE;
	}
	reg.id = IXPIO_8254C5;
	reg.value = 0xd0;
	if (ioctl(fd, IXPIO_WRITE_REG, &reg)) {
		close(fd);
		sigaction(MY_SIG, &act_old, NULL);
		puts("Failure of 8254 counter 2 LSB.");
		return FAILURE;
	}
	reg.value = 0x07;
	if (ioctl(fd, IXPIO_WRITE_REG, &reg)) {
		close(fd);
		sigaction(MY_SIG, &act_old, NULL);
		puts("Failure of 8254 counter 2 MSB.");
		return FAILURE;
	}

	puts("Press <enter> to exit");
	while (getchar() != 10);

	close(fd);
	sigaction(MY_SIG, &act_old, NULL);
	puts("End of program.");
	return SUCCESS;
}
