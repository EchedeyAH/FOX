/* Example of analog output for PIO-DA16/DA8/DA4.

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

   v 0.0.0 18 Nov 2009 by Reed Lai
     create, blah blah... */

#include <stdio.h>
#include <memory.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "ixpio.h"

#define DA_CHANNEL_0 0x00;  /* channel 0 */
#define DA_CHANNEL_1 0x40;  /* channel 1 */
#define DA_CHANNEL_2 0x80;  /* channel 2 */
#define DA_CHANNEL_3 0xc0;  /* channel 3 */

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
	int fd, i,j,count = 0;
	char *dev_file;
	ixpio_reg_t cs, aol, aoh;
	unsigned long start;
	unsigned long value[8] = {0x1fff, 0x1fff, 0x1fff, 0x1fff, 0x1fff, 0x1fff, 0x1fff, 0x1fff};
	unsigned int ao_value;
	unsigned int channel[8] = {0x00,0x40,0x80,0xc0,0x00,0x40,0x80,0xc0};

	dev_file = "/dev/ixpio1";

	/* open device file */
	fd = open(dev_file, O_RDWR);
	if (fd < 0) {
		printf("Failure of open device file \"%s.\"\n", dev_file);
		return FAILURE;
	}

	aol.id = IXPIO_DAL;
	aoh.id = IXPIO_DAH;
	ao_value = 0x1fff;


        start = getjiffies();

        while(1)
        {
		if (getjiffies()-start >= 1000)
			break;

		for ( i = 0; i < 8; i++)
                {
			if ( i <= 3 )
			{
	                       	cs.id = IXPIO_DA0CS;
			}
			else if( i > 3 )
			{
				cs.id = IXPIO_DA1CS;
			}
			

                        aol.value = value[i] & 0xff;
                        aoh.value = (value[i] >> 8) | channel[i];

                        if (ioctl(fd, IXPIO_WRITE_REG, &aol) ||
                                ioctl(fd, IXPIO_WRITE_REG, &aoh) ||
                                ioctl(fd, IXPIO_WRITE_REG, &cs )) 
			{
                                close(fd);
                                puts("Failure of analog output.");
                        }

			//usleep(10);
                        if (value[i] == 0x1fff)
                                value[i] = 0x2fff;
                        else
                                value[i] = 0x1fff;

			//printf("channel : %d Analog output: 0x %04x \n", i,value[i]);

                }

		count++;
        }

	
/*
	for ( i = 0; i < 8; i++)
	{	
		if ( i > 3 )
                	cs.id = IXPIO_DA1CS;

		start = getjiffies();
				
		while(1)
		{
			if (getjiffies()-start >= 1000)
	                        break;

			aol.value = ao_value & 0xff;
			aoh.value = (ao_value >> 8) | channel[i];

			if (ioctl(fd, IXPIO_WRITE_REG, &aol) ||
				ioctl(fd, IXPIO_WRITE_REG, &aoh) ||
				ioctl(fd, IXPIO_WRITE_REG, &cs )) {
				close(fd);
				puts("Failure of analog output.");
			}


			if (ao_value == 0x1fff)
				ao_value = 0x2fff;
			else
				ao_value = 0x1fff;

		}
	}
*/

	close(fd);
	printf("\nEnd of program. %d Hz\n",(count/2));
	return SUCCESS;
}
