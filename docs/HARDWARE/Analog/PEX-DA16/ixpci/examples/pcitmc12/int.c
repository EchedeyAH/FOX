/* Example of interrupt handle for PCI-TMC12

   Author: Reed lai

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

   J26 selects CLOCK2 for 80K
               3
             +---+
             |   |
        800K | o |
             |+-+|
      CLOCK2 ||o||
             || ||
         80K ||o||
             |+-+|
             +---+
              J26

   J24 feeds CLOCK2 to CLK12
            1 2
          +-----+
      J24 |  +-+|
          | o|o||
          |  | ||
          | o|o||
          |  +-+|
          | o o |
          |     |
          +-----+

   J25 selects the interrupt source as CH3
            J25
              +-----+
              |+---+|
          CH3 ||o o||
              |+---+|
          CH6 | o o |
              |     |
          CH9 | o o |
              |     |
         CH12 | o o |
              |     |
          EXT | o o |
              |     |
      (SPARE) | o o |
              |     |
              +-----+

   then you can rocking roll now.

   v 0.0.0 18 Mar 2003 by Reed Lai
     Hello world */

/* *INDENT-OFF* */
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
	static unsigned int sig_counter, dout;

	if (dout == 0x8000 || dout == 0) dout = 1;
	else dout <<= 1;
	ioctl(fd, IXPCI_IOCTL_DO, &dout); // DO scanning
	printf("\rGot single %d for %u time(s), DO = 0x%04x ", sig, ++sig_counter, dout);
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
	sig.bedge = 0;  // set if signal for both edges
	sig.edge = 0; // falling edge
	sig.pid = getpid();
	if (ioctl(fd, IXPCI_SET_SIG, &sig)) {
		close(fd);
		sigaction(SIGUSR2, &act_old, NULL);
		puts("Failure of signal condition.");
		return FAILURE;
	}

	/* set IntXor On to invert the int source */
	reg.id = IXPCI_XOR;
	reg.value = 0x1000;	//counter3
	if (ioctl(fd, IXPCI_WRITE_REG, &reg)) {
                close(fd);
                sigaction(SIGUSR2, &act_old, NULL);
                puts("Failure of 8254 chip selecting.");
                return FAILURE;
        }

	/*pre-set int_signal_to_PC to High value to enable next interrupt operation*/
	reg.id = IXPCI_XOR;
	if (ioctl(fd, IXPCI_READ_REG, &reg)) {
                close(fd);
                sigaction(SIGUSR2, &act_old, NULL);
                puts("Failure of 8254 chip selecting.");
                return FAILURE;
        }

	/* select the 8254 chip 1 */
	reg.id = IXPCI_8254CS;
	reg.value = 0;  /* chip 1, counter 1-3 */
	if (ioctl(fd, IXPCI_WRITE_REG, &reg)) {
		close(fd);
		sigaction(SIGUSR2, &act_old, NULL);
		puts("Failure of 8254 chip selecting.");
		return FAILURE;
	}

	/* configure the 8254 counter 2 */
	reg.id = IXPCI_8254CR;
	reg.value = 0xb6;  // counter 2 on chip, counter 12 on board, mode 3
	if (ioctl(fd, IXPCI_WRITE_REG, &reg)) {
		close(fd);
		sigaction(SIGUSR2, &act_old, NULL);
		puts("Failure of 8254 control register.");
		return FAILURE;
	}
	reg.id = IXPCI_8254C2;
	reg.value = 0x3f;
	if (ioctl(fd, IXPCI_WRITE_REG, &reg)) {
		close(fd);
		sigaction(SIGUSR2, &act_old, NULL);
		puts("Failure of 8254 counter LSB.");
		return FAILURE;
	}
	reg.value = 0x9c;
	if (ioctl(fd, IXPCI_WRITE_REG, &reg)) {
		close(fd);
		sigaction(SIGUSR2, &act_old, NULL);
		puts("Failure of 8254 counter MSB.");
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
/* *INDENT-ON* */
