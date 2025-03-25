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
#include <sys/time.h>

#include"ixpci.h"

int main()
{
	int fd,ch, rs;
	int sig, i = 0;
	char *dev_file;
	float input_value, output_value;
	ixpci_reg_t reg, ai, aist;
	struct timeval before,after;
	
	dev_file = "/dev/ixpci1";

	fd = open(dev_file, O_RDWR);

//AI setting
//---------------------------------------------------------------------------
	/* magic scan mode setting */
	reg.id = IXPCI_RW_MS_BF_CS;
	reg.value = 0x0000;	//polling

	if(ioctl(fd, IXPCI_WRITE_REG, &reg)){
                puts("Failure of ioctl command IXPCI_WRITE_REG: IXPCI_RW_MS_BF_CS.");
                close(fd);
                return FAILURE;
        }

	/* start/stop magic scan */
	reg.id = IXPCI_SS_MS;
	reg.value = 0x4000;	//disable pacer

	if(ioctl(fd, IXPCI_WRITE_REG, &reg)){
                puts("Failure of ioctl command IXPCI_WRITE_REG: IXPCI_SS_MS.");
                close(fd);
                return FAILURE;
        }

	/* A/D polling config */
	reg.id = IXPCI_ADPR;
//	printf("Select AI Channel:(0~31)");
//	rs = scanf("%d",&ch);
	ch = 0;
	reg.value = 0x8010 + ch;  //select AI channel 0

	if(ioctl(fd, IXPCI_WRITE_REG, &reg)){
                puts("Failure of ioctl command IXPCI_WRITE_REG: IXPCI_ADPR.");
                close(fd);
                return FAILURE;
        }

	/* read data from FIFO */
	ai.id = IXPCI_AI;
	
	/* A/D trigger for polling mode */
	aist.id = IXPCI_ADST;
	aist.value = 0x0000;
//---------------------------------------------------------------------------

//AO setting
//---------------------------------------------------------------------------
	/* enable/disable AO channel */
        reg.id = IXPCI_ED_DA_CH;
        reg.value = 3;

	if(ioctl(fd, IXPCI_WRITE_REG, &reg)){
                puts("Failure of ioctl command IXPCI_WRITE_REG: IXPCI_ED_DA_CH.");
                close(fd);
                return FAILURE;
        }

        /* Set Analog output channel 0 */
        reg.id = IXPCI_RS_DA_CS;
        reg.value = 0;

        if(ioctl(fd, IXPCI_WRITE_REG, &reg)){
                puts("Failure of ioctl command IXPCI_WRITE_REG IXPCI_RS_DA_CS.");
                close(fd);
                return FAILURE;
        }
//---------------------------------------------------------------------------
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
	//	printf("ai.value is %d\n",ai.value);
	//	usleep(100000);

		if(ai.value > 32767){
			input_value = ai.value / 3276.8 - 20;
			printf("AI value is %.2f\n",input_value);
			ai.value = 0;
		}
		else{
			input_value = ai.value / 3276.8;
			printf("AI value is %.2f\n",input_value);
			ai.value = 0;
		}

//		gettimeofday(&before,NULL);
		if(input_value >= 5.0)	//Analog intput value >= 5.0 will send analog output value
		{
			reg.id = IXPCI_RW_DA;

			output_value = 5;	//Analog output value

		        if(output_value == 10)
		                reg.value = 3276.8 * (output_value + 10) - 1;
		        else
                		reg.value = 3276.8 * (output_value + 10);

		        /* write DATA to AO */
		        if(ioctl(fd, IXPCI_WRITE_REG, &reg)){
                		puts("Failure of ioctl command IXPCI_WRITE_REG: IXPCI_RW_DA.");
		                close(fd);
                		return FAILURE;
		        }
//			gettimeofday(&after,NULL);

		        printf("The Analog output channel is set 0\n");

			reg.id = IXPCI_RW_DA;

		        if(ioctl(fd, IXPCI_READ_REG, &reg)){
		                puts("Failure of ioctl command IXPCI_READ_REG: IXPCI_RW_DA.");
                		close(fd);
		                return FAILURE;
		        }
                        printf("READ D/A DATA is %.2f\n",reg.value / 3276.8 - 10);

			break;
		}
	}

//	printf("%ld - %ld = %ldus.\n", after.tv_usec,before.tv_usec,after.tv_usec-before.tv_usec);
	printf("When AI channel 0 value > 5.0V, AO channel 0 send 5V\n");
	close(fd);
	return SUCCESS;
}
