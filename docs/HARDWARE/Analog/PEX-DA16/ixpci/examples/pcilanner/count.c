/* Example of Counter for Lanner OEM I/O.

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

   This example shows the analog output by basic register read/write.

   v 0.0.0 19 May 2010 by Golden Wang
     create, blah blah... */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "ixpci.h"

typedef unsigned short WORD;
typedef char BYTE;
WORD PCIDA_OutputByte(WORD, ixpci_reg_t *);
WORD PCIDA_InputByte(WORD, ixpci_reg_t *);

int main()
{
	int fd;
	char *dev_file;
	ixpci_reg_t reg;

	dev_file = "/dev/ixpci1";

	/* open device file */
	fd = open(dev_file, O_RDWR);

	if (fd < 0)
	{
		printf("Failure of open device file \"%s.\"\n", dev_file);
		return FAILURE;
	}

	printf("Press <enter> for next, ESC to exit.");
	/* read Count0, get out if error or ESC pressed */
	while (getchar() != 27)
	{
		reg.id = IXPCI_READ_C0;
		PCIDA_InputByte(fd, &reg);

		printf("Counter0 : %d\n",reg.value);
	}  

	/* clear counter0 */
	reg.id = IXPCI_CLEAR_C0;
	PCIDA_OutputByte(fd, &reg);

	close(fd);
	puts("\nEnd of program.");
	return SUCCESS;
}

WORD PCIDA_OutputByte(WORD fd, ixpci_reg_t *p_reg)
{
	if (ioctl(fd, IXPCI_WRITE_REG, p_reg))
	{
		close(fd);
		return FAILURE;
	}

	return SUCCESS;
}

WORD PCIDA_InputByte(WORD fd, ixpci_reg_t *p_reg)
{
	if (ioctl(fd, IXPCI_READ_REG, p_reg))
	{
		close(fd);
		return FAILURE;
	}

	return SUCCESS;
}

