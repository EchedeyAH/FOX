/* Example of Digital I/O for PCI-1202

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

   v 0.1.0 25 Oct 2001 by Reed Lai
     Change all "ixpci" to "ixpci."

   v 0.0.0 14 May 2001 by Reed Lai
     create, blah blah... */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>

#include "ixpci.h"

#define SRAM_M512_SIZE  0x80000  // PCI-M512 512KB (512 * 1024)

int main()
{
	int fd_dev;
	ixpci_reg_t rdi;
	char *dev_file;
	unsigned int i, read_count = 0, error = 0;
	
	dev_file = "/dev/ixpci1";
		/* change this for your device entry */

	/* open device */
	fd_dev = open(dev_file, O_RDWR);

	if (fd_dev < 0) {
		printf("Cannot open device file \"%s.\"\n", dev_file);
		return FAILURE;
	}

	rdi.id = IXPCI_SRAM512;

        for(i=0x0;i<SRAM_M512_SIZE;i++)
	{

		rdi.sram_off = i;
                if (ioctl(fd_dev, IXPCI_READ_REG, &rdi)) {
                        puts("Failure of ioctl command IXPCI_WRITE_REG: IXPCI_SRAM512.");
                        close(fd_dev);
                        return FAILURE;
                }

		 if ( i % 4 == 0)
                        printf("\n");

                printf("read %02x from SARM \t",rdi.value);

		read_count++;
		if ( rdi.value != ((i+2)&0xff) ) error++;

        }

	printf("\n\nread count : %d --> compare error : %d\n",read_count,error);

	close(fd_dev);
	return SUCCESS;
}
