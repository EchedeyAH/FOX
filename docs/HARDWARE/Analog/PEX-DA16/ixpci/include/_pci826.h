/* Declarations for PCI-826

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
 
   v 0.0.0 12 Sep 2013 by Winson Chen
   create
*/
#ifndef _PCI826_H
#define _PCI826_H

int _write_reg(ixpci_reg_t * , unsigned long int []);
int _read_reg(ixpci_reg_t * , unsigned long int []);
ixpci_devinfo_t *_align_minor(int);

#define MODULE_NAME "ixpci826"

/* offset of registers */
/* bar 1 */
#define _READ_WRITE_DIGITAL_IO_PORTA	0x00
#define _READ_WRITE_DIGITAL_IO_PORTB	0x04
#define _READ_WRITE_EEPROM		0x08
#define _GET_DIO_JUMPER_STATUS		0x0c
#define _GET_CARD_ID			0x0c
#define _SET_DIO_PORTA_PORTB_CONFIGURATION		0x0c
/* bar 2 */
#define _READ_WRITE_DA_DATA		0x00
#define	_READ_SET_DA_CONTROL_SETTING	0x04
#define _ENABLE_DISABLE_DA_CHANNEL	0x08
/* bar 3 */
#define	_READ_WRTIE_AD_POLLING_CONFIG	0x00
#define _AD_TRIGGER_FOR_POLLING_MODE	0x04
#define _READ_FIFO_DATA			0x04
#define _READ_SET_SAMPLING_RATE		0x08
#define _READBACK_SET_CONUNT_NUMBER_FOR_MAGIC_SCAN	0x0c
#define _AD_PACER_CONTROL_SETTING	0x10
#define _READ_AD_PACER_CONTROL_SETTING	0x10
#define _READ_WRITE_MAGIC_SCAN_BASE_FREQUENCY_SETTING	0x14
#define _START_STOP_MAGIC_SCAN		0x18
#define _CLEAR_INTERRUPT		0x18
#define _READ_SET_INTERRUPT_CONTROL	0x1c


/* abbreviation of registers */
/* bar 1 */
#define _DIO_PA		_READ_WRITE_DIGITAL_IO_PORTA
#define _DIO_PB		_READ_WRITE_DIGITAL_IO_PORTB
#define _EEP		_READ_WRITE_EEPROM
#define _GDIO_JS	_GET_DIO_JUMPER_STATUS
#define _GCID		_GET_CARD_ID
#define _PAB_CONFIG	_SET_DIO_PORTA_PORTB_CONFIGURATION
/* bar 2 */
#define _RW_DA		_READ_WRITE_DA_DATA
#define _RS_DA_CS	_READ_SET_DA_CONTROL_SETTING
#define _ED_DA_CH	_ENABLE_DISABLE_DA_CHANNEL
/* bar 3 */
#define _RW_AD_PC	_READ_WRTIE_AD_POLLING_CONFIG
#define _AD_TP		_AD_TRIGGER_FOR_POLLING_MODE
#define _AI		_READ_FIFO_DATA
#define _RS_SR		_READ_SET_SAMPLING_RATE
#define _RS_CN_MS	_READBACK_SET_CONUNT_NUMBER_FOR_MAGIC_SCAN
#define _AI_PC		_AD_PACER_CONTROL_SETTING
#define _RAI_PC		_READ_AD_PACER_CONTROL_SETTING
#define _RW_MS_BF_CS	_READ_WRITE_MAGIC_SCAN_BASE_FREQUENCY_SETTING
#define _SS_MS		_START_STOP_MAGIC_SCAN
#define _CI		_CLEAR_INTERRUPT
#define _RS_IC		_READ_SET_INTERRUPT_CONTROL

/* mask of registers (16-bit operation) */
#define _DIO_MASK		0xffff
#define _EEP_MASK		0xffff
#define _GDIO_JS_MASK		0x07
#define _GCID_MASK		0x0f	
#define _PAB_CONFIG_MASK	0x03
#define _RW_DA_MASK		0xffff
#define _RS_DA_CS_MASK		0x03
#define _ED_DA_CH_MASK		0x03
#define _RW_AD_PC_MASK		0x80ff
#define _AD_TP_MASK		0xffff
#define _AI_MASK		0xffff
#define _RS_SR_MASK		0xffff
#define _RS_CN_MS_MASK		0xffff
#define _AI_PC_MASK		0xfcff
#define _RAI_PC_MASK		0xf800
#define _RW_MS_BF_CS_MASK	0x8fff
#define _SS_MS_MASK		0xc000
#define _CI_MASK		0xffff
#define _RS_IC_MASK		0x000f

#endif					/* _PCI826_H */
