/* Declarations for PCI-1002

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

   v 0.0.0  7 Jan 2003 by Reed Lai
     create, blah blah... */

#ifndef _PCI1002_H
#define _PCI1002_H

void clear_int(ixpci_devinfo_t * dp);
struct task_struct *find_task_by_pid(pid_t nr, struct pid_namespace *ns);
int _write_reg(ixpci_reg_t * reg, unsigned long int base[]);
int _read_reg(ixpci_reg_t * reg, unsigned long int base[]);
int _time_span(int span, unsigned long base[]);
int _reset_dev(ixpci_devinfo_t * dp);
int set_signal(ixpci_signal_t * sig, ixpci_devinfo_t * dp);

#define MODULE_NAME "ixpci1002"

/* offset of registers */
/* bar 0 */
/* bar 1 */
#define PCI_INTERRUPT_CONTROL_REG	  0x4c
/* bar 2 */
#define _8254_TIMER_1                     0x00
#define _8254_TIMER_2                     0x04
#define _8254_TIMER_3                     0x08
#define _8254_CONTROL_REG                 0x0c
#define ANALOG_INPUT_CHANNEL_CONTROL_REG  0x10
#define STATUS_REG                        0x10
#define ANALOG_INPUT_GAIN_CONTROL_REG     0x14
#define GENERAL_CONTROL_REG               0x18
#define AD_SOFTWARE_TRIGGER               0x1c
#define CLEAR_INTERRUPT                   0x1c
#define DIGITAL_OUTPUT_REG                0x20
#define DIGITAL_INPUT_REG                 0x20
#define CARD_ID				  0x2c
#define AD_DATA_REG                       0x30
/* bar 3 */
/* bar 4 */
/* bar 5 */

/* abbreviation of registers */
/* bar 0 */
/* bar 1 */
#define _PICR    PCI_INTERRUPT_CONTROL_REG
/* bar 2 */
#define _8254C0  _8254_TIMER_1	/* take care the digit!! */
#define _8254C1  _8254_TIMER_2
#define _8254C2  _8254_TIMER_3
#define _8254CR  _8254_CONTROL_REG
#define _AICR    ANALOG_INPUT_CHANNEL_CONTROL_REG
#define _SR      STATUS_REG
#define _AIGR    ANALOG_INPUT_GAIN_CONTROL_REG
#define _GCR     GENERAL_CONTROL_REG
#define _CR      GENERAL_CONTROL_REG
#define _ADST    AD_SOFTWARE_TRIGGER
#define _CI      CLEAR_INTERRUPT
#define _DO      DIGITAL_OUTPUT_REG
#define _DI      DIGITAL_INPUT_REG
#define _CID	 CARD_ID
#define _AI      AD_DATA_REG
/* bar 3 */
/* bar 4 */
/* bar 5 */

/* mask of registers (16-bit operation) */
#define _PICR_MASK         0x43
#define BOARD_IRQ_ENABLE   0x43
#define BOARD_IRQ_DISABLE  0x03
#define _PICR_INT_FLAG     0x04

#define _8254C0_MASK  0xff
#define _8254C1_MASK  0xff
#define _8254C2_MASK  0xff
#define _8254CR_MASK  0xff
#define _AICR_MASK    0x1f

#define _SR_MASK        0xfb
#define _8254C0_STATUS  0x10
#define _8254C1_STATUS  0x20
#define _8254C2_STATUS  0x08
#define _AD_BUSY        0x01

#define _AIGR_MASK    0x03
#define _GCR_MASK     0x1f
#define _CR_MASK      _GCR_MASK
#define _ADST_MASK    0xff
#define _CI_MASK      0xff
#define _DO_MASK      0xffff
#define _DI_MASK      0xffff
#define _AI_MASK      0xffff
#define _AI_DATA_MASK 0x0fff
#define _CID_MASK     0xf0

#endif							/* _PCI1002_H */
