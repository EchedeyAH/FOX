/* Example of Digital I/O for PCI-826 PCI-822

   Author: WInson Chen

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

    This program shows digital io by ioctl register read/write command.

   v 0.0.0 17 Oct 2013 by Winson chen
     create */

#include<stdio.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<sys/ioctl.h>
#include<unistd.h>
#include<string.h>

#include"ixpci.h"

int main()
{
	int fd,ch, rs;
	int sig, i = 0;
	char *dev_file;
	float n;
	ixpci_reg_t ai, aist, wcon, rcon, wmode, wssms;
	
	dev_file = "/dev/ixpci1";

	fd = open(dev_file, O_RDWR);

	/* magic scan mode setting */
	wmode.id = IXPCI_RW_MS_BF_CS;
	wmode.value = 0x0000;	//polling

	/* start/stop magic scan */
	wssms.id = IXPCI_SS_MS;
	wssms.value = 0x4000;	//disable pacer

	/* A/D polling config */
	rcon.id = IXPCI_ADPR;
	wcon.id = IXPCI_ADPR;
	printf("Select AI Channel:(0~31)");
	rs = scanf("%d",&ch);
	wcon.value = 0x8010 + ch;  //select AI channel

	/* A/D trigger for polling mode */
	aist.id = IXPCI_ADST;
	aist.value = 0x0000;
	
	/* read data from FIFO */
	ai.id = IXPCI_AI;

	if(ioctl(fd, IXPCI_WRITE_REG, &wmode)){
                puts("Failure of ioctl command IXPCI_WRITE_REG: IXPCI_RW_MS_BF_CS.");
                close(fd);
                return FAILURE;
        }

	if(ioctl(fd, IXPCI_WRITE_REG, &wssms)){
                puts("Failure of ioctl command IXPCI_WRITE_REG: IXPCI_SS_MS.");
                close(fd);
                return FAILURE;
        }

        if(ioctl(fd, IXPCI_WRITE_REG, &wcon)){
                puts("Failure of ioctl command IXPCI_WRITE_REG: IXPCI_ADPR.");
                close(fd);
                return FAILURE;
        }

        if(ioctl(fd, IXPCI_READ_REG, &rcon)){
                puts("Failure of ioctl command IXPCI_READ_REG: IXPCI_ADPR.");
                close(fd);
                return FAILURE;
        }

        printf("Polling config is 0x%x.\n",rcon.value);
	printf("Confirm your JP1 status.\n");
	
	sleep(1.5);
	while(1){
		if(ioctl(fd, IXPCI_WRITE_REG, &aist)){
			printf("Failure of ioctl command IXPCI_WRITE_REG: IXPCI_ADST.");
			close(fd);
			return FAILURE;
		}
		if(ioctl(fd, IXPCI_READ_REG, &ai)){
			printf("Failure of ioctl command IXPCI_READ_REG: IXPCI_AI.");
			close(fd);
			return FAILURE;
		}
		if(i == 0){
			i++;
			continue;
		}
		//printf("ai.value is %d\n",ai.value);
		usleep(100000);
		if(ai.value > 32767){
			n = ai.value / 3276.8 - 20;
			printf("AI value is %.2f\n",n);
			ai.value = 0;
		}
		else{
			printf("AI value is %.2f\n",ai.value / 3276.8 );
			ai.value = 0;
		}
	}	
	close(fd);
	return SUCCESS;
}
