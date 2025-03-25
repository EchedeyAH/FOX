/* Example of Analog I/O port for PCI-1800 with PCI Static Libary.

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

   This example uses CN0 to examine the Analog I/O.

   v 0.0.0 1 Aug 2015 by Winson Chen
     create */


#include "pcidio.h"
int main()
{
        int fd;
        char *dev_file;
        float fAdVals[10];
	float vol;

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

	vol = 5;
        if (PCI_180X_DaF_Output(fd, 0, vol)) return FAILURE;

        printf("Analog output = %1.1f\n",vol);

        // +/- 10V range for PCI-1602
        PCI_180X_SetChannelConfig(fd, 0, 0x0);
        PCI_180X_DelayUs(fd, 3);
        // +/- 10V range for PCI-1602
        // PCI_180X_SetChannelConfig(fd, 0, 0x10);
        // PCI_180X_DelayUs(fd, 23);
        PCI_1602_AdsPolling(fd, fAdVals, 5);
        
        printf("Ch:0 PCI_1602_AdsPolling --> 0:[%5.3f] 1:[%5.3f] 2:[%5.3f] 3:[%5.3f] 4:[%5.3f]\n", fAdVals[0], fAdVals[1], fAdVals[2], fAdVals[3], fAdVals[4]);
	/* close device */
        PCIDA_Close(fd);

        puts("End of program");

        return SUCCESS;
}

