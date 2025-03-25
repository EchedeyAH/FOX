/* Example of IO command IXPCI_RESET for PCI-1202

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

   This program reset board to
       general trigger mode
	   no interrupt source (disalble all interrupts).

   v 0.0.0 1 Nov 2019 by Winson Chen
     create, blah blah... */

#include "pcidio.h"

int main()
{
	int fd;
	char *dev_file;
	
	dev_file = "/dev/ixpci1";
	/* change this for your device entry */

	/* open device file */
	fd = PCIDA_Open(dev_file);
	if (fd < 0) {
		printf("Cannot open device file \"%s.\"\n", dev_file);
		return FAILURE;
	}

	//Device Init.
        PCIDA_DriverInit(fd);

	PCIDA_ResetDevice(fd);

	puts("Success of reseting device.");
	PCIDA_Close(fd);
	return SUCCESS;
}
