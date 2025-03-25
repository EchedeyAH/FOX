/* Example of Change of status for PCI-D96/128SU

   Author: Winson Chen

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

   v 0.0.0  24 Jun 2020 by Winson Chen
     Set PA as DO port and PB as DI port, 
     Set PB Change of status with interrupt mode.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include "ixpci.h"

int count = 0;

void sig_handler(int sig)
{
	//printf("Got single %d times\n",++count);
	++count;
}

int main()
{
	int fd_dev, tmp;
	char *dev_file;
	ixpci_reg_t reg;
	ixpci_signal_t sig;
	unsigned int dwValue, dwDOVal;

        static struct sigaction act, act_old;
	
	dev_file = "/dev/ixpci1";
		/* change this for your device entry */

	/* open device */
	fd_dev = open(dev_file, O_RDWR);
	if (fd_dev < 0) {
		printf("Cannot open device file \"%s.\"\n", dev_file);
		return FAILURE;
	}

	/* signal action */
        act.sa_handler = sig_handler;
        sigemptyset(&act.sa_mask);
        sigaddset(&act.sa_mask, SIGUSR2);
        if (sigaction(SIGUSR2, &act, &act_old)) {
                close(fd_dev);
                puts("Failure of signal action.");
                return FAILURE;
        }

	/* signal condition */
        sig.sid = SIGUSR2;
        sig.pid = getpid();
        if (ioctl(fd_dev, IXPCI_SET_SIG, &sig)) {
                close(fd_dev);
                sigaction(SIGUSR2, &act_old, NULL);
                puts("Failure of signal condition.");
                return FAILURE;
        }

	printf("Set PortA as DO mode, PortB as DI mode(Change of status) demo.\n");

	//Set PA to DO Mode
	reg.id = IXPCI_DIOPA_CR;
        reg.value = 0xf;
        if (ioctl(fd_dev, IXPCI_WRITE_REG, &reg)) {
                puts("Failure of ioctl command IXPCI_WRITE_REG.");
                close(fd_dev);
                return FAILURE;
        }

	//Get CompareTrigger/Change Status Readback.
	reg.id = IXPCI_PPM_CS;
	if (ioctl(fd_dev, IXPCI_READ_REG, &reg)) {
		puts("Failure of ioctl command IXPCI_READ_REG.");
		close(fd_dev);
		return FAILURE;
	}
	dwValue = reg.value;

	//Close PortB Compare Trigger/Change Status
	reg.id = IXPCI_PPM_CS;
	reg.value = dwValue & 0x1d;
	if (ioctl(fd_dev, IXPCI_WRITE_REG, &reg)) {
		puts("Failure of ioctl command IXPCI_WRITE_REG.");
		close(fd_dev);
		return FAILURE;
	}

	//Get CompareTrigger/Change Status Readback.
	reg.id = IXPCI_PPM_CS;
	if (ioctl(fd_dev, IXPCI_READ_REG, &reg)) {
		puts("Failure of ioctl command IXPCI_READ_REG.");
		close(fd_dev);
		return FAILURE;
	}
	dwValue = reg.value;

	//Set PortB as Change of status
	reg.id = IXPCI_PPM_CS;
	reg.value = dwValue | 0x20;
	if (ioctl(fd_dev, IXPCI_WRITE_REG, &reg)) {
		puts("Failure of ioctl command IXPCI_WRITE_REG.");
		close(fd_dev);
		return FAILURE;
	}

	/* enable board interrupt */
	if (ioctl(fd_dev, IXPCI_IRQ_ENABLE)) {
		close(fd_dev);
		sigaction(SIGUSR2, &act_old, NULL);
		puts("Failure of enabling board irq.");
		return FAILURE;
        }

	/* wait for exit */
        puts("DO output 0 to 0xfff\n");
	for(dwDOVal = 0; dwDOVal < 0xfff; dwDOVal++)
	{
		/* Set PortA DO value */
		reg.id = IXPCI_DIOPA;   //PA output value
	        reg.value = dwDOVal;
        	if (ioctl(fd_dev, IXPCI_WRITE_REG, &reg)) {
                	puts("Failure of ioctl command IXPCI_WRITE_REG.");
	                close(fd_dev);
        	        return FAILURE;
	        }

		usleep(20);

		/* Read PortB value */
		reg.id = IXPCI_DIOPB;
                if (ioctl(fd_dev, IXPCI_READ_REG, &reg)) {
                        puts("Failure of ioctl command IXPCI_READ_REG.");
                        close(fd_dev);
                        return FAILURE;
                }
		printf("Read PortB value is 0x%x\n",reg.value);	//printf may cause loop stop
/*
		// Read status
		reg.id = IXPCI_PPM_CS;
		if (ioctl(fd_dev, IXPCI_READ_REG, &reg)) {
			puts("Failure of ioctl command IXPCI_READ_REG.");
			close(fd_dev);
			return FAILURE;
		}
		printf("\"Change of status\" status value 0x%x\n",reg.value);
*/
	}

	if (ioctl(fd_dev, IXPCI_IRQ_DISABLE)) {
                close(fd_dev);
                sigaction(SIGUSR2, &act_old, NULL);
                puts("Failure of enabling board irq.");
                return FAILURE;
        }

	printf("Got single %d times\n",count);

	close(fd_dev);
	sigaction(SIGUSR2, &act_old, NULL);
        puts("\nEnd of program.");
	return SUCCESS;
}
