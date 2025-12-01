/* Declarations for PCI-FC16

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

/* File level history (record changes for this file here.
 
   *** Do not include this file in your code. ***
 
   v 0.0.0 23 Mar 2017 by Golden Wang
   create
*/
#ifndef _PCIFC16_H
#define _PCIFC16_H

int _write_reg(ixpci_reg_t * reg, void __iomem *base[]);
int _read_reg(ixpci_reg_t * reg, void __iomem * base[]);

#define MODULE_NAME "ixpcifc16"

/* offset of registers */
/* bar 1 */
#define _READ_WRITE_DIGITAL_IO_PORTA	0x00
#define _READ_WRITE_DIGITAL_IO_PORTB	0x04
#define _GET_DIO_JUMPER_STATUS		0x0c
#define _GET_CARD_ID			0x0c
#define _SET_DIO_PORTA_PORTB_CONFIGURATION 0x0c
/* bar 2, 3 */
#define _TIMER_CHANNEL_MODE		0x20
#define _TIMER_SPEED_MODE		0x24
#define _TIMER_SELECT_CHANNEL		0x40
#define _TIMER_LATCH_CHANNEL		0x44
#define _TIMER_CLEAR_CHANNEL		0x4c
#define	_TIMER_READ_BIT0_7  	 	0x40
#define	_TIMER_READ_BIT8_15  	 	0x44
#define	_TIMER_READ_BIT16_23  	 	0x48
#define	_TIMER_READ_BIT24_31  	 	0x4c

/* abbreviation of registers */
/* bar 1 */
#define _DIO_PA		_READ_WRITE_DIGITAL_IO_PORTA
#define _DIO_PB		_READ_WRITE_DIGITAL_IO_PORTB
#define _GDIO_JS	_GET_DIO_JUMPER_STATUS
#define _GCID		_GET_CARD_ID
#define _PAB_CONFIG	_SET_DIO_PORTA_PORTB_CONFIGURATION
/* bar 2, 3 */
#define _TCM		_TIMER_CHANNEL_MODE
#define _TSM		_TIMER_SPEED_MODE
#define _TSC		_TIMER_SELECT_CHANNEL
#define _TLC		_TIMER_LATCH_CHANNEL
#define _TCC		_TIMER_CLEAR_CHANNEL
#define _TRB0_7		_TIMER_READ_BIT0_7
#define _TRB8_15	_TIMER_READ_BIT8_15
#define _TRB16_23	_TIMER_READ_BIT16_23
#define _TRB24_31	_TIMER_READ_BIT24_31

/* mask of registers (16-bit operation) */
#define _DIO_MASK		0xffff
#define _EEP_MASK		0xffff
#define _GDIO_JS_MASK		0x07
#define _GCID_MASK		0x0f	
#define _PAB_CONFIG_MASK	0x03
#define _TCM_MASK		0x00ff
#define _TSM_MASK		0x00ff
#define _TSC_MASK		0x00ff
#define _TLC_MASK		0x00ff
#define _TCC_MASK		0x00ff
#define _TRD_MASK		0x00ff

#endif					/* _PCIFC16_H */
