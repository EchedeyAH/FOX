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

   v 0.0.0 18 Oct 2013 by Winson chen
     Magicscan can scan mulitiple channels.
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
	int fd, chcount, j = 1;
	char *dev_file;	
	float n;
	ixpci_reg_t wssms, wai, wsample, wmagic, wbfms, ai, aist;

	dev_file = "/dev/ixpci1";

	fd = open(dev_file, O_RDWR);
	if(fd < 0){
		printf("Cannot not open device file \"%s.\"\n",dev_file);
		return FAILURE;
	}
	/* start/stop magic scan */
	wssms.id = IXPCI_SS_MS;
        wssms.value = 0x4000;   //disable pacer

	if(ioctl(fd, IXPCI_WRITE_REG, &wssms)){
		puts("Failure of ioctl command IXPCI_WRITE_REG: IXPCI_SS_MS.");
		close(fd);
		return FAILURE;
	}

	/* select first channel */
	wai.id = IXPCI_AI_PC;
	wai.value = 0x8010;

	if(ioctl(fd, IXPCI_WRITE_REG, &wai)){
                puts("Failure of ioctl command IXPCI_WRITE_REG: IXPCI_AI_PC.");
                close(fd);
                return FAILURE;
        }

	/* select second channel */
	wai.id = IXPCI_AI_PC;
	wai.value = 0x8411;

	if(ioctl(fd, IXPCI_WRITE_REG, &wai)){
                puts("Failure of ioctl command IXPCI_WRITE_REG: IXPCI_AI_PC.");
                close(fd);
                return FAILURE;
        }

	/* select third channel */
	wai.id = IXPCI_AI_PC;
	wai.value = 0x8812;
	
	if(ioctl(fd, IXPCI_WRITE_REG, &wai)){
                puts("Failure of ioctl command IXPCI_WRITE_REG: IXPCI_AI_PC.");
                close(fd);
                return FAILURE;
        }

	wai.id = IXPCI_AI_PC;
        wai.value = 0x8c13;

        if(ioctl(fd, IXPCI_WRITE_REG, &wai)){
                puts("Failure of ioctl command IXPCI_WRITE_REG: IXPCI_AI_PC.");
                close(fd);
                return FAILURE;
        }

	/* set sampling rate */
	wsample.id = IXPCI_RS_SR;
	wsample.value = 28;

	if(ioctl(fd, IXPCI_WRITE_REG, &wsample)){
                puts("Failure of ioctl command IXPCI_WRITE_REG: IXPCI_RS_SR.");
                close(fd);
                return FAILURE;
        }

	/* magic scan max count is 5000 */
	wmagic.id = IXPCI_RS_CN_MS;
	wmagic.value = 5000;
	
	if(ioctl(fd, IXPCI_WRITE_REG, &wmagic)){
                puts("Failure of ioctl command IXPCI_WRITE_REG: IXPCI_RS_CN_MS.");
                close(fd);
                return FAILURE;
        }

	/* select channel number and mode */
	wbfms.id = IXPCI_RW_MS_BF_CS;
	wbfms.value = 0x0011;

	if(ioctl(fd, IXPCI_WRITE_REG, &wbfms)){
                puts("Failure of ioctl command IXPCI_WRITE_REG: IXPCI_RW_MS_BF_CS.");
                close(fd);
                return FAILURE;
        }

	/* start/stop magic scan */
	wssms.id = IXPCI_SS_MS;
	wssms.value = 0x8000;

	if(ioctl(fd, IXPCI_WRITE_REG, &wssms)){
                puts("Failure of ioctl command IXPCI_WRITE_REG: IXPCI_SS_MS.");
                close(fd);
                return FAILURE;
        }

	/* A/D conversion */
	aist.id = IXPCI_ADST;
        aist.value = 0;

	/* read AI data from FIFO */
        ai.id = IXPCI_AI;

	chcount = ((wbfms.value >> 3) & 0xf) + 1 ;
	printf("Select %d channels\n", chcount);

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
		
		if(j == chcount + 1){
                        printf("\n");
                        j = 1;
                }
		printf("%d.",j);

                if(ai.value > 32767){
                        n = ai.value / 3276.8 - 20;
                        printf("AI value is %.2f\n",n);
                }
                else
                        printf("AI value is %.2f\n",ai.value / 3276.8);
		j++;
		usleep(180000);
	}
	close(fd);
	return SUCCESS;
}
