#include "_pcie8622.h"
//#include "_pcie8622_cal.h"
#include <stdio.h>

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
