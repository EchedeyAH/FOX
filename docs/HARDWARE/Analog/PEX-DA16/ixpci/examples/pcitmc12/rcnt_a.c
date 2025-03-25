/* Example of interrupt handle for PCI-TMC12

   Author: Reed lai

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

   J26 selects CLOCK2 for 80K
               3
             +---+
             |   |
        800K | o |
             |+-+|
      CLOCK2 ||o||
             || ||
         80K ||o||
             |+-+|
             +---+
              J26

   J3 feeds CLOCK2 to CLK12
            1 2
          +-----+
       J3 |  +-+|
          | o|o||
          |  | ||
          | o|o||
          |  +-+|
          | o o |
          |     |
          +-----+

   v 0.0.0 18 Mar 2021 by Reed Lai
     Hello world */

/* *INDENT-OFF* */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <signal.h>

#include "ixpci.h"
#include "pcidaq.h"

DWORD dwBoardNo = 0;

int main()
{
        WORD wBoards;
	DWORD rs = 0;
	DWORD cnt_data = 0;
	int i;

	rs = PTMC12_DriverInit(&wBoards);

	if(rs)
	{
		printf("Error Code = %d !!!\n", rs);
		return FAILURE;
	}

	printf("Total PCI card = %d\n", wBoards);

	rs = PTMC12_DetectBoards();

	printf("Total PCI-TMC12 = %d\n", rs);

	dwBoardNo = PTMC12_DetectBoardNo();

	printf("PCI-TMC12 BoardNo= %d\n", dwBoardNo);
        /* open device file */
        rs = PTMC12_OpenBoard(dwBoardNo, 1);

        if (rs)
        {
		printf("Can't open device\n");
		return FAILURE;
	}

	rs = PTMC12_WriteCounter(dwBoardNo, 1, 0, 0x1F40);	//Counter 1, mode 0, data "0x1F40"

	if(rs)
	{
        	PTMC12_CloseBoard(dwBoardNo);
		return FAILURE;
	}
	

	while (getchar() != 27) 
	{
		for (i=0; i<10; i++) 
		{
			PTMC12_ReadCounter(dwBoardNo, 1, &cnt_data);
			//delay_one_ms();
			printf("CNT data = %d\n", cnt_data);
		}
	}

	/* close device */
        PTMC12_CloseBoard(dwBoardNo);
        puts("End of program");

        return SUCCESS;
}

