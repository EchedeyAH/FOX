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
 
   v 0.0.0 27 Sep 2013 by Winson chen
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
	ixpci_reg_t gid;

	dev_file = "/dev/ixpci1";
		/* change this for your deivce entry */

	fd = open(dev_file, O_RDWR);
	if(fd < 0){
		printf("Connot open device file \"%s.\"\n",dev_file);
		return FAILURE;
	}
	
	gid.id = IXPCI_GCID;

	if(ioctl(fd, IXPCI_READ_REG, &gid)){	/* Get CardID */
		puts("Failure of ioctl command IXPI_READ_REG: IXPCI_GCID.");
		close(fd);
		return FAILURE;
	}
	printf("CardID is %d\n",gid.value);

	close(fd);
	return SUCCESS;
}























