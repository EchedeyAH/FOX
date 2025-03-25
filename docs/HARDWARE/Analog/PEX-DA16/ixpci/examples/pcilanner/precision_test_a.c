/* Example of Voltage precision test for Lanner OEM I/O.

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

unsigned long getjiffies()
{
        int fd, rs;
        unsigned long sec,usec;
        char buf[64];

        fd = open("/proc/uptime",O_RDONLY);

        if (fd == -1)
        {
                perror("open");
                return 0;
        }
        memset(buf,0,64);
        rs = read(fd,buf,64);
        sscanf(buf,"%lu.%lu",&sec,&usec);
        close(fd);
        
        //the number of milliseconds since system was started
        return ((sec * 100 + usec)*10);
}

int main()
{
	int fd, i, first = 1;
        char *dev_file;
        DWORD gain, channel;
	unsigned long stime, etime;
	WORD ai_hex, max_aihex = 0, min_aihex = 0;

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

	PCI_LANNER_Set_Voltage_Gain_MUX(fd, channel, gain);
	
	stime = getjiffies();
	while (1)
	{
		PCI_LANNER_ReadAI_Hex(fd, &ai_hex);

//		printf("AI Voltage : %f\n", ((ai_hex*10)/(float)2047));
		
		if(first)
		{
			max_aihex = min_aihex = ai_hex;
			first = 0;
		}
		else
		{
			if ( ai_hex > max_aihex )
			{
				max_aihex = ai_hex;
			}

			if ( ai_hex < min_aihex)
			{
				min_aihex = ai_hex;
			}
		}

		if( getjiffies() - stime > 600000 )	// 10 min
		{ 
			break;
		}

		printf("AI Hex Value : 0x%03x -- MAX : 0x%03x -- Min : 0x%03x\n",ai_hex, max_aihex, min_aihex);
	}

	/* close device */
        PCIDA_Close(fd);

	printf("Get AI Hex Value : MAX : 0x%03x -- MIN : 0x%03x\n",max_aihex, min_aihex);
        puts("End of program");

        return SUCCESS;
}

