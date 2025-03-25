/* Example of Card ID for PISO-813U .

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
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. 

   v 0.0.0  18 Oct 2019 by Winson Chen
   Notice, The Card ID is only supported by the PISO-813U (Ver. 1.0 or above)
*/

#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "ixpio.h"

int main()
{
        int fd;
        char *dev_file;
        ixpio_reg_t reg;

        dev_file = "/dev/ixpio1";

        /* open device file */
	fd = open(dev_file, O_RDWR);
        if (fd < 0) {
                printf("Failure of open device file \"%s.\"\n", dev_file);
                return FAILURE;
        }

        reg.id = IXPIO_CID;
	if (ioctl(fd, IXPIO_READ_REG, &reg)) {
		puts("Failure of Read Card ID.");
		close(fd);
		return FAILURE;
	}

	printf("PISO-813U Card ID is %d\n",reg.value);

        close(fd);
        return SUCCESS;
}

