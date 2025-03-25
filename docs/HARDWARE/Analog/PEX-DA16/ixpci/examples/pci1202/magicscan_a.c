/* Example of magic scan for PCI-120x with PCI Static Libary.

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

   v 0.0.0 13 Jan Winson 
     create */


/*
The configuration table of PEX-1202L and PCI-1202/1800/1802(L/LU) is given as follows:
Bipolar/Unipolar Input Signal Range Gain Settling Time [B9,B8,B7,B6]

	Bipolar +/- 5V		0000	//gain = 0, +-5V
	Bipolar +/- 2.5V 	0001
	Bipolar +/- 1.25V  	0010
	Bipolar +/- 0.625V	0011
	Bipolar +/- 10V   	0100
	Bipolar +/- 5V		0101
	Bipolar +/- 2.5V	0110
	Bipolar +/- 1.25V	0111
	Unipolar 0V~10V		1000
	Unipolar 0V~5V		1001
	Unipolar 0V~2.5V	1010
	Unipolar 0V~1.25V	1011

The configuration table of PEX-1202H and PCI-1202/1800/1802(H/HU) is given as follows:
Bipolar/Unipolar Input Signal Range Gain Settling Time [B9,B8,B7,B6]

	Bipolar +/- 5V		0000
	Bipolar +/- 0.5V  	0001
	Bipolar +/- 0.05V  	0010
	Bipolar +/- 0.005V  	0011
	Bipolar +/- 10V  	0100
	Bipolar +/- 1V  	0101
	Bipolar +/- 0.1V  	0110
	Bipolar +/- 0.01V  	0111
	Unipolar 0V~10V 	1000
	Unipolar 0V~1V  	1001
	Unipolar 0V~0.1V 	1010
	Unipolar 0V~0.01V  	1011
*/

#include "pcidio.h"
int main()
{
        int fd;
        char *dev_file;
	WORD wSampleRateDiv=40; // The sampling rate of MagicScan is 8M/wSampleRateDiv
	WORD wRetVal,wStatus;
	DWORD dwLowAlarm, dwHighAlarm, dwI;
	DWORD DATALENGTH=100; // how many data to be scaned for each channel
	int  nPriority=2;	  //
	int cardtype;

	WORD   wV0[100];	 // AD ch:0 buffer
	WORD   wV1[100];	 // AD ch:3 buffer
	WORD   wV2[100];	 // AD ch:5 buffer
	WORD   old_wV0[10];  // AD ch:4 buffer for erasing wave form
	WORD   old_wV1[10];  // AD ch:3 buffer for erasing wave form
	WORD   old_wV2[10];  // AD ch:5 buffer for erasing wave form
	WORD   gain[32];	//each cheannel gain set;
	WORD   dwAdConfig[32];
	float  fValue[32];

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

        wRetVal=PCI_120X_ClearScan(fd);

	gain[0] = 0;	// +/-5V
	gain[1] = 1;	// +/-2.5V
	gain[2] = 0;	// +/-5V

        wRetVal += PCI_120X_AddToScan(fd,0,gain[0],1,0,0,0); // CH:0 to scan
        wRetVal += PCI_120X_SaveScan(fd,0,wV0);        // Notice: 0 not 4
        wRetVal += PCI_120X_AddToScan(fd,3,gain[1],1,0,0,0); // CH:3 to scan
        wRetVal += PCI_120X_SaveScan(fd,1,wV1);        // Notice: 1 not 3
        wRetVal += PCI_120X_AddToScan(fd,5,gain[2],1,0,0,0); // CH:5 to scan
        wRetVal += PCI_120X_SaveScan(fd,2,wV2);        // Notice: 2 not 5

        PCI_120X_StartScan(fd,wSampleRateDiv,DATALENGTH,nPriority); 

        for(; ;)
        {
	  PCI_120X_ReadScanStatus(&wStatus,&dwLowAlarm,&dwHighAlarm);
	  if( wStatus>1 )
	     break;
	  sleep(3);
        }
        PCI_120X_StopMagicScan(fd);

	cardtype = 0x0;		//PCI/PEX-1202L
	//cardtype = 0x10;	//PCI/PEX-1202H

        for (dwI=0; dwI<DATALENGTH; dwI++)
        {
		fValue[0] = PCI_120X_ComputeRealValue(fd, gain[0]+cardtype, wV0[dwI]);
		fValue[1] = PCI_120X_ComputeRealValue(fd, gain[1]+cardtype, wV1[dwI]);
		fValue[2] = PCI_120X_ComputeRealValue(fd, gain[2]+cardtype, wV2[dwI]);

		printf("fValue[0]:%f fValue[1]:%f fValue[2]:%f\n", fValue[0],fValue[1],fValue[2]);
	/* close device */
        }

        PCIDA_Close(fd);

        puts("End of program");

        return SUCCESS;
}

