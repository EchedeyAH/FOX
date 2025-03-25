/* Example of I/O port for PISO-P8R8U/P16R16U.

   Author: Moki Matsushima

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

   This is an example for the DI performance.
   Connect DI[0:7]

   v 0.0.0  18 Nov 2009 by Golden Wang
     Create. */

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "ixpio.h"

unsigned long getjiffies(void);

unsigned long getjiffies()
{
        int fd;
        unsigned long sec,usec;
        char buf[64];

        fd = open("/proc/uptime",O_RDONLY);

        if (fd == -1)
        {
                perror("open");
                return 0;
        }

        memset(buf,0,64);
        read(fd,buf,64);
        sscanf(buf,"%lu.%lu",&sec,&usec);
        close(fd);

        //the number of milliseconds since system was started
        return ((sec * 100 + usec)*10);
}

int main()
{
	int fd,i;
	char *dev_file;
	unsigned long datacount = 5000;
	unsigned long start, totalms, count = 0, idx = 0;
	ixpio_reg_t d_o, d_i;

	dev_file = "/dev/ixpio1";

	/* open device file */
	fd = open(dev_file, O_RDWR);
	if (fd < 0) {
		printf("Failure of open device file \"%s.\"\n", dev_file);
		return FAILURE;
	}

	d_i.id = IXPIO_DIO_A;

	start = getjiffies();
	
	while(1)
	{
		if(getjiffies()-start >= 1000)
			break;

		if(ioctl(fd, IXPIO_READ_REG, &d_i)) 
		{
			puts("Failure of IXPIO_READ_REG: DIO.");
			break;
		}	
	

		if ( !idx )
		{
			if ( d_i.value )
				idx = 1;
		}

		if ( !d_i.value )
		{
			if (idx)
			{
				idx = 0;
				count++;
			}
		}

		//printf("DI[0:7]=0x%02x\n", d_i.value);
	}

        printf("channel polling %ld Hz\n\n", count);

	close(fd);
	return SUCCESS;
}
