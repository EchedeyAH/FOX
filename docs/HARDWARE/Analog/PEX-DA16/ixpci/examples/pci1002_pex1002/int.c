/* Example of interrupt handle for PCI-1002

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

   This program shows the basic interrupt operation.

   v 0.0.0 15 Jan 2003 by Reed Lai
     create, blah blah... */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <signal.h>

#include "ixpci.h"

void sig_handler(int sig)
{
	static unsigned int sig_counter;
	static unsigned int i;

	++sig_counter;
	++i;

	if (i > 9) {
		printf("\rGot single %d for %u time(s) ", sig, sig_counter);
		i = 0;
	}
}

int main()
{
	int fd;
	char *dev_file;
	ixpci_reg_t reg;
	ixpci_signal_t sig;

	static struct sigaction act, act_old;

	dev_file = "/dev/ixpci1";

	/* open device file */
	fd = open(dev_file, O_RDWR);
	if (fd < 0) {
		printf("Failure of open device file \"%s.\"\n", dev_file);
		return FAILURE;
	}

	/* signal action */
	act.sa_handler = sig_handler;
	sigemptyset(&act.sa_mask);
	sigaddset(&act.sa_mask, SIGUSR2);
	if (sigaction(SIGUSR2, &act, &act_old)) {
		close(fd);
		puts("Failure of signal action.");
		return FAILURE;
	}

	/* signal condition */
	sig.sid = SIGUSR2;
	sig.pid = getpid();
	if (ioctl(fd, IXPCI_SET_SIG, &sig)) {
		close(fd);
		sigaction(SIGUSR2, &act_old, NULL);
		puts("Failure of signal condition.");
		return FAILURE;
	}

	/* choose the interrupt source */
	reg.id = IXPCI_CR;
	reg.value = 0x08;  /* 8254 counter 0 */
	if (ioctl(fd, IXPCI_WRITE_REG, &reg)) {
		close(fd);
		sigaction(SIGUSR2, &act_old, NULL);
		puts("Failure of choosing interrupt source.");
		return FAILURE;
	}

	/* configure the 8254 counter 0 */
	reg.id = IXPCI_8254CR;
	reg.value = 0x36;  /* counter-0, mode 3 */
	if (ioctl(fd, IXPCI_WRITE_REG, &reg)) {
		close(fd);
		sigaction(SIGUSR2, &act_old, NULL);
		puts("Failure of 8254 control register.");
		return FAILURE;
	}
	reg.id = IXPCI_8254C0;
	reg.value = 0xff;
	if (ioctl(fd, IXPCI_WRITE_REG, &reg)) {
		close(fd);
		sigaction(SIGUSR2, &act_old, NULL);
		puts("Failure of 8254 counter 1 LSB.");
		return FAILURE;
	}

	reg.value = 0xff;
	if (ioctl(fd, IXPCI_WRITE_REG, &reg)) {
		close(fd);
		sigaction(SIGUSR2, &act_old, NULL);
		puts("Failure of 8254 counter 1 MSB.");
		return FAILURE;
	}

	/* enable board interrupt */
	if (ioctl(fd, IXPCI_IRQ_ENABLE)) {
		close(fd);
		sigaction(SIGUSR2, &act_old, NULL);
		puts("Failure of enabling board irq.");
		return FAILURE;
	}

	/* wait for exit */
	puts("Press Enter key to exit");
	while (getchar() != 10);

	close(fd);
	sigaction(SIGUSR2, &act_old, NULL);
	puts("\nEnd of program.");
	return SUCCESS;
}
