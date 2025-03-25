/* Example of counter for PIO-DA16/DA8/DA4.

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
     Complex signal.

   v 0.0.1  1 Jul 2003 by Reed Lai
     Corrected the value of the INT1_NEG_EDGE.

   v 0.0.0  9 Dec 2002 by Reed Lai
     create, blah blah... */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <signal.h>

#include "ixpio.h"

/* custom signal, 33-63 */
#define MY_SIG 34

#define INT1           2
#define INT1_NEG_EDGE  0

int fd;
ixpio_digital_t din, dout;

void sig_handler(int sig)
{
	static unsigned int beat;
	if (ioctl(fd, IXPIO_DIGITAL_OUT, &dout))
		puts("Failure of digital output.");
	if (ioctl(fd, IXPIO_DIGITAL_IN, &din))
		puts("Failure of digital input.");

	printf("\rTime beat: %d    Digital Output: 0x%04x    Input: 0x%04x    <enter> exit ", ++beat, dout.data.u16, din.data.u16);

	if (dout.data.u16 == 0x8000) dout.data.u16 = 1;
	else dout.data.u16 <<= 1;
}

int main()
{
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

	din.data.u16 = 0;
	dout.data.u16 = 1;

	/* set action for signal */
	act.sa_handler = sig_handler;
	sigemptyset(&act.sa_mask);
	sigaddset(&act.sa_mask, MY_SIG);
	sigaction(MY_SIG, &act, &act_old);

	/* config board interrupt */
	reg.id = IXPIO_IMCR;
	reg.value = INT1;
		/* enable INT_CHAN_1 (INT1) for interrupt */

	if (ioctl(fd, IXPIO_WRITE_REG, &reg)) {
		close(fd);
		sigaction(MY_SIG, &act_old, NULL);
		puts("Failure of configuring interrupt.");
		return FAILURE;
	}

	/* signal condiction */
	sig.sid = MY_SIG;
	sig.pid = getpid();
	sig.is = INT1;
		/* INT_CHAN_1 */
	sig.edge = INT1_NEG_EDGE;
		/* signal for negative edge */
	sig.bedge = 0;
		/* don't signal for both edges */

	if (ioctl(fd, IXPIO_SET_SIG, &sig)) {
		close(fd);
		sigaction(MY_SIG, &act_old, NULL);
		puts("Failure of signal condiction.");
		return FAILURE;
	}

	/*****************************************
	 * setup 8254 counter-1/2 for INT_CHAN_1 *
	 *                                       *
	 * 4MHz / (40000 * 100) = 1Hz            *
	 *****************************************/

	reg.id = IXPIO_8254CW;
	reg.value = 0x76;			/* counter-1, mode 3 */
	if (ioctl(fd, IXPIO_WRITE_REG, &reg)) {
		close(fd);
		sigaction(MY_SIG, &act_old, NULL);
		puts("Failure of 8254 counter 1 mode.");
		return FAILURE;
	}
	reg.id = IXPIO_8254C1;
	reg.value = 0x40;
	if (ioctl(fd, IXPIO_WRITE_REG, &reg)) {
		close(fd);
		sigaction(MY_SIG, &act_old, NULL);
		puts("Failure of 8254 counter 1 low byte.");
		return FAILURE;
	}
	reg.value = 0x9c;  /* 0x9c40 == 40000 */
	if (ioctl(fd, IXPIO_WRITE_REG, &reg)) {
		close(fd);
		sigaction(MY_SIG, &act_old, NULL);
		puts("Failure of 8254 counter 1 high byte.");
		return FAILURE;
	}
	reg.id = IXPIO_8254CW;
	reg.value = 0xb6;			/* counter-2, mode 3 */
	if (ioctl(fd, IXPIO_WRITE_REG, &reg)) {
		close(fd);
		sigaction(MY_SIG, &act_old, NULL);
		puts("Failure of counter 2 mode.");
		return FAILURE;
	}
	reg.id = IXPIO_8254C2;
	reg.value = 100;
	if (ioctl(fd, IXPIO_WRITE_REG, &reg)) {
		close(fd);
		sigaction(MY_SIG, &act_old, NULL);
		puts("Failure of 8254 counter 2 low byte.");
		return FAILURE;
	}
	reg.value = 0;
	if (ioctl(fd, IXPIO_WRITE_REG, &reg)) {
		close(fd);
		sigaction(MY_SIG, &act_old, NULL);
		puts("Failure of 8254 counter 2 high byte.");
		return FAILURE;
	}
	/* press <enter> to get out */
	while (getchar() != 10);

	close(fd);
	sigaction(MY_SIG, &act_old, NULL);
	puts("End of program.");
	return SUCCESS;
}
