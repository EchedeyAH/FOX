/* Example of Analog I/O for PCIe-8622

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

   v 0.0.0 1 Oct 2015 by Winson Chen
     create, blah blah...

   v 0.01 21 Dec 2015 by Winson Chen
     Add calibration value.
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>

#include "ixpci.h"
#include "_pcie8622.h"

void calibration(ixpci_eep_t *eep)
{
        int index;

        //printf("AI_ch is %d\n",eep->AI_ch);

        for(index = 0; index < eep->AI_ch; index++)
        {
                //AI Bipolar 10.00V
                if(eep->read_eep[index*4+0] == 0)
                {
                        eep->CalAI[PCIE86XX_AI_BI_10V][index] = 1.0;
                        eep->CalAIShift[PCIE86XX_AI_BI_10V][index] = 0;
                }
                else
                {
                        eep->CalAI[PCIE86XX_AI_BI_10V][index] = (9.9 / (float)(eep->read_eep[index*4+0])) / (10.0 / (float)(0x7FFF));
                        //printf("eep->CalAI %f\n",eep->CalAI[PCIE86XX_AI_BI_10V][index]);
                        eep->CalAIShift[PCIE86XX_AI_BI_10V][index] = (float)((short)(eep->read_eep[index*4+1])) * (-1);
                        //printf("eep->CalAIShift %f\n",eep->CalAIShift[PCIE86XX_AI_BI_10V][index]);
                }

                //AI Bipolar 5.00V
                if(eep->read_eep[index*4+2] == 0)
                {
                        eep->CalAI[PCIE86XX_AI_BI_5V][index] = 1.0;
                        eep->CalAIShift[PCIE86XX_AI_BI_5V][index] = 0;
                }
                else
                {
                        eep->CalAI[PCIE86XX_AI_BI_5V][index] = (4.9 / (float)(eep->read_eep[index*4+2])) / (5.0 / (float)(0x7FFF));
                        eep->CalAIShift[PCIE86XX_AI_BI_5V][index] = (float)((short)(eep->read_eep[index*4+3])) * (-1);
                }

        }

}

int main(int argc, char **argv)
{

	if(argc != 4)
	{
		printf("Polling specific AI channel\n");
		printf("Example: ./plling_ai 0 1 10\n");
		printf("Select channel 0, mode 1, polling 10 times\n");
		printf("(mode 0 means -5V~+5V, mode 1 means -10V~+10V)\n");
		return 0;
	}

	int fd_dev;
	ixpci_reg_t reg;
	ixpci_eep_t eep;
	char *dev_file, polling_ch, mode, ai_range, ai_config, i, index;
	unsigned int timeout = 0, times;

	polling_ch = atoi(argv[1]);
	mode = atoi(argv[2]);
	times = atoi(argv[3]);

	float ai_value[times*16];

	switch(mode)
	{
		case 0:
		  mode = 0x0;
		  ai_range = (float)5.0;
		  ai_config = PCIE86XX_AI_BI_5V;
		  break;
		case 1:
		  mode = 0x80;
		  ai_range = (float)10.0;
		  ai_config = PCIE86XX_AI_BI_10V;
		  break;
		default:
		  printf("Wrong mode.\n");
		  return 0;
	}

	dev_file = "/dev/ixpci1";
		/* change this for your device entry */

	/* open device */
	fd_dev = open(dev_file, O_RDWR);
	if (fd_dev < 0) {
		printf("Cannot open device file \"%s.\"\n", dev_file);
		return FAILURE;
	}

	/* Read EEPROM calibration value */
	if(ioctl(fd_dev, IXPCI_READ_EEP, &eep)){
		puts("Failure of ioctl command IXPCI_READ_EEP");
	}
	calibration(&eep);

	/* Clear FIFO 0x04 */
	reg.id = IXPCI_AI_CF;
	reg.value = 0;
	if (ioctl(fd_dev, IXPCI_WRITE_REG, &reg)){
                puts("Failure of ioctl command IXPCI_WRITE_REG: IXPCI_AI_CF.");
        }
	usleep(1);

	/* AI Scan Mode Control/Status 0x0c  */
	reg.id = IXPCI_AIGR;
	reg.value = mode;  //0x0: +5V~-5V  0x80: +10V~-10V

	if (ioctl(fd_dev, IXPCI_WRITE_REG, &reg)){
                puts("Failure of ioctl command IXPCI_WRITE_REG: IXPCI_AIGR.");
        }
	usleep(1);
	printf("Analog Input Range :-%dV~+%dV\n",ai_range,ai_range);

	for(index = 0; index < times; index++)
	{
		/* Clear FIFO  0x04 */
		reg.id = IXPCI_AI_CF;
		reg.value = 0;
		if (ioctl(fd_dev, IXPCI_WRITE_REG, &reg)){
			puts("Failure of ioctl command IXPCI_WRITE_REG: IXPCI_AI_CF.");
		}
		usleep(1);

		/* Software trigger 0x10 */
		reg.id = IXPCI_ADST;
		reg.value = 0x0;
		if (ioctl(fd_dev, IXPCI_WRITE_REG, &reg)){
			puts("Failure of ioctl command IXPCI_WRITE_REG: IXPCI_ADST.");
		}
		usleep(1);

		/* Read ADC status 0x0c */
		reg.id = IXPCI_AIGR;
	       	for(;;)
		{
			if (ioctl(fd_dev, IXPCI_READ_REG, &reg)){
				puts("Failure of ioctl command IXPCI_READ_REG: IXPCI_AIGR.");
			}

			reg.value = reg.value & 0x200;
			if(!reg.value)  //ready
				break;

			if(timeout > 100000)
				break;

			timeout++;
		}

		for(i = 0; i < eep.AI_ch; i++)
		{
			if(i < 8)
			{
				/* Read AI data 0x10 */
				reg.id = IXPCI_AI_L;
				if (ioctl(fd_dev, IXPCI_READ_REG, &reg)){
        		        	puts("Failure of ioctl command IXPCI_READ_REG: IXPCI_AI.");
		        	}
			}
			else
			{
				/* Read AI data 0x14 */
				reg.id = IXPCI_AI_H;
        	                if (ioctl(fd_dev, IXPCI_READ_REG, &reg)){
                	                puts("Failure of ioctl command IXPCI_READ_REG: IXPCI_AI.");
                        	}
			}
		
			if(i == polling_ch)
			{
				ai_value[index] = ((float)((short)reg.value * eep.CalAI[ai_config][i] + eep.CalAIShift[ai_config][i]) / 32768.0 * ai_range);
				printf("%d.CH[%d] %f\n",index+1,i,ai_value[index]);
			}
		}
	}
	close(fd_dev);
	return SUCCESS;
}
