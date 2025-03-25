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

   J25 selects the interrupt source as CH12
            J25
              +-----+
              |     |
          CH3 | o o |
              |     |
          CH6 | o o |
              |     |
          CH9 | o o |
              |+---+|
         CH12 ||o o||
              |+---+|
          EXT | o o |
              |     |
      (SPARE) | o o |
              |     |
              +-----+

   then you can rocking roll now.

   v 0.0.0 18 Mar 2003 by Reed Lai
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

void sig_handler(int sig)
{
	static unsigned int sig_counter, dout, Data;

	if (dout == 0x8000 || dout == 0) 
		dout = 1;
	else 
		dout <<= 1;

	PTMC12_WriteWord(dwBoardNo, IXPCI_DO, dout);
	printf("\rGot single %d for %u time(s), DO = 0x%04x ", sig, ++sig_counter, dout);
}

int main()
{
        WORD wBoards;
	DWORD rs = 0;
	DWORD mydata = 0;
	int i;
	unsigned int xor_data = 0;

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

        xor_data = 0x1000;      //clk3
        //xor_data = 0; //clk3
        PTMC12_WriteXORLogic(dwBoardNo, xor_data);
        PTMC12_ReadXORLogic(dwBoardNo);

	rs = PTMC12_InstallCallBackFunc(dwBoardNo, 2,  sig_handler);

	if(rs)
	{
        	PTMC12_CloseBoard(dwBoardNo);
		PCIDA_RemoveAllCallBackFunc(dwBoardNo);
		return FAILURE;
	}

	rs = PTMC12_WriteCounter(dwBoardNo, 3, 3, 0x9c3f);	//counter3(1~12), mode3

	if(rs)
	{
        	PTMC12_CloseBoard(dwBoardNo);
		PCIDA_RemoveAllCallBackFunc(dwBoardNo);
		return FAILURE;
	}

	rs = PTMC12_EnableInt(dwBoardNo);

	if(rs)
	{
        	PTMC12_CloseBoard(dwBoardNo);
		PCIDA_RemoveAllCallBackFunc(dwBoardNo);
		return FAILURE;
	}

	/* wait for exit */
	puts("Press Enter key to exit");
	while (getchar() != 10);

	/* close device */
        PTMC12_CloseBoard(dwBoardNo);
	PTMC12_RemoveAllCallBackFunc(dwBoardNo);
        puts("End of program");

        return SUCCESS;
}

