/* Example of Card ID for PCI-2602U

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

   v 0.0.0 27 Oct 2023 by Winson Chen
     Set/Get PA PB PC PD DIO mode. */

//	PD   PC   PB   PA
//	bit3 bit2 bit1 bit0

//	set 1 indicate DO, set 0 indicate DI
//	set PA~PD all as DO, value is 0xf, set PA~PD all as DI, value is 0x0

#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "ixpci.h"

int main()
{
	int fd_dev, i;
        ixpci_reg_t reg;
        char *dev_file;

        dev_file = "/dev/ixpci1";
        /* change this for your device entry */

        /* open device */
        fd_dev = open(dev_file, O_RDWR);
        if (fd_dev < 0) {
                printf("Cannot open device file \"%s.\"\n", dev_file);
                return FAILURE;
        }

	//Read DIO port mode
	reg.id = IXPCI_DIO_PABCD_C;
	if (ioctl(fd_dev, IXPCI_READ_REG, &reg)) {
                puts("Failure of ioctl command IXPCI_READ_REG.");
                close(fd_dev);
                return FAILURE;
        }

	printf("reg.value is 0x%x\n",reg.value);

	if(reg.value & 0x1)
	        printf("Read PortA DO.");
	else
		printf("Read PortA DI.");

	if(reg.value & 0x2)
	        printf(" PortB DO.");
	else
		printf(" PortB DI.");

	if(reg.value & 0x4)
	        printf(" PortC DO.");
	else
		printf(" PortC DI.");

	if(reg.value & 0x8)
	        printf(" PortD DO.\n");
	else
		printf(" PortD DI.\n");

        //Set DIO port mode
        reg.id = IXPCI_DIO_PABCD_C;
        reg.value = 0x1;
        if (ioctl(fd_dev, IXPCI_WRITE_REG, &reg)) {
	        puts("Failure of ioctl command IXPCI_WRITE_REG.");
        	close(fd_dev);
	        return FAILURE;
        }

	if(reg.value & 0x1)
	        printf("Set  PortA DO.");
	else
		printf("Set  PortA DI.");

	if(reg.value & 0x2)
	        printf(" PortB DO.");
	else
		printf(" PortB DI.");

	if(reg.value & 0x4)
	        printf(" PortC DO.");
	else
		printf(" PortC DI.");

	if(reg.value & 0x8)
	        printf(" PortD DO.\n");
	else
		printf(" PortD DI.\n");

	//write DO value ro portA
	reg.id = IXPCI_DIOPA;
	reg.value = 0xf;
	if (ioctl(fd_dev, IXPCI_WRITE_REG, &reg)) {
                puts("Failure of ioctl command IXPCI_WRITE_REG.");
                close(fd_dev);
                return FAILURE;
        }

	usleep(10);

	//read DI value from portB
	reg.id = IXPCI_DIOPB;
	if (ioctl(fd_dev, IXPCI_READ_REG, &reg)) {
                puts("Failure of ioctl command IXPCI_READ_REG IXPCI_DIOPB.");
                close(fd_dev);
                return FAILURE;
        }	

	printf("DI value is 0x%x\n",reg.value);

	//read DI FIFO status
/*	reg.id = IXPCI_DI_FS;
	if (ioctl(fd_dev, IXPCI_READ_REG, &reg)) {
                puts("Failure of ioctl command IXPCI_READ_REG IXPCI_DI_FS.");
                close(fd_dev);
                return FAILURE;
        }	
	
	printf("FIFO status is 0x%x\n",reg.value);
*/
	close(fd_dev);
	return SUCCESS;
}
