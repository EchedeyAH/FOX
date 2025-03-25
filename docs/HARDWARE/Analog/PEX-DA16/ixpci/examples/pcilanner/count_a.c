/* Example of Counter for Lanner OEM I/O

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

   v 0.0.0 12 May 2010 by Golden Wang
     create, blah blah... */

#include "pcidio.h"

int main()
{
	int fd;
        char *dev_file;
        DWORD count_value;

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

	printf("Press <enter> for next, ESC to exit.");
	
	/* read Count0, get out if error or ESC pressed */
	while (getchar() != 27)
	{
		if(PCI_LANNER_Read_Count(fd, COUNTER0, &count_value))
                        return FAILURE;

                printf("Counter%d : %d\n", COUNTER0, count_value);


		if(PCI_LANNER_Read_Count(fd, COUNTER1, &count_value))
			return FAILURE;

		printf("Counter%d : %d\n", COUNTER1, count_value);
	}

	/* clear counter0 */
	if(PCI_LANNER_Clear_Count(fd, COUNTER0))
                return FAILURE;

	if(PCI_LANNER_Clear_Count(fd, COUNTER1))
		return FAILURE;

        /* close device */
        PCIDA_Close(fd);

        puts("End of program");

        return SUCCESS;
}
