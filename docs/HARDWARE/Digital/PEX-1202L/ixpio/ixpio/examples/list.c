/* List PIO devices.

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

   v 0.0.0 5 Oct 2000 by Reed Lai
     create, blah blah... */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>

#include "ixpio.h"

#define LINE_SIZE  128
#define NAME_SIZE  16

int main()
{
	FILE *fd_proc;
	char dev_file[NAME_SIZE + 1];
	char rbuf[LINE_SIZE + 1];
	int fd_dev;
	ixpio_devinfo_t dev;

	fd_proc = fopen(IXPIO_PROC_FILE, "r");
	if (!fd_proc) {
		printf("There is not PIO device found.\n");
		return FAILURE;
	}
	while (fgets(rbuf, LINE_SIZE, fd_proc)) {
		if (!strncmp(rbuf, "dev:", 4)) {

			/* make up available device file name */
			sprintf(dev_file, "/dev/%s", strtok(rbuf + 4, " "));
                        printf("dev_file : %s\n",dev_file);
			/* open device */
			fd_dev = open(dev_file, O_RDWR);
			if (fd_dev < 0) {
				printf("Cannot open device file \"%s.\"\n", dev_file);
				close(fd_dev);
				fclose(fd_proc);
				return FAILURE;
			}

			/* get device information */
			if (ioctl(fd_dev, IXPIO_GET_INFO, &dev)) {
				puts("Failure of getting device information.\n");
				close(fd_dev);
				fclose(fd_proc);
				return FAILURE;
			}

			printf("%s csid=0x%x irq=%u base=0x%lx name=%s no=%u\n",
				   dev_file, dev.csid, dev.irq, dev.base, dev.name,
				   dev.no);

			close(fd_dev);
		}
	}
	
	fclose(fd_proc);
	return SUCCESS;
}
