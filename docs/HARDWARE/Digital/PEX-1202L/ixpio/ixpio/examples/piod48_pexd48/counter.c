/* Example of counter for PIO/PEX-D48.

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
   v 0.1.2 21 Mar 2019 by Winson 
     Appear INT_CHAN_3 counter frequency.

   v 0.1.1  9 Dec 2002 by Reed Lai
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
	unsigned int HZ;
	int Counter1, Counter2;

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
	sigaddset(&act.sa_mask, SIGALRM);
	sigaction(SIGALRM, &act, &act_old);

	/* choose clock */
	reg.id = IXPIO_CICR;	//0xf0
	reg.value = 0x15;			/* 32768Hz */
	if (reg.value & 0x1)
		HZ = 32768;
	else
		HZ = 2048000;

	if (ioctl(fd, IXPIO_WRITE_REG, &reg)) {
		close(fd);
		sigaction(SIGALRM, &act_old, NULL);
		puts("Failure of choosing clock.");
		return FAILURE;
	}

	/* config board interrupt */
	reg.id = IXPIO_IMCR;
	reg.value = 0x8;
		/* enable INT_CHAN_3 for interrupt */

	if (ioctl(fd, IXPIO_WRITE_REG, &reg)) {
		close(fd);
		sigaction(SIGALRM, &act_old, NULL);
		puts("Failure of configuring interrupt.");
		return FAILURE;
	}

	/* set signal condiction for irq */
	sig.sid = SIGALRM;
	sig.pid = getpid();
	sig.is = 0x8;
		/* INT_CHAN_3 */
	sig.edge = 0x8;
		/* signal for negative edge */
	sig.bedge = 0;
		/* don't signal for both edges */

	if (ioctl(fd, IXPIO_SIG, &sig)) {
		close(fd);
		sigaction(SIGALRM, &act_old, NULL);
		puts("Failure of signal condiction.");
		return FAILURE;
	}

	/*****************************************
	 * setup 8254 counter-1/2 for INT_CHAN_3 *
	 *                                       *
	 * 32768Hz / (0x4000 * 2) = 1Hz          *
	 *****************************************/
	reg.id = IXPIO_8254CW;
	reg.value = 0x76;			/* counter-1, mode 3 */
	if (ioctl(fd, IXPIO_WRITE_REG, &reg)) {
		close(fd);
		sigaction(SIGALRM, &act_old, NULL);
		puts("Failure of 8254 counter-1 mode.");
		return FAILURE;
	}
	reg.id = IXPIO_8254C1;			/* Notice Counter1 can't be 1 */
	reg.value = 0x00;			/* Set 8254 counter 1 LSB */
	Counter1 = reg.value;
	if (ioctl(fd, IXPIO_WRITE_REG, &reg)) {
		close(fd);
		sigaction(SIGALRM, &act_old, NULL);
		puts("Failure of 8254 counter 1 LSB.");
		return FAILURE;
	}
	reg.value = 0x40;			/* Set 8254 counter 1 MSB */
	Counter1 = (reg.value << 8 | Counter1);
	if (ioctl(fd, IXPIO_WRITE_REG, &reg)) {
		close(fd);
		sigaction(SIGALRM, &act_old, NULL);
		puts("Failure of 8254 counter 1 MSB.");
		return FAILURE;
	}
	reg.id = IXPIO_8254CW;
	reg.value = 0xb6;			/* counter-2, mode 3 */
	if (ioctl(fd, IXPIO_WRITE_REG, &reg)) {
		close(fd);
		sigaction(SIGALRM, &act_old, NULL);
		puts("Failure of 8254 counter 2 mode.");
		return FAILURE;
	}
	reg.id = IXPIO_8254C2;			/* Notice Counter2 can't be 1 */
	reg.value = 0x2;			/* Set 8254 counter 2 LSB */
	Counter2 = reg.value;
	if (ioctl(fd, IXPIO_WRITE_REG, &reg)) {
		close(fd);
		sigaction(SIGALRM, &act_old, NULL);
		puts("Failure of 8254 counter 2 LSB.");
		return FAILURE;
	}
	reg.value = 0x0;			/* Set 8254 counter 2 MSB */
	Counter2 = (reg.value << 8 | Counter2);
	if (ioctl(fd, IXPIO_WRITE_REG, &reg)) {
		close(fd);
		sigaction(SIGALRM, &act_old, NULL);
		puts("Failure of 8254 counter 2 MSB.");
		return FAILURE;
	}

	/* Count counter HZ */
	HZ = (HZ/Counter1)/Counter2;
	printf("INT_CHAN_3 is %d HZ\n",HZ);


	/* press <enter> to get out */
	puts("Press <enter> to exit");
	while (getchar() != 10);

	close(fd);
	sigaction(SIGALRM, &act_old, NULL);
	puts("End of program.");
	return SUCCESS;
}
