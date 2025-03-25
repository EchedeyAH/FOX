/* Example of analog polling for Lanner OEM I/O.

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

#include "pcidio.h"

int main()
{
	int fd, i;
        char *dev_file;
        DWORD gain, channel;
	unsigned int datacount = 10;
        float *voltage = malloc(sizeof(float)*datacount);

        dev_file = "/dev/ixpci1";

        /* open device file */
        fd = PCIDA_Open(dev_file);

        if (fd < 0)
        {
          printf("Can't open device\n");
          return FAILURE;
        }

        /* init device */
        if (PCIDA_DriverInit(fd)) return FAILURE;

	/* Set Gain = 0, Ch = 0 */
	channel = 0;
	gain = 0;
	
	if(PCI_LANNER_Read_Voltage_Polling(fd, channel, gain, datacount, voltage))
	{
		return FAILURE;
	}
	else
	{
		for(i=0;i<datacount;i++)
	        {
			printf("voltage[%d] : %f V\n",i, voltage[i]);
	        }
	}

	/* close device */
        PCIDA_Close(fd);

        puts("End of program");

        return SUCCESS;
}

