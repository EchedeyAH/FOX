/* Read ADC result by pacer trigger and interrupt.

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

   v 0.0.0 21 Jan 2003 by Reed Lai
     Create */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <signal.h>

#include "ixpci.h"

int fd;

void sig_handler(int sig)
{
	ixpci_reg_t reg;

	/* analog input */
	reg.id = IXPCI_AI;
	reg.mode = IXPCI_RM_NORMAL;

	if (ioctl(fd, IXPCI_READ_REG, &reg))
		puts("Failure of analog input.");
	printf("%x ", reg.value);
}

int main()
{
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

	/* channel 0 */
	reg.id = IXPCI_AICR;
	reg.value = 0;
	if (ioctl(fd, IXPCI_WRITE_REG, &reg)) {
		close(fd);
		sigaction(SIGUSR2, &act_old, NULL);
		puts("Failure of channel.");
		return FAILURE;
	}

	/* gain 1 */
	reg.id = IXPCI_AIGR;
	reg.value = 0;
	if (ioctl(fd, IXPCI_WRITE_REG, &reg)) {
		close(fd);
		sigaction(SIGUSR2, &act_old, NULL);
		puts("Failure of gain control.");
		return FAILURE;
	}
	usleep(23);  /* delay 23us for channel switching */

	/* pacer trigger, interrput */
	reg.id = IXPCI_CR;
	reg.value = 0x04;
	if (ioctl(fd, IXPCI_WRITE_REG, &reg)) {
		close(fd);
		sigaction(SIGUSR2, &act_old, NULL);
		puts("Failure of configuring pacer trigger.");
		return FAILURE;
	}

	/* 8254 counter 0 */
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
		puts("Failure of 8254 counter 0 LSB.");
		return FAILURE;
	}
	//reg.value = 0xff;
	if (ioctl(fd, IXPCI_WRITE_REG, &reg)) {
		close(fd);
		sigaction(SIGUSR2, &act_old, NULL);
		puts("Failure of 8254 counter 0 MSB.");
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
	puts("Press Enter key to stop.");
	while (getchar() != '\n');

	sigaction(SIGUSR2, &act_old, NULL);
	puts("\nEnd of program.");
	close(fd);
	return SUCCESS;
}
