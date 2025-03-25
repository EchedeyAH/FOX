/* Example of interrupt handling for PIO-D144. This program configures
   driver to send signals in different signal id for the four interrupt
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

   v 0.0.0  8 Aug 2003 by Reed Lai
     What? Today is father's day? */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <signal.h>

#include "ixpio.h"

/* custom signal id, 33-63 */
#define MY_SIG_CH0 33
#define MY_SIG_CH1 34
#define MY_SIG_CH2 35
#define MY_SIG_CH3 36

struct sigaction act_ch0_old, act_ch1_old, act_ch2_old, act_ch3_old;

void sig_handler(int sig)
{
	static unsigned int sig_ch0, sig_ch1, sig_ch2, sig_ch3, unknown;

	switch (sig) {
	case MY_SIG_CH0:
		++sig_ch0;
		break;
	case MY_SIG_CH1:
		++sig_ch1;
		break;
	case MY_SIG_CH2:
		++sig_ch2;
		break;
	case MY_SIG_CH3:
		++sig_ch3;
		break;
	default:
		++unknown;
	}

	printf("\rGot single SIG_CH0:%u  SIG_CH1:%u  SIG_CH2:%u  SIG_CH3:%u  Unknown:%u  ", sig_ch0, sig_ch1, sig_ch2, sig_ch3, unknown);
}

void restore_old_sig_act()
{
	sigaction(MY_SIG_CH0, &act_ch0_old, NULL);
	sigaction(MY_SIG_CH1, &act_ch1_old, NULL);
	sigaction(MY_SIG_CH2, &act_ch2_old, NULL);
	sigaction(MY_SIG_CH3, &act_ch3_old, NULL);
}

int configure_sig_act()
{
	struct sigaction act;

	/* use a handler for all signals. */
	act.sa_handler = sig_handler;
	sigemptyset(&act.sa_mask);

	/* mask of signals which should be blocked during execution
	 * of the signal handler */
	sigaddset(&act.sa_mask, MY_SIG_CH0);
	sigaddset(&act.sa_mask, MY_SIG_CH1);
	sigaddset(&act.sa_mask, MY_SIG_CH2);
	sigaddset(&act.sa_mask, MY_SIG_CH3);

	if (sigaction(MY_SIG_CH0, &act, &act_ch0_old))
		return FAILURE;

	if (sigaction(MY_SIG_CH1, &act, &act_ch1_old)) {
		sigaction(MY_SIG_CH0, &act_ch0_old, NULL);
		return FAILURE;
	}

	if (sigaction(MY_SIG_CH2, &act, &act_ch2_old)) {
		sigaction(MY_SIG_CH0, &act_ch0_old, NULL);
		sigaction(MY_SIG_CH1, &act_ch1_old, NULL);
		return FAILURE;
	}

	if (sigaction(MY_SIG_CH3, &act, &act_ch3_old)) {
		sigaction(MY_SIG_CH0, &act_ch0_old, NULL);
		sigaction(MY_SIG_CH1, &act_ch1_old, NULL);
		sigaction(MY_SIG_CH2, &act_ch2_old, NULL);
		return FAILURE;
	}

	return SUCCESS;
}

int signal_condition(int fd)
{
	ixpio_signal_t sig;

	sig.pid = getpid();

	sig.sid = MY_SIG_CH0;
	sig.is = 0x1;     /* signaling for CH0 */
	sig.bedge = 0x1;  /* signaling for both edges */
	if (ioctl(fd, IXPIO_SIG, &sig))
		return FAILURE;

	sig.sid = MY_SIG_CH1;
	sig.is = 0x2;        /* signaling for CH1 */
	// sig.bedge = 0x1;  /* signaling for both edges */
	if (ioctl(fd, IXPIO_SIG_ADD, &sig))
		return FAILURE;

	sig.sid = MY_SIG_CH2;
	sig.is = 0x4;        /* signaling for CH2 */
	// sig.bedge = 0x1;  /* signaling for both edges */
	if (ioctl(fd, IXPIO_SIG_ADD, &sig))
		return FAILURE;

	sig.sid = MY_SIG_CH3;
	sig.is = 0x8;        /* signaling for CH3 */
	// sig.bedge = 0x1;  /* signaling for both edges */
	if (ioctl(fd, IXPIO_SIG_ADD, &sig))
		return FAILURE;

	return SUCCESS;
}

int main()
{
	int fd;
	char *dev_file;
	ixpio_reg_t reg;

	dev_file = "/dev/ixpio1";

	/* open device file */
	fd = open(dev_file, O_RDWR);
	if (fd < 0) {
		printf("Failure of open device file \"%s.\"\n", dev_file);
		return FAILURE;
	}

	if (configure_sig_act()) {
		close(fd);
		puts("Failure of signal action.");
	}

	/* configure ports */
	reg.id = IXPIO_AIOPCR;
	reg.value = 0x02;			/* port C/2 */
	if (ioctl(fd, IXPIO_WRITE_REG, &reg)) {
		close(fd);
		restore_old_sig_act();
		puts("Failure of configuring ports.");
		return FAILURE;
	}

	/* configure board interrupt */
	reg.id = IXPIO_IMCR;
	reg.value = 0xf;  /* enable P2C0/1/2/3 */
	if (ioctl(fd, IXPIO_WRITE_REG, &reg)) {
		close(fd);
		restore_old_sig_act();
		puts("Failure of configuring interrupt.");
		return FAILURE;
	}

	if (signal_condition(fd)) {
		close(fd);
		restore_old_sig_act();
		puts("Failure of signal condiction.");
		return FAILURE;
	}

	/* wait for exit */
	puts("press <enter> to exit\n");
	while (getchar() != 10);

	close(fd);
	restore_old_sig_act();
	puts("\nEnd of program.");
	return SUCCESS;
}
