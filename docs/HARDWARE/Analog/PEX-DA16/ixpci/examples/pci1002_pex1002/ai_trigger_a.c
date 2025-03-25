/* Read ADC result by external trigger and interrupt.

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

   v 0.0.0 25 Oct 2019 by Winson Chen
     Create */

#include "pcidio.h"
int fd;

void sig_handler(int sig)
{
	WORD value;

	PCIDA_ReadAI(fd, EXTERNAL_TRIGGER, &value);
        printf("%x ", value);
}

int main()
{
	char *dev_file;

	dev_file = "/dev/ixpci1";

	/* open device file */
	fd = PCIDA_Open(dev_file);
	if (fd < 0)
	{
		printf("Can't open device\n");
		return FAILURE;
	}

	//Device Init.
	PCIDA_DriverInit(fd);

	//Instll Interrupt
	PCIDA_IntInstall(fd, sig_handler, SIGUSR2);

	/* channel 0 */
	PCIDA_SetAIChannel(fd ,0);
	
	/* gain 1 */
	PCIDA_SetGain(fd, 0);

	/* external trigger, interrput */
	PCIDA_SetTriggerType(fd, EXTERNAL_TRIGGER);

	/* enable board interrupt */
	PCIDA_IRQEnable(fd);

	/* wait for exit */
	puts("Press Enter key to stop.");
	while (getchar() != '\n');

	PCIDA_Close(fd);
	PCIDA_IntRemove(fd);
	puts("\nEnd of program.");
	return SUCCESS;
}
