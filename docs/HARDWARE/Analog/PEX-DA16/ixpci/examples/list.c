/* List PCI DAQ devices

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

   v 0.2.0 25 Jun 2003 by Reed Lai
     Use IXPCI_PROC_FILE.

   v 0.1.0 25 Oct 2001 by Reed Lai
     Change all "ixpci" to "ixpci."

   v 0.0.0 3 May 2001 by Reed Lai
     create, blah blah... */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>

#include "ixpci.h"

#define LINE_SIZE  128
#define NAME_SIZE  32

int main()
{
	FILE *fd_proc;
	char dev_file[NAME_SIZE + 1];
	char rbuf[LINE_SIZE + 1];
	int fd_dev;
	ixpci_devinfo_t dev;

	fd_proc = fopen(IXPCI_PROC_FILE, "r");
	if (!fd_proc) {
		printf("There is not PCI DAQ device found.\n");
		return FAILURE;
	}
	while (fgets(rbuf, LINE_SIZE, fd_proc)) {
		if (!strncmp(rbuf, "dev:", 4)) {

			/* make up available device file name */
			sprintf(dev_file, "/dev/%s", strtok(rbuf + 4, " "));

			/* open device */
			fd_dev = open(dev_file, O_RDWR);
			if (fd_dev < 0) {
				printf("Cannot open device file \"%s.\"\n", dev_file);
				close(fd_dev);
				fclose(fd_proc);
				return FAILURE;
			}

			/* get device information */
			if (ioctl(fd_dev, IXPCI_GET_INFO, &dev)) {
				printf("Failure of ioctl command IXPCI_GET_INFO.\n");
				close(fd_dev);
				fclose(fd_proc);
				return FAILURE;
			}

			printf
				("%s id=0x%llx base=0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx name=%s no=%u\n",
				 dev_file, dev.id, dev.base[0], dev.base[1], dev.base[2],
				 dev.base[3], dev.base[4], dev.base[5], dev.name, dev.no);

			close(fd_dev);
		}
	}

	fclose(fd_proc);
	return SUCCESS;
}
