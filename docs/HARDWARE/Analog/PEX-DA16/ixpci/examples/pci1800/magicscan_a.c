/* Example of magic scan for PCI-1800 with PCI Static Libary.

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

   This example uses CN1 to examine the digital I/O.

   v 0.0.0 1 May 2008 by Golden Wang
     create */


#include "pcidio.h"
int main()
{
        int fd;
        char *dev_file;
	WORD wSampleRateDiv=40; // The sampling rate of MagicScan is 8M/wSampleRateDiv
	WORD wRetVal,wStatus;
	DWORD dwLowAlarm,dwHighAlarm;
	DWORD dwI;
	DWORD DATALENGTH=100; // how many data to be scaned for each channel
	int  nPriority=2;	  //

	WORD   wV0[100];	 // AD ch:4 buffer
	WORD   wV1[100];	 // AD ch:3 buffer
	WORD   wV2[100];	 // AD ch:5 buffer
	WORD   old_wV0[10];  // AD ch:4 buffer for erasing wave form
	WORD   old_wV1[10];  // AD ch:3 buffer for erasing wave form
	WORD   old_wV2[10];  // AD ch:5 buffer for erasing wave form

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

	for (dwI=0; dwI<10; dwI++)
        {
	  old_wV0[dwI]=wV0[dwI];
	  old_wV1[dwI]=wV1[dwI];
	  old_wV2[dwI]=wV2[dwI];
        }

        for (dwI=0; dwI<DATALENGTH; dwI++) wV0[dwI]=wV1[dwI]=wV2[dwI]=0;

        wRetVal=PCI_180X_ClearScan(fd);

        wRetVal += PCI_180X_AddToScan(fd,0,1,1,0,0,0); // CH:4 to scan
        wRetVal += PCI_180X_SaveScan(fd,0,wV0);        // Notice: 0 not 4
        wRetVal += PCI_180X_AddToScan(fd,3,0,1,0,0,0); // CH:3 to scan
        wRetVal += PCI_180X_SaveScan(fd,1,wV1);        // Notice: 1 not 3
        wRetVal += PCI_180X_AddToScan(fd,5,0,1,0,0,0); // CH:5 to scan
        wRetVal += PCI_180X_SaveScan(fd,2,wV2);        // Notice: 2 not 5

        PCI_180X_StartScan(fd,wSampleRateDiv,DATALENGTH,nPriority); 

        for(; ;)
        {
	  PCI_180X_ReadScanStatus(&wStatus,&dwLowAlarm,&dwHighAlarm);
	  if( wStatus>1 )
	     break;
	  sleep(10);
        }
        PCI_180X_StopMagicScan(fd);
        
        for (dwI=0; dwI<DATALENGTH; dwI++)
        {
          printf("wV0:[%d] wV1:[%d] wV2:[%d]\n", wV0[dwI], wV1[dwI], wV2[dwI]);
	/* close device */
        }

        PCIDA_Close(fd);

        puts("End of program");

        return SUCCESS;
}

