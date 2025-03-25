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

   v 0.0.0 27 Oct 2015 by Winson Chen
     create, blah blah... */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <signal.h>

#include "ixpci.h"

#define MY_SIG 55
volatile int data_count = 0;
int ai_count = 0;
volatile int AIvalue[10000] = {0};
int fd, count_limit = 1000;  //sample value
int sig_time = 0;

void sig_handler(int sig)
{
	ixpci_reg_t reg;
	int datacount;

	/* Read FIFO DATA */
        reg.id = IXPCI_AI_FD;

	for(datacount = 0; datacount < 1024; datacount++)
	{
		if(ioctl(fd, IXPCI_READ_REG, &reg)) {
	       	        puts("Failure of ioctl command IXPCI_WRITE_REG: IXPCI_AI_FD. sig");
       		}

		if(datacount % 16 == 2)
		{
			AIvalue[ai_count++] = reg.value;
			//printf("0x%x \n", AIvalue[ai_count-1]);
		}
	}
	printf("ai_count %d\n",ai_count);
	printf("sig times %d\n",sig_time++);

	/* Clear half-full FIFO Interrupt */
	reg.id = IXPCI_CI;
	reg.value = 0x0;
	if (ioctl(fd, IXPCI_WRITE_REG, &reg)) {
		puts("Failure of ioctl command IXPCI_WRITE_REG: IXPCI_RS_IC.");
	}

	/* Enable interrupt Enable FIFO_Half_Full int */
        reg.id = IXPCI_RS_IC;
        reg.value = (5<<4) | 0x1;

        if (ioctl(fd, IXPCI_WRITE_REG, &reg)) {
                puts("Failure of ioctl command IXPCI_WRITE_REG: IXPCI_RS_IC.");
        }

	data_count+=64;

	//usleep(10);
}

int main()
{
	int i, count = 0;
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
        sigaddset(&act.sa_mask, MY_SIG);

        if (sigaction(MY_SIG, &act, &act_old)) {
                close(fd);
                puts("Failure of signal action.");
                return FAILURE;
        }

	/* signal condition */
        sig.sid = MY_SIG;
        sig.pid = getpid();

        if (ioctl(fd, IXPCI_SET_SIG, &sig)) {
                close(fd);
                sigaction(MY_SIG, &act_old, NULL);
                puts("Failure of signal condition.");
                return FAILURE;
        }

	/* Clear FIFO */
	reg.id = IXPCI_AI_CF;
	reg.value = 0;

	if (ioctl(fd, IXPCI_WRITE_REG, &reg)) {
                close(fd);
                sigaction(MY_SIG, &act_old, NULL);
                puts("Failure of ioctl command IXPCI_WRITE_REG: IXPCI_AI_CF.");
                return FAILURE;
        }

	/* pacer trigger source internal clock trigger */
	reg.id = IXPCI_AIGR;
	reg.value = 0x8f;  //internal clock, bipolar 10 V, 16channels

	if (ioctl(fd, IXPCI_WRITE_REG, &reg)) {
		close(fd);
                sigaction(MY_SIG, &act_old, NULL);
		puts("Failure of ioctl command IXPCI_WRITE_REG: IXPCI_AIGR.");
                return FAILURE;
        }

	/* Enable interrupt Enable FIFO_Half_Full int */
	reg.id = IXPCI_RS_IC;
        reg.value = (5<<4) | 0x1;

        if (ioctl(fd, IXPCI_WRITE_REG, &reg)) {
		close(fd);
		sigaction(MY_SIG, &act_old, NULL);
                puts("Failure of ioctl command IXPCI_WRITE_REG: IXPCI_RS_IC.");
                return FAILURE;
        }

	/* set sampling rate */
	reg.id = IXPCI_AI_ICCS;
	reg.value = 20000;  //20M / 20000 = 1000 --> sampling rate

	if (ioctl(fd, IXPCI_WRITE_REG, &reg)) {
		close(fd);
                sigaction(MY_SIG, &act_old, NULL);
                puts("Failure of ioctl command IXPCI_WRITE_REG: IXPCI_AI_ICCS.");
                return FAILURE;
        }

	/* Clear FIFO */
        reg.id = IXPCI_AI_CF;
        reg.value = 0;

        if (ioctl(fd, IXPCI_WRITE_REG, &reg)) {
                close(fd);
                sigaction(MY_SIG, &act_old, NULL);
                puts("Failure of ioctl command IXPCI_WRITE_REG: IXPCI_AI_CF.");
                return FAILURE;
        }

	/* Start Pacer 0x14 */
	reg.id = IXPCI_AI_DAS;
	reg.value = 0x1;

	if (ioctl(fd, IXPCI_WRITE_REG, &reg)) {
                close(fd);
                sigaction(MY_SIG, &act_old, NULL);
                puts("Failure of ioctl command IXPCI_WRITE_REG: IXPCI_AI_DAS.");
                return FAILURE;
        }

	while( data_count < count_limit ){}

	/* Disable Interrupt */
	reg.id = IXPCI_RS_IC;
	reg.value = 0x0;

	if (ioctl(fd, IXPCI_WRITE_REG, &reg)) {
                close(fd);
                sigaction(MY_SIG, &act_old, NULL);
                puts("Failure of ioctl command IXPCI_WRITE_REG: IXPCI_RS_IC.");
                return FAILURE;
        }

	/* Clear all interrupt */
	reg.id = IXPCI_CI;
	reg.value = 0x0;

	if (ioctl(fd, IXPCI_WRITE_REG, &reg)) {
                close(fd);
                sigaction(MY_SIG, &act_old, NULL);
                puts("Failure of ioctl command IXPCI_WRITE_REG: IXPCI_CI.");
                return FAILURE;
	}

	/* Stop Pacer */
	reg.id = IXPCI_AI_DAS;
        reg.value = 0x0;

        if (ioctl(fd, IXPCI_WRITE_REG, &reg)) {
                close(fd);
                sigaction(MY_SIG, &act_old, NULL);
                puts("Failure of ioctl command IXPCI_WRITE_REG: IXPCI_AI_DAS.");
                return FAILURE;
        }

	/* Set Sampling Rate to Default */
	reg.id = IXPCI_AI_ICCS;
	reg.value = 0x63;

	if (ioctl(fd, IXPCI_WRITE_REG, &reg)) {
                close(fd);
                sigaction(MY_SIG, &act_old, NULL);
                puts("Failure of ioctl command IXPCI_WRITE_REG: IXPCI_AI_ICCS.");
                return FAILURE;
        }

	close(fd);
	sigaction(MY_SIG, &act_old, NULL);
        puts("\nEnd of program.");
	return SUCCESS;
}
