/* Example of interrupt handle for PCI-1002 with static library

   Author: Winson Chen

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

   This program shows the basic interrupt operation.

   v 0.0.0 1 Nov 2019 by Winson Chen
     create, blah blah... */

#include "pcidio.h"

void sig_handler(int sig)
{
	static unsigned int sig_counter;
	static unsigned int i;

	++sig_counter;
	++i;

	if (i > 9) {
		printf("\rGot single %d for %u time(s) ", sig, sig_counter);
		i = 0;
	}
}

int main()
{
	int fd;
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

	PCIDA_SetTriggerType(fd, PACER_TRIGGER);

	/* configure the 8254 counter 0 */
        PCIDA_8254Control(fd, 0x36);    //0x36 counter-0, mode 3

	PCIDA_8254C0(fd, 0xff, 0xff);	//high byte, low byte

	/* enable board interrupt */
	PCIDA_IRQEnable(fd);

	/* wait for exit */
	puts("Press Enter key to exit");
	while (getchar() != 10);

	PCIDA_Close(fd);
	PCIDA_IntRemove(fd);
	puts("\nEnd of program.");
	return SUCCESS;
}
