/* Example of Card ID for PIO-D48U/D48SU and PEX-D48.

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

   This example shows the digital output and input by basic register
   read/write.  You can connect the CON1 and CON2 with a 20-pin cable
   to examine the output.

   v 0.0.0  18 Ocy 2019 by Winson Chen
   Note: The Card ID function is only supported by the PIO-D48U/D48SU and PEX-D48 (Ver. 1.0 or above)
*/

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
	ixpio_reg_t cid;

	dev_file = "/dev/ixpio1";

	/* open device file */
	fd = open(dev_file, O_RDWR);
	if (fd < 0) {
		printf("Failure of open device file \"%s.\"\n", dev_file);
		return FAILURE;
	}

	cid.id = IXPIO_CID;
	cid.value = 0;
					
	if (ioctl(fd, IXPIO_READ_REG, &cid)) {
		close(fd);
		puts("Failure of Read Card ID.");
	}

	printf("CARD ID is 0x%02x \n",cid.value);

	close(fd);
	return SUCCESS;
}
