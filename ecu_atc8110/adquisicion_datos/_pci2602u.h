/* Items of PCI-2602U

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

   v 0.0.0  31 Aug 2023 by Winson Chen
     Create */

#ifndef _PCI2602U
#define _PCI2602U

struct task_struct *find_task_by_pid(pid_t nr, struct pid_namespace *ns);
int _write_reg(ixpci_reg_t * reg, void __iomem *ioaddr[]);
int _read_reg(ixpci_reg_t * reg, void __iomem * ioaddr[]);

#define MODULE_NAME "ixpci2602u"

/* offset of registers */
/* bar 0  */
/* bar 1  */
/* bar 2  */
/* bar 3  */
#define _RESET_AI_MODE				0x200
#define _CLEAR_AI_FIFO				0x200
#define _RESET_AI_FIFO_OVERFLOW_STATUS		0x200
#define	_CLEAR_INTERRUPT			0x200
#define _RESET_DI_FIFO				0x200
#define _RESET_DO_FIFO				0x200
#define _RESET_AO0_FIFO				0x200
#define _RESET_DI_FIFO_OVERFLOW_STATUS		0x200
#define _GET_CARD_ID				0x200
#define _DIO_PORTA_TO_PORTD_CONFIGURATION	0x210
#define _DIO_PORTA_TO_PORTD			0x214
#define _DIGITAL_INPUT_OUTPUT_PORTA		0x214
#define _DIGITAL_INPUT_OUTPUT_PORTB		0x214
#define _DIGITAL_INPUT_OUTPUT_PORTC		0x214
#define _DIGITAL_INPUT_OUTPUT_PORTD		0x214
#define _DI_FIFO_STATUS				0x21c
#define _AI_SOFTWARE_TRIGGER_CONRTOL		0x294	//write
#define _AI_SOFTWARE_TRIGGER_STATUS		0x294	//read
#define _AI_SCAN_ADDRESS			0x298
#define _AO_CONFIGURATION			0x2b0
#define _AO_CHANNEL_0				0x2b4
#define _AO_CHANNEL_1				0x2b8
#define _AI_CONFIGURATION_CONTROL_STATUS	0x2ec
#define _SAVE_AI_CONFIGURATION			0x2f0
/* bar 4  */
/* bar 5  */

/* abbreviation of registers */
/* bar 0 */
/* bar 1 */
/* bar 2 */
/* bar 3 */
#define _AI_RM		_RESET_AI_MODE
#define _AI_CF		_CLEAR_AI_FIFO
#define	_AI_RFOS	_RESET_AI_FIFO_OVERFLOW_STATUS
#define	_CI		_CLEAR_INTERRUPT
#define _DI_RF		_RESET_DI_FIFO
#define _DO_RF		_RESET_DO_FIFO
#define _AO0_RF		_RESET_AO0_FIFO
#define _DI_RFOS	_RESET_DI_FIFO_OVERFLOW_STATUS
#define _GCID		_GET_CARD_ID
#define _DIO_PABCD_C	_DIO_PORTA_TO_PORTD_CONFIGURATION
#define _DIO_PABCD	_DIO_PORTA_TO_PORTD
#define _DIOPA		_DIGITAL_INPUT_OUTPUT_PORTA
#define _DIOPB		_DIGITAL_INPUT_OUTPUT_PORTB
#define _DIOPC		_DIGITAL_INPUT_OUTPUT_PORTC
#define _DIOPD		_DIGITAL_INPUT_OUTPUT_PORTD
#define _DI_FS		_DI_FIFO_STATUS
#define _AI_STC		_AI_SOFTWARE_TRIGGER_CONRTOL
#define _AI_STS		_AI_SOFTWARE_TRIGGER_STATUS
#define _AI_SA		_AI_SCAN_ADDRESS
#define _AO_CFG		_AO_CONFIGURATION
#define _AO0		_AO_CHANNEL_0
#define _AO1		_AO_CHANNEL_1
#define _AI_CC		_AI_CONFIGURATION_CONTROL_STATUS
#define _SAI_C		_SAVE_AI_CONFIGURATION

/* bar 4 */
/* bar 5 */

/* mask of registers (8-bit operation) */
#define _AI_RM_MASK		0x1
#define _AI_CF_MASK		0x2
#define _AI_RFOS_MASK		0x4
#define	_CI_MASK		0x8
#define	_DI_RF_MASK		0x40
#define	_DO_RF_MASK		0x80
#define	_AO0_RF_MASK		0x100
#define	_DI_RFOS_MASK		0x200
#define _GCID_MASK		0xf
#define _DIO_PABCD_C_MASK	0xf
#define _DIO_PABCD_MASK		0xffffffff
#define _DIOPA_MASK		0xff
#define _DIOPB_MASK		0xff00
#define _DIOPC_MASK		0xff0000
#define _DIOPD_MASK		0xff000000
#define _DI_FS_MASK		0x3f0000
#define _AI_STC_MASK		0x1
#define _AI_STS_MASK		0x1ffff
#define _AI_SA_MASK		0xffffffff
#define _AO_CFG_MASK		0xffff
#define _AO0_MASK		0xffff
#define _AO1_MASK		0xffff
#define _AI_CC_MASK		0x79fff
#define _SAI_C_MASK		0x1

#endif
