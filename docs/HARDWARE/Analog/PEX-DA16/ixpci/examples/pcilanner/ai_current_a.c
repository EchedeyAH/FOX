/* Example of analog input for Lanner OEM I/O.

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
	float current;
	WORD ret;

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

	/* Set Gain = 2, Ch = 0 */
	channel = 0;

	PCI_LANNER_Set_Current_Gain_MUX(fd, channel);
	
	printf("Press <enter> for next, ESC to exit.");
        /* read AI Channel 0, get out if error or ESC pressed */
        while (getchar() != 27)
	{
		ret = PCI_LANNER_Read_CalCurrent(fd, channel, &current);

		if(ret)
                        printf("Analog Input Error\n");
		else
			printf("Calibrated Current : %f mA\n", current);

		ret = PCI_LANNER_Read_Current(fd, channel, &current);

		if(ret)
                        printf("Analog Input Error\n");
		else
			printf("Uncalibrated Current : %f mA\n", current);
	}

	/* close device */
        PCIDA_Close(fd);

        puts("End of program");

        return SUCCESS;
}

