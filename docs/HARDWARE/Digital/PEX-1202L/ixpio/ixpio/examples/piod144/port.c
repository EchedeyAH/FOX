/* Example of I/O port for PIO-D144.

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

   v 0.2.0  8 Jul 2003 by Reed Lai
     Use IXPIO_8DR instead of IXPIO_RW8DR.

   v 0.1.0 29 Apr 2002 by Reed Lai
     Rename PIO to IXPIO.

   v 0.0.0 5 Oct 2000 by Reed Lai
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
	ixpio_reg_t reg;
	unsigned int old = -1;

	dev_file = "/dev/ixpio1";

	/* open device file */
	fd = open(dev_file, O_RDWR);
	if (fd < 0) {
		printf("Failure of open device file \"%s.\"\n", dev_file);
		return FAILURE;
	}

	/* select io port */
	reg.id = IXPIO_AIOPCR;
	reg.value = 0x02;			/* port C/2 */
	if (ioctl(fd, IXPIO_WRITE_REG, &reg)) {
		printf("Failure of ioctl command IXPIO_WRITE_REG IXPIO_AIOPCR.\n");
		close(fd);
		return FAILURE;
	}

	/* read port C/2 */
	reg.id = IXPIO_8DR;

	/* read, get out if error or port C/2 bit 7 = 0 */
	while (!ioctl(fd, IXPIO_READ_REG, &reg) && (reg.value & 0x80)) {
		if (old != reg.value) {
			old = reg.value;
			printf("port C/2 = 0x%x\n", reg.value);
		}
	}

	close(fd);
	return SUCCESS;
}
