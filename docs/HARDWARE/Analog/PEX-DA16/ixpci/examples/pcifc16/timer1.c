/* Example of Timer1 Counter/Frequency for PCI-FC16

   Author: Golden Wang

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

   v 0.0.0 23 Mar 2017 by Golden Wang
     create */

/* *INDENT-OFF* */
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
	int fd;
	char *dev_file;
	ixpci_reg_t rd0,reg;
	unsigned int channel = 8;
	
	dev_file = "/dev/ixpci1";

	fd = open(dev_file, O_RDWR);
	if (fd < 0){
		printf("Cannot open device file \"%s.\"\n",dev_file);
		return FAILURE;
	}
	reg.id = IXPCI_T1CM;	//set channel mode(counter or frequency)
        reg.value = 0;  //set CH8 ~ CH15 = counter channel
			//0 = counter channel
			//1 = frequency channel

        if(ioctl(fd, IXPCI_WRITE_REG, &reg)){
                puts("Failure of ioctl command IXPCI_WRITE_REG: IXPCI_T1CM.");
                close(fd);
                return FAILURE;
        }

        printf("set  CH8 ~ CH15 = counter channel.\n");

	reg.id = IXPCI_T1SM;	//set channel speed mode
	reg.value = 0;  //set CH8 ~ CH15 speed mode = 1Hz ~ 1kHz
                        //0 = 1 Hz ~ 1kHz
                        //1 = 1 kHz ~ 250 kHz

        if(ioctl(fd, IXPCI_WRITE_REG, &reg)){
                puts("Failure of ioctl command IXPCI_WRITE_REG: IXPCI_T1SM.");
                close(fd);
                return FAILURE;
        }

        printf("set  CH8 ~ CH15 speed mode = 1Hz ~ 1kHz.\n");

	reg.id = IXPCI_T1SC;	//select channel 8~15
	reg.value = channel;  //select channel 8

        if(ioctl(fd, IXPCI_WRITE_REG, &reg)){
                puts("Failure of ioctl command IXPCI_WRITE_REG: IXPCI_T1SC.");
                close(fd);
                return FAILURE;
        }
	printf("Select channel CH%d.\n", channel);
	
	reg.id = IXPCI_CLEAR_C1;	//clear channel 8~15
	reg.value = channel;	//clear channel 8

	if(ioctl(fd, IXPCI_WRITE_REG, &reg)){
		puts("Failure of ioctl command IXPCI_WRITE_REG: IXPCI_T1CC.");
		close(fd);
		return FAILURE;
	}
	
	printf("Clear channel CH%d.\n", channel);

	puts("ESC and Enter to go out. Enter to continue.");
	while(getchar() != 27)
	{
		reg.id = IXPCI_T1LC;	//latch channel 8~15
		reg.value = channel;  //latch channel 8

	        if(ioctl(fd, IXPCI_WRITE_REG, &reg)){
        	        puts("Failure of ioctl command IXPCI_WRITE_REG: IXPCI_T1LC.");
                	close(fd);
	                return FAILURE;
        	}
        
		printf("Latch channel CH%d.\n", channel);
	
		reg.id = IXPCI_READ_C1;	//read Timer1 counter
		reg.value = 0;

	        if(ioctl(fd, IXPCI_READ_REG, &reg)){    
                	puts("Failure of ioctl command IXPI_READ_REG: IXPCI_T1RD.");
        	        close(fd);
	                return FAILURE;
        	}

	        printf("Read Timer1 CH%d counter =  %d\n", channel, reg.value);
	}

	close(fd);

	return SUCCESS;
}
