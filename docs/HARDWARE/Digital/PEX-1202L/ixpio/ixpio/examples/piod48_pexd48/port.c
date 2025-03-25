/* Example of I/O port for PIO-D48.

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

   v 0.1.0 29 Apr 2002 by Reed Lai
     Rename PIO to IXPIO.

   v 0.0.0 30 Nov 2000 by Reed Lai
     create, blah blah... */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "ixpio.h"

int main()
{
	int fd;
	char *dev_file;
	ixpio_reg_t reg, reg2;
	unsigned int old = -1;

	dev_file = "/dev/ixpio1";

	/* open device file */
	fd = open(dev_file, O_RDWR);
	if (fd < 0) {
		printf("Failure of open device file \"%s.\"\n", dev_file);
		return FAILURE;
	}

	/* setup 8255-1 control word */
	reg.id = IXPIO_82551CW;
	reg.value = 0x90;			/* CON1 PA as input port */
	if (ioctl(fd, IXPIO_WRITE_REG, &reg)) {
		printf("Failure of ioctl command IXPIO_WRITE_REG IXPIO_82551CW.\n");
		close(fd);
		return FAILURE;
	}

	/* Enable CON1 port PB DO channel */
	reg.id = IXPIO_82551PB;
	reg.value = 255;
	ioctl(fd, IXPIO_WRITE_REG, &reg);


	/* read PA7 of port-0 */
	reg.id = IXPIO_82551PA;

	/* read, get out if error or PA7 = 1 */
	while (!ioctl(fd, IXPIO_READ_REG, &reg) && !(reg.value & 0x80)) {
		if (old != reg.value) {
			old = reg.value;
			printf("port 0/A = 0x%x\n", reg.value);
		}
	}

	printf("Close\n");
	close(fd);
	return SUCCESS;
}
