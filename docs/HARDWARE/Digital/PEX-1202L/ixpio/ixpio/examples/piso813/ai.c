/* Example of analog input for PISO-813.

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

   Analog input range -10V ~ +10V, giain x1

   JP1 : 20V

       +---+ 10V
       |   |
       | o |
       |+-+|
       ||o||
       || ||
       ||o||
       |+-+|
       +---+ 20V

   JP2 : Bipolar

       +---+ UNI
       |   |
       | o |
       |+-+|
       ||o||
       || ||
       ||o||
       |+-+|
       +---+ BI

   v 0.0.0  25 Nov 2002 by Reed Lai
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
	ixpio_reg_t pgcr, mcsr, ai;
	float av;

	dev_file = "/dev/ixpio1";

	/* open device file */
	fd = open(dev_file, O_RDWR);
	if (fd < 0) {
		printf("Failure of open device file \"%s.\"\n", dev_file);
		return FAILURE;
	}

	pgcr.id = IXPIO_PGCR;
	pgcr.value = 0; /* gain x1 */
	if (ioctl(fd, IXPIO_WRITE_REG, &pgcr)) {
		puts("Failure of writing PGA Gain Control Register.");
		close(fd);
		return FAILURE;
	}
	usleep(12); /* PGA + propagation settling delay */

	mcsr.id = IXPIO_MCSR;
	mcsr.value = 0; /* analog input channel 0 */
	if (ioctl(fd, IXPIO_WRITE_REG, &mcsr)) {
		puts("Failure of writing Multiplexer Channel Select Register.");
		close(fd);
		return FAILURE;
	}
	usleep(10); /* multiplexer settling delay */

	ai.id = IXPIO_AD;
	while (getchar() != 27) {
		if (ioctl(fd, IXPIO_READ_REG, &ai)) {
			puts("Failure of analog input.");
			close(fd);
			return FAILURE;
		}
		av = 20 * (float)ai.value / 0xfff - 10;
		printf("ADC read = 0x%04x ==> %fV\t ESC exit\n", ai.value, av);
	}

	close(fd);
	return SUCCESS;
}
