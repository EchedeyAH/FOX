/* Example of interrupt handling for PIO-D64. This program is similar
   to the int2.c but uses two handlers.

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

   v 0.0.0 12 Aug 2003 by Reed Lai
     I do this for Aiur... */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <signal.h>

#include "ixpio.h"

/* custom signal id , 33-63 */
#define MY_SIG_TMRIRQ 33
#define MY_SIG_EXTIRQ 34

struct sigaction act_tmrirq_old, act_extirq_old;
unsigned int sig_tmrirq, sig_extirq, unknown;

void sig_tmrirq_handler(int sig)
{
	if (sig == MY_SIG_TMRIRQ) ++sig_tmrirq;
	else ++unknown;
	printf("\rGot signal for TMRIRQ:%u    EXTIRQ:%u   Unknown:%u ", sig_tmrirq, sig_extirq, unknown);
}

void sig_extirq_handler(int sig)
{
	if (sig == MY_SIG_EXTIRQ) ++sig_extirq;
	else ++unknown;
	printf("\rGot signal for TMRIRQ:%u    EXTIRQ:%u   Unknown:%u ", sig_tmrirq, sig_extirq, unknown);
}

void restore_old_sig_act()
{
	sigaction(MY_SIG_TMRIRQ, &act_tmrirq_old, NULL);
	sigaction(MY_SIG_EXTIRQ, &act_extirq_old, NULL);
}

int configure_sig_act()
{
	struct sigaction act;

	/* use a handler for all signals. */
	sigemptyset(&act.sa_mask);

	act.sa_handler = sig_tmrirq_handler;
	//sigaddset(&act.sa_mask, MY_SIG_TMRIRQ);
	if (sigaction(MY_SIG_TMRIRQ, &act, &act_tmrirq_old))
		return FAILURE;

	act.sa_handler = sig_extirq_handler;
	//sigaddset(&act.sa_mask, MY_SIG_EXTIRQ);
	if (sigaction(MY_SIG_EXTIRQ, &act, &act_extirq_old)) {
		sigaction(MY_SIG_TMRIRQ, &act_tmrirq_old, NULL);
		return FAILURE;
	}

	return SUCCESS;
}

int signal_condition(int fd)
{
	ixpio_signal_t sig;

	sig.pid = getpid();

	sig.sid = MY_SIG_EXTIRQ;
	sig.is = 0x1;     /* signaling for EXTIRQ */
	sig.edge = 0x1;  /* signaling for positive edges */
	sig.bedge = 0;  /* up to sig.edge */
	if (ioctl(fd, IXPIO_SIG, &sig))
		return FAILURE;

	sig.sid = MY_SIG_TMRIRQ;
	sig.is = 0x4;        /* signaling for TMRIRQ */
	sig.edge = 0x4;  /* signaling for positive edges */
	// sig.bedge = 0;  /* up to sig.edge */
	if (ioctl(fd, IXPIO_SIG_ADD, &sig))
		return FAILURE;

	return SUCCESS;
}

int main()
{
	int fd;
	char *dev_file;
	ixpio_reg_t reg;
	ixpio_signal_t sig;

	static struct sigaction act, act_old;

	dev_file = "/dev/ixpio1";

	fd = open(dev_file, O_RDWR);
	if (fd < 0) {
		printf("Failure of open device file \"%s.\"\n", dev_file);
		return FAILURE;
	}

	if (configure_sig_act()) {
		close(fd);
		puts("Failure of signal action.");
	}

	reg.id = IXPIO_IMCR;
	reg.value = 0x5;
		/* enable INT_CHAN_2 (TMRIRQ) and INT_CHAN_0 (EXTIRQ)
		 * as interrupt sources */
	if (ioctl(fd, IXPIO_WRITE_REG, &reg)) {
		close(fd);
		restore_old_sig_act();
		puts("Failure of configuring board interrupt.");
		return FAILURE;
	}

	if (signal_condition(fd)) {
		close(fd);
		restore_old_sig_act();
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
		restore_old_sig_act();
		puts("Failure of 8254 counter-1 mode.");
		return FAILURE;
	}
	reg.id = IXPIO_8254C4;
	reg.value = 0xd0;
	if (ioctl(fd, IXPIO_WRITE_REG, &reg)) {
		close(fd);
		restore_old_sig_act();
		puts("Failure of 8254 counter 1 LSB.");
		return FAILURE;
	}
	reg.value = 0x07;
	if (ioctl(fd, IXPIO_WRITE_REG, &reg)) {
		close(fd);
		restore_old_sig_act();
		puts("Failure of 8254 counter 1 MSB.");
		return FAILURE;
	}
	reg.id = IXPIO_8254CW2;
	reg.value = 0xb6;			/* counter-2, mode 3 */
	if (ioctl(fd, IXPIO_WRITE_REG, &reg)) {
		close(fd);
		restore_old_sig_act();
		puts("Failure of 8254 counter 2 mode.");
		return FAILURE;
	}
	reg.id = IXPIO_8254C5;
	reg.value = 0xd0;
	if (ioctl(fd, IXPIO_WRITE_REG, &reg)) {
		close(fd);
		restore_old_sig_act();
		puts("Failure of 8254 counter 2 LSB.");
		return FAILURE;
	}
	reg.value = 0x07;
	if (ioctl(fd, IXPIO_WRITE_REG, &reg)) {
		close(fd);
		restore_old_sig_act();
		puts("Failure of 8254 counter 2 MSB.");
		return FAILURE;
	}

	puts("Press <enter> to exit");
	while (getchar() != 10);

	close(fd);
	restore_old_sig_act();
	puts("End of program.");
	return SUCCESS;
}
