/* Items of PCI-D96SU/D128SU

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

/* File level history (record changes for this file here.
 
   *** Do not include this file in your code. ***

   v 0.0.0  4 May 2020 by Winson Chen
     Create */

#ifndef _PCID96_D128
#define _PCID96_D128

struct task_struct *find_task_by_pid(pid_t nr, struct pid_namespace *ns);
int _write_reg(ixpci_reg_t * reg, void __iomem *ioaddr[]);
int _read_reg(ixpci_reg_t * reg, void __iomem * ioaddr[]);
int set_signal(ixpci_signal_t * sig, ixpci_devinfo_t * dp);

#define MODULE_NAME "ixpcid96"

/* offset of registers */
/* bar 0  */
#define _DIGITAL_IO_PORTA_SELECTION_CONTROL	0x0
#define _DIGITAL_IO_PORTA			0x4
#define _DIGITAL_IO_PORTB_SELECTION_CONTROL	0x8
#define _DIGITAL_IO_PORTB			0xc
#define _DIGITAL_IO_PORTC_SELECTION_CONTROL	0x10
#define _DIGITAL_IO_PORTC			0x14
#define _DIGITAL_IO_PORTD_SELECTION_CONTROL	0x18
#define _DIGITAL_IO_PORTD			0x1c
#define _PORTA_CAMPARE_VALUE_SETTING		0x20
#define _PORTB_CAMPARE_VALUE_SETTING		0x24
#define _PORTC_CAMPARE_VALUE_SETTING		0x28
#define _PORTD_CAMPARE_VALUE_SETTING		0x2c
#define _PORT_PATTERN_MATCH_CHANGE_OF_STATUS	0x30
#define _PORTA_CLEAR_CHANGE_OF_STATUS		0x34
#define _PORTB_CLEAR_CHANGE_OF_STATUS		0x3c
#define _PORT_PATTERN_CLK			0x54	//PortC only
#define _PORT_PATTERN_NUMBER			0x58	//PortC only
#define _PORT_PATTERN_OUTPUT			0x5c	//PortC only
#define _GET_CARD_ID				0x60

/* bar 1  */
#define _PATTERN_DATA_SETTING			0x0	//0x0~0x01ff

/* bar 2 (NA)*/

/* bar 3 (NA) */
/* bar 4 (NA) */
/* bar 5 (NA) */

/* abbreviation of registers */
/* bar 0 */
#define _DIO_PA_CR	_DIGITAL_IO_PORTA_SELECTION_CONTROL
#define _DIO_PA		_DIGITAL_IO_PORTA
#define _DIO_PB_CR	_DIGITAL_IO_PORTB_SELECTION_CONTROL
#define _DIO_PB		_DIGITAL_IO_PORTB
#define _DIO_PC_CR	_DIGITAL_IO_PORTC_SELECTION_CONTROL
#define _DIO_PC		_DIGITAL_IO_PORTC
#define _DIO_PD_CR	_DIGITAL_IO_PORTD_SELECTION_CONTROL
#define _DIO_PD		_DIGITAL_IO_PORTD
#define _PA_CVS		_PORTA_CAMPARE_VALUE_SETTING
#define _PB_CVS		_PORTB_CAMPARE_VALUE_SETTING
#define _PC_CVS		_PORTC_CAMPARE_VALUE_SETTING
#define _PD_CVS		_PORTD_CAMPARE_VALUE_SETTING
#define _PPM_CS		_PORT_PATTERN_MATCH_CHANGE_OF_STATUS
#define _PA_CCS		_PORTA_CLEAR_CHANGE_OF_STATUS
#define _PB_CCS		_PORTB_CLEAR_CHANGE_OF_STATUS
#define _PPC		_PORT_PATTERN_CLK
#define _PPN		_PORT_PATTERN_NUMBER
#define _PPO		_PORT_PATTERN_OUTPUT
#define _GCID		_GET_CARD_ID
/* bar 1 */
#define _PDS		_PATTERN_DATA_SETTING
/* bar 2 */
/* bar 3 */
/* bar 4 */
/* bar 5 */

/* mask of registers (8-bit operation) */
#define _DIOP_CR_MASK		0xf		//_DIO_PA_CR to _DIO_PD_CR
#define _DIOP_MASK		0xffffffff	//_DIO_PA to _DIO_PD
#define _PORT_CVS_MASK  	0xffffffff	//_PA_CVS to _PD_CVS
#define _PPM_CS_MASK		0x3f3f3f	//_PPM_CS
#define _PORT_CCS_MASK		0x1		//_PA_CCS & _PB_CCS
#define _PPC_MASK		0x1
#define _PPN_MASK		0x1
#define _PPO_MASK		0x1
#define _GCID_MASK      	0xf
#define _PDS_MASK		0xffffffff	//not 0x1

#endif
