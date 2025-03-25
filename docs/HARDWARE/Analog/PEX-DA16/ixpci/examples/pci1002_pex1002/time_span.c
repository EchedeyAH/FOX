/* Example of IO command IXPCI_TIME_SPAN for PCI-1002

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

   v 0.0.0 17 Jan 2003 by Reed Lai
     create, blah blah... */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

#include "ixpci.h"


int main()
{
	int fd_dev, span;
	char *dev_file;
	struct timeval before,after;
	
	dev_file = "/dev/ixpci1";
		/* change this for your device entry */

	span = 6000;
		/* span for 6000 us */

	/* open device */
	fd_dev = open(dev_file, O_RDWR);
	if (fd_dev < 0) {
		printf("Cannot open device file \"%s.\"\n", dev_file);
		return FAILURE;
	}
	gettimeofday(&before,NULL);
	if (ioctl(fd_dev, IXPCI_TIME_SPAN, span)) {
		printf("Failure of spanning %dus.\n", span);
		close(fd_dev);
		return FAILURE;
	}
	gettimeofday(&after,NULL);
	printf("Success of spanning %dus.\n", span);
	printf("%ld - %ld = %ldus.\n", after.tv_usec,before.tv_usec,after.tv_usec-before.tv_usec);
	close(fd_dev);
	return SUCCESS;
}
