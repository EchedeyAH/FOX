/* Example of Digital I/O for PCI-FC16

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
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>

#include "ixpci.h"

#define DIO_S0 0x04
#define DIO_S1_S2 0x01	//PA = DI, PB = DO

int main()
{
	int fd;
	ixpci_reg_t rdi, rdo, set;
	char *dev_file;

	dev_file = "/dev/ixpci1";
		/* change this for your deivce entry */

	fd = open(dev_file, O_RDWR);
	if(fd < 0){
		printf("Cannot open device file \"%s.\"\n",dev_file);
		return FAILURE;
	}

        set.id = IXPCI_GDIO_JS;

        if(ioctl(fd, IXPCI_READ_REG, &set)){
                puts("Failure of ioctl command IXPCI_READ_REG: IXPCI_GDIO_JS.");
                close(fd);
                return FAILURE;
        }

        if(set.value & DIO_S0 != 0x04)
        {
                puts("The jumper DIO-S0 isn't the jumper selectable mode.");
                close(fd);
                return FAILURE;
        }
	else
	{
		if(set.value & DIO_S1_S2 != 1) 
		{
                	puts("The jumper DIO-S1, DIO-S2 isn't the PA = DI or PB = DO.");
                	close(fd);
	                return FAILURE;
		}
		else
		{
        		printf("JP1(HW) : set PA(DI), PB(DO).\n");
		}
	}

	rdi.id = IXPCI_DIOPA;
	rdo.id = IXPCI_DIOPB;
	rdo.value = 1;
	
	puts("ESC and Enter to go out. Enter to continue.");
	while(getchar() != 27){
		if(ioctl(fd, IXPCI_WRITE_REG, &rdo)){ /* DO */
			puts("Failure of ioctl command IXPCI_WRITE_REG: IXPCI_DO.");
			close(fd);
			return FAILURE;
		}
		printf("DO = 0x%04x\t", rdo.value);

		if(ioctl(fd, IXPCI_READ_REG, &rdi)){  /* DI */
			puts("Failure of ioctl command IXPCI_READ_REG: IXPCI_DI.");
			close(fd);
			return FAILURE;
		}
		printf("DI = 0x%04x\n", rdi.value);

		rdo.value == 0x8000 ? rdo.value = 1 : (rdo.value <<= 1);
	}

	close(fd);
	return SUCCESS;
}
