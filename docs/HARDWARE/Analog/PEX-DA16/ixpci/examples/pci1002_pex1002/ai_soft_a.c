/* Read ADC result by software trigger.

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

   This program shows the AI by software trigger.

   v 0.0.0 24 Sep 2019 by Winson Chen
     Create */

#include "pcidio.h"

int main()
{
	int fd, ch, i;
	char *dev_file;
	WORD value;

	dev_file = "/dev/ixpci1";

	/* open device file */
        fd = PCIDA_Open(dev_file);

        if (fd < 0)
        {
		printf("Can't open device\n");
		return FAILURE;
        }

	PCIDA_DriverInit(fd);

	/* gain 1 */
	PCIDA_SetGain(fd, 0);

	//set trigger mode as software trigger.
	PCIDA_SetTriggerType(fd, SOFTWARE_TRIGGER);

	ch = 0;
	puts("Enter to continue, ESC to exit.");
	while (getchar() != 27) {
		for (ch = 0; ch < 2; ++ch) {	//ch0

			PCIDA_SetAIChannel(fd, ch);
			PCIDA_ReadAI(fd, SOFTWARE_TRIGGER, &value);
			printf("%x ", value);
		}
	}

	PCIDA_Close(fd);
	return SUCCESS;
}
