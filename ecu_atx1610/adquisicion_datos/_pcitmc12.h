/* Declarations for PCI-TMC12

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

/* File level history (record changes for this file here.
 
   *** Do not include this file in your code. ***

   v 0.0.0 14 Mar 2003 by Reed Lai
     Sunny day. */

#ifndef _PCITMC12_H
#define _PCITMC12_H

struct task_struct *find_task_by_pid(pid_t nr, struct pid_namespace *ns);
int _write_reg(ixpci_reg_t * , unsigned long int []);
int _read_reg(ixpci_reg_t * , unsigned long int []);
int _reset_dev(ixpci_devinfo_t *);
int _set_signal(ixpci_signal_t *, ixpci_devinfo_t *);

#define MODULE_NAME "ixpcitmc12"

#define NUMBER_OF_8254_CHIPS 4

/* The IO address names come from the hardware manual */
/* bar 0 */
/* bar 1 */
#define PCI_INTERRUPT_CONTROL_REG  0x4c
/* bar 2 */
#define _8254_COUNTER_0              0x00
#define _8254_COUNTER_1              0x04
#define _8254_COUNTER_2              0x08
#define _8254_CONTROL_WORD           0x0c
#define SELECT_THE_ACTIVE_8254_CHIP  0x10
#define DIGITAL_OUTPUT_CHANNEL       0x14
#define DIGITAL_INPUT_CHANNEL        0x14
#define XOR_CONTROL_REGISTER         0x18
/* bar 3 */
/* bar 4 */
/* bar 5 */

/* abbreviation of names */
/* bar 0 */
/* bar 1 */
#define _PICR    PCI_INTERRUPT_CONTROL_REG
/* bar 2 */
#define _8254C0  _8254_COUNTER_0
#define _8254C1  _8254_COUNTER_1
#define _8254C2  _8254_COUNTER_2
#define _8254CR  _8254_CONTROL_WORD
#define _8254CS  SELECT_THE_ACTIVE_8254_CHIP
#define _DO      DIGITAL_OUTPUT_CHANNEL
#define _DI      DIGITAL_INPUT_CHANNEL
#define _XOR	 XOR_CONTROL_REGISTER
/* bar 3 */
/* bar 4 */
/* bar 5 */

#define LATCH_CNT_ENABLE 0x1000	//20211216 : bit 12 = 1(LATCH Enable), bit 12 = 0(LATCH Disable) 

/* mask of registers (16-bit operation) */
#define _PICR_MASK         0xc3
#define _PICR_INT_ENABLE   0x41
#define _PICR_INT_STATUS   0x04
#define _PICR_INT_POLARITY 0x02

#define _8254C0_MASK  0xff
#define _8254C1_MASK  0xff
#define _8254C2_MASK  0xff
#define _8254CR_MASK  0xff
#define _8254CS_MASK  0xff

#define _DO_MASK      0xffff
#define _DI_MASK      0xffff
#define _XOR_MASK     0xffff	

//void _disable_irq(ixpci_devinfo_t *);

#endif							/* _PCITMC12_H */
