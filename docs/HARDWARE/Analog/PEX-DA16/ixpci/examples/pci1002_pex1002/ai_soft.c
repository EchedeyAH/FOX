/* Read ADC result by software trigger.

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

   This program shows the AI by software trigger.

   v 0.0.0 21 Jan 2003 by Reed Lai
     Create */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "ixpci.h"

int main()
{
	int fd, count;
	char *dev_file;
	ixpci_reg_t reg, rai;

	dev_file = "/dev/ixpci1";

	/* open device file */
	fd = open(dev_file, O_RDWR);
	if (fd < 0) {
		printf("Failure of open device file \"%s.\"\n", dev_file);
		return FAILURE;
	}

	/* gain 1 */
	reg.id = IXPCI_AIGR;
	reg.value = 0;
	if (ioctl(fd, IXPCI_WRITE_REG, &reg)) {
		puts("Failure of gain control.");
		close(fd);
		return FAILURE;
	}

	/* software trigger mode */
	reg.id = IXPCI_CR;
	reg.value = 0;
	if (ioctl(fd, IXPCI_WRITE_REG, &reg)) {
		puts("Failure of software trigger mode.");
		close(fd);
		return FAILURE;
	}


	puts("Enter to continue, ESC to exit.");
	while (getchar() != 27) {

	for(count = 0; count < 2; count++)
	{
	
	/* set channel */
	reg.id = IXPCI_AICR;
	reg.value = count;
	if (ioctl(fd, IXPCI_WRITE_REG, &reg)) {
		puts("Failure of software trigger mode.");
		close(fd);
		return FAILURE;
	}

	usleep(23);	
	/* delay 23us for channel switching */

	/* read 16 channels by software trigger */
	rai.id = IXPCI_AI;
	rai.mode = IXPCI_RM_TRIGGER;

			if (ioctl(fd, IXPCI_READ_REG, &rai)) {
				puts("Failure of analog input.");
				close(fd);
				return FAILURE;
			}

			usleep(23);

			rai.mode = IXPCI_RM_NORMAL;

				if (ioctl(fd, IXPCI_READ_REG, &rai)) {
                                puts("Failure of analog input.");
                                close(fd);
                                return FAILURE;
                        }

			printf("channel %d value %x ",count, rai.value);

			usleep(23);
	}
	}

	close(fd);
	return SUCCESS;
}
