/* Example of Digital I/O for PCI-FC16

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
	ixpci_reg_t gjs;
	
	dev_file = "/dev/ixpci1";

	fd = open(dev_file, O_RDWR);
	if (fd < 0){
		printf("Cannot open device file \"%s.\"\n",dev_file);
		return FAILURE;
	}
	gjs.id = IXPCI_GDIO_JS;

	if(ioctl(fd, IXPCI_READ_REG, &gjs)){
		puts("Failure of ioctl command IXPCI_READ_REG: IXPCI_DI.");
		close(fd);
		return FAILURE;
	}
	printf("Jumper Status is %d\n",gjs.value);

	close(fd);
	return SUCCESS;
}
