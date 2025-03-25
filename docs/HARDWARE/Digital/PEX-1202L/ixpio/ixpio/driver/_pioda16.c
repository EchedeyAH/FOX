/* PIO-DA16/DA8/DA4 Service Module.

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

/* File level history (record changes for this file here.)

   Use vi or gvim to edit this file and set tabstop=4.

   v 0.3.0  28 Mar 2011 by Golden Wang
     Give support to linux kernel 2.6.37

   v 0.2.0  28 Jan 2007 by Reed Lai
     Give support to linux kernel 2.6.x

   v 0.1.0 12 Aug 2003 by Reed Lai
     Complex signal.

   v 0.0.5  6 Aug 2003 by Reed Lai
     Fixed the clear_analog_pat, and rename to del_analog_pat.

   v 0.0.4  9 Jul 2003 by Reed Lai
     Refined digital_in() and digital_out().

   v 0.0.3  3 Jul 2003 by Reed Lai
     Fixed the bug about align_to_minor() that caused the Segmentation fault
     when opening a fake device file.

   v 0.0.2 30 Jun 2003 by Reed Lai
     Fixed the start_data() and stop_data() that used wrong symbol.

   v 0.0.1 26 Mar 2003 by Reed Lai
     Fixed the bug that free_irq with different argument to the request_irq.
     That bug caused the kernel was unable to handle a NULL pointer when
     interrupting.

   v 0.0.0  3 Dec 2002 by Reed Lai
     create, blah blah... */

/* *INDENT-OFF* */

/* Mandatory */
#include <linux/kernel.h>		/* ta, kernel work */
#include <linux/module.h>		/* is a module */

#include "ixpio.h"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
#include <linux/cdev.h>
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
        /* Deal with CONFIG_MODVERSIONS that is defined in
         * /usr/include/linux/config.h (config.h is included by module.h) */
        #if CONFIG_MODVERSIONS==1
        #define MODVERSIONS
        #include <linux/modversions.h>
        #endif

        /* for compatibility with future version of Linux */
        #include <linux/wrapper.h>
#endif
/* Additional */
#include <linux/fs.h>

/* function copy_from_user() & copy_to_user()*/
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,2,0) && LINUX_VERSION_CODE < KERNEL_VERSION(4,10,0)
#include <asm/uaccess.h>
#else
#include <linux/uaccess.h>
#endif

	/* function copy_from_user() & copy_to_user*/
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,2,0) && LINUX_VERSION_CODE < KERNEL_VERSION(4,10,0)
#include <asm/uaccess.h>
#else
#include <linux/uaccess.h>
#endif

/* use I/O ports */
#include <asm/io.h>
#include <linux/ioport.h>

/* need kmalloc */
#include <linux/slab.h>

/* irq service */
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <asm/irq.h>

/* Local */
#include "_flags_inline.h"
#include "_pioda16.h"

ixpio_devinfo_t *dev;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
int ixpio_minor[256] = {0};
#endif

#ifdef MODULE_LICENSE
MODULE_AUTHOR("Reed Lai <reed@icpdas.com>");
MODULE_DESCRIPTION("ICPDAS PIO-series driver, PIO-DA16 service module");
MODULE_LICENSE(ICPDAS_LICENSE);
#endif							/* MODULE_LICENSE */

/* symbols exportation */
// EXPORT_NO_SYMBOLS; /* do not export symbols */
/*
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
EXPORT_SYMBOL_GPL(ixpioda16_ioctl);
EXPORT_SYMBOL_GPL(ixpioda16_release);
EXPORT_SYMBOL_GPL(ixpioda16_open);
#else
EXPORT_SYMBOL_NOVERS(ixpioda16_ioctl);
EXPORT_SYMBOL_NOVERS(ixpioda16_release);
EXPORT_SYMBOL_NOVERS(ixpioda16_open);
#endif
*/

irqreturn_t irq_handler(int irq, void *dev_id);
int add_analog_pat(ixpio_analog_t * ap, ixpio_devinfo_t * dp);
void clear_data(ixpio_devinfo_t * dp);
ixpio_devinfo_t *align_to_minor(int minor);
void _close(ixpio_devinfo_t * dp);
int _open(ixpio_devinfo_t * dp);


__inline__ void pattern_out(ixpio_devinfo_t * dp, unsigned int is)
{
	unsigned char hbyte;
	ixpio_analog_t *ap;
	__u16 *data;

	ap = (ixpio_analog_t *) dp->data.ptr;
	while (ap) {
		if (get_flag(&ap->flags, IXPIO_FLAG_ANALOG_PAT_START)) {
			if (ap->sig.is & is) {
				/* an int source */
				/* not both edges, up to sig.edge */
				if (!((ap->sig.edge & is) ^ (dp->is_edge & is))) {
					data = (__u16 *) ap->data.ptr;
					hbyte = (ap->channel << 6) | (data[ap->data_cur] >> 8);
#if DAL_MASK == 0xff
					outb((unsigned char) data[ap->data_cur], dp->base + DAL);
#else
					outb(((unsigned char) data[ap->data_cur]) & DAL_MASK,
						 dp->base + DAL);
#endif
#if DAH_MASK == 0xff
					outb(hbyte, dp->base + DAH);
#else
					outb(hbyte & DAH_MASK, dp->base + DAH);
#endif
					outb(0, dp->base + 0xe0 + (ap->channel & 0x0c));

					if ((++ap->data_cur) == ap->data_size)
						ap->data_cur = 0;
				}
			}
		}
		ap = ap->next;
	}
}


__inline__ unsigned int eoi(ixpio_devinfo_t * dp)
{
	/* edge of interrupt
	 *
	 * The interrupt output signal of PIO series behaves level-trigger and
	 * active-low on /INT of PCI bus.  This procedure detects the status of
	 * interrupt source (PC0/1/2/3) from APSR (Aux Pin Status Register,)
	 * and turns the level-triggered interrupt to P/N edge-triggered
	 * interrupt by toggles the bit of IPCR (INT polarity control register)
	 * to prevent interrupt storm.
	 *
	 * Arguments:
	 *   dp       pointer to device information
	 *
	 * Returned:  an unsigned int value for interrupt source */

	unsigned int imcr;			/* INT Mask Control Register */
	unsigned int apsr;			/* AUX Pin Status Register */
	unsigned int ipcr;
	unsigned int mask;			/* mask for test */

	dp->is = 0;

	apsr = inb(dp->base + APSR) & APSR_INT_MASK;
	imcr = inb(dp->base + IMCR) & IMCR_INT_MASK;
	ipcr = inb(dp->base + IPCR) & IPCR_INT_MASK;

	for (mask = 1; mask < INT_MASK_SHIFT_STOP_BIT; mask <<= 1) {
		if (imcr & mask) {
			if (ipcr & mask) {
				/* non-inverse mode */
				if (apsr & mask) {
					dp->is |= mask;
					/* interrupt source */
					dp->is_edge |= mask;
					/* positive edge */
				}
			} else {
				/* inverse mode */
				if (!(apsr & mask)) {
					dp->is |= mask;
					/* interrupt source */
					dp->is_edge &= !mask;
					/* negative edge */
				}
			}
		}
	}

	//dp->ipcr ^= dp->is;
	/* Inverts the polarity of interrupt */
	outb(ipcr ^ dp->is, dp->base + IPCR);
	/* source for next edge. */

	return dp->is;
}

#ifdef BOTTOM_HELF
void irq_handler_bh(void *data)
{
	/* bottom helf of irq handler
	 *
	 * Arguments:
	 *   data     data pointer as <linux/tqueue.h> defined
	 *
	 * Returned:  none */

	ixpio_devinfo_t *dp;

	dp = (ixpio_devinfo_t *) data;

	/* reserved */
}
#endif							/* BOTTOM_HELF */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)
  irqreturn_t irq_handler(int irq, void *dev_id)
#else
  #if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
  irqreturn_t irq_handler(int irq, void *dev_id, struct pt_regs *regs)
  #else
  void irq_handler(int irq, void *dev_id, struct pt_regs *regs)
  #endif
#endif
{
	/* Arguments:  as <linux/sched.h> defined
	 *
	 * Returned:  none */

	unsigned int is;			/* interrupt source */
	ixpio_devinfo_t *dp;

#ifdef BOTTOM_HELF
	static struct tq_struct task = { NULL, 0, irq_handler_bh, NULL };

	task.data = dev_id;
#endif							/* BOTTOM_HELF */

	dp = (ixpio_devinfo_t *) dev_id;
	is = eoi(dp);

	/* User's data pattern */
	if (get_flag(&dp->flags, IXPIO_FLAG_DATA_START))
		pattern_out(dp, is);

	ixpio_signaling(is, (ixpio_devinfo_t *) dev_id);

#ifdef BOTTOM_HELF
	/* Scheduale bottom half to run */
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,2,0)
	queue_task(&task, &tq_immediate);
#else
	queue_task_irq(&task, &tq_immediate);
#endif
	mark_bh(IMMEDIATE_BH);
#endif							/* BOTTOM_HELF */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
        return IRQ_HANDLED;
#endif
}

__inline__ void stop_data(ixpio_devinfo_t * dp)
{
	off_flag(&dp->flags, IXPIO_FLAG_DATA_START);
}

__inline__ void start_data(ixpio_devinfo_t * dp)
{
	if (dp->data.ptr)
		on_flag(&dp->flags, IXPIO_FLAG_DATA_START);
}

__inline__ int start_analog_pat(unsigned int id, ixpio_devinfo_t * dp)
{
	ixpio_analog_t *pat;
	int found = 0;

	pat = (ixpio_analog_t *) dp->data.ptr;

	while (pat) {
		if (pat->id == id) {
			on_flag(&pat->flags, IXPIO_FLAG_ANALOG_PAT_START);
			++found;
		}
		pat = pat->next;
	}
	return (found ? 0 : FAILURE);
}

__inline__ int stop_analog_pat(unsigned int id, ixpio_devinfo_t * dp)
{
	ixpio_analog_t *pat;
	int found = 0;

	pat = (ixpio_analog_t *) dp->data.ptr;

	while (pat) {
		if (pat->id == id) {
			off_flag(&pat->flags, IXPIO_FLAG_ANALOG_PAT_START);
			++found;
		}
		pat = pat->next;
	}
	return (found ? 0 : FAILURE);
}

__inline__ int del_analog_pat(unsigned int id, ixpio_devinfo_t * dp)
{
	ixpio_analog_t *pat, *found;

	found = 0;
	pat = (ixpio_analog_t *) dp->data.ptr;

	while (pat) {
		if (pat->id == id) {
			found = pat;
			if (pat->prev) {
				pat->prev->next = pat->next;
				if (pat->next) pat->next->prev = pat->prev;
			} else {
				dp->data.ptr = pat->next;
				if (pat->next) ((ixpio_analog_t *)dp->data.ptr)->prev = 0;
			}
			pat = pat->next;
			kfree(found);
		} else pat = pat->next;
	}
	return (found ? 0 : FAILURE);
}

__inline__ int analog_pat_status(ixpio_analog_t * ap, ixpio_devinfo_t * dp)
{
	ixpio_analog_t *pat;

	pat = (ixpio_analog_t *) dp->data.ptr;

	while (pat) {
		if (pat->id == ap->id) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
                        if (copy_to_user(ap, pat, sizeof(ixpio_analog_t))) return -EFAULT;
#else
                        copy_to_user(ap, pat, sizeof(ixpio_analog_t));
#endif
			return 0;
		}
		pat = pat->next;
	}
	return FAILURE;
}

__inline__ int retrieve_analog_pat(ixpio_analog_t * ap, ixpio_devinfo_t * dp)
{
	ixpio_analog_t *pat;

	pat = (ixpio_analog_t *) dp->data.ptr;

	while (pat) {
		if (pat->id == ap->id) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
                        if (copy_to_user(ap, pat, sizeof(ixpio_analog_t))) return -EFAULT;
#else
                        copy_to_user(ap, pat, sizeof(ixpio_analog_t));
#endif
			if (ap->data_size && ap->data.ptr)

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
                        if (copy_to_user(ap->data.ptr, pat->data.ptr, ap->data_size << 1)) return -EFAULT;
#else
                        copy_to_user(ap->data.ptr, pat->data.ptr, ap->data_size << 1);
#endif
			return 0;
		}
		pat = pat->next;
	}
	return FAILURE;
}

int add_analog_pat(ixpio_analog_t * ap, ixpio_devinfo_t * dp)
{
	/* Add user's data to this device's pattern list */
	ixpio_analog_t *pat, *prev;

	if (!ap)
		return FAILURE;

	prev = 0;
	pat = (ixpio_analog_t *) dp->data.ptr;

	while (pat) {
		/* seek the tail of list */
		prev = pat;
		pat = pat->next;
	}

	if (prev) {
		/* pattern list is already followed */
		pat = prev->next = kmalloc(sizeof(ixpio_analog_t), GFP_KERNEL);
		if (!pat)
			return FAILURE;
		memset(pat, 0, sizeof(ixpio_analog_t));
	} else {
		/* pattern list is empty, initiate */
		dp->data.ptr = kmalloc(sizeof(ixpio_analog_t), GFP_KERNEL);
		if (!dp->data.ptr)
			return FAILURE;
		memset(dp->data.ptr, 0, sizeof(ixpio_analog_t));
		pat = (ixpio_analog_t *) dp->data.ptr;
	}

	//memcpy(pat, ap, sizeof(ixpio_analog_t));
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
        if (copy_from_user(pat, ap, sizeof(ixpio_analog_t)))return -EFAULT;
#else
        copy_from_user(pat, ap, sizeof(ixpio_analog_t));
#endif
	pat->prev = prev;
	pat->next = 0;
	if (pat->data_cur >= pat->data_size)
		pat->data_cur = 0;

	/* allocate memory for pattern data */
	if (ap->data.ptr) {
		pat->data.ptr = kmalloc(ap->data_size << 1, GFP_KERNEL);
		if (!pat->data.ptr) {
			pat->prev->next = 0;
			kfree(pat);
			return FAILURE;
		}
		//memcpy(pat->data.ptr, ap->data.ptr, ap->data_size << 1);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
                if (copy_from_user(pat->data.ptr, ap->data.ptr, ap->data_size << 1)) return -EFAULT;
#else
                copy_from_user(pat->data.ptr, ap->data.ptr, ap->data_size << 1);
#endif
	}

	return SUCCESS;
}

void clear_data(ixpio_devinfo_t * dp)
{
	/* release analog pattern memory allocated for this device */
	ixpio_analog_t *pat, *prev;

	pat = dp->data.ptr;

	while (pat) {
		if (pat->data.ptr)
			kfree(pat->data.ptr);
		prev = pat;
		pat = pat->next;
		kfree(prev);
	}
	dp->data.ptr = 0;
}

__inline__ int write_reg(ixpio_reg_t * reg, unsigned long int base)
{
	/* Write to register
	 *
	 * Arguments:
	 *   reg      pointer to a structure ixpio_reg for register
	 *   base     base address for current device
	 *
	 * Returned:  SUCCESS or FAILURE */

	switch (reg->id) {
	case IXPIO_RCR:			/* Register Control Register */
#if RCR_MASK == 0xff
		outb(((unsigned char) reg->value), base + RCR);
#else
		outb(((unsigned char) reg->value) & RCR_MASK, base + RCR);
#endif
		break;
	case IXPIO_ACR:			/* AUX Control Register */
#if ACR_MASK == 0xff
		outb(((unsigned char) reg->value), base + ACR);
#else
		outb(((unsigned char) reg->value) & ACR_MASK, base + ACR);
#endif
		break;
	case IXPIO_ADR:			/* AUX Data Register */
#if ADR_MASK == 0xff
		outb(((unsigned char) reg->value), base + ADR);
#else
		outb(((unsigned char) reg->value) & ADR_MASK, base + ADR);
#endif
		break;
	case IXPIO_IMCR:			/* INT Mask Control Register */
#if IMCR_MASK == 0xff
		outb(((unsigned char) reg->value), base + IMCR);
#else
		outb(((unsigned char) reg->value) & IMCR_MASK, base + IMCR);
#endif
		break;
	case IXPIO_APSR:			/* AUX Pin Status Register */
#if APSR_MASK == 0xff
		outb(((unsigned char) reg->value), base + APSR);
#else
		outb(((unsigned char) reg->value) & APSR_MASK, base + APSR);
#endif
		break;
	case IXPIO_IPCR:			/* INT Polarity Control Register */
#if IPCR_MASK == 0xff
		outb(((unsigned char) reg->value), base + IPCR);
#else
		outb(((unsigned char) reg->value) & IPCR_MASK, base + IPCR);
#endif
		break;

	case IXPIO_8254C0:			/* 8254 Counter 0 */
#if _8254C0_MASK == 0xff
		outb(((unsigned char) reg->value), base + _8254C0);
#else
		outb(((unsigned char) reg->value) & _8254C0_MASK, base + _8254C0);
#endif
		break;
	case IXPIO_8254C1:			/* 8254 Counter 1 */
#if _8254C1_MASK == 0xff
		outb(((unsigned char) reg->value), base + _8254C1);
#else
		outb(((unsigned char) reg->value) & _8254C1_MASK, base + _8254C1);
#endif
		break;
	case IXPIO_8254C2:			/* 8254 Counter 2 */
#if _8254C2_MASK == 0xff
		outb(((unsigned char) reg->value), base + _8254C2);
#else
		outb(((unsigned char) reg->value) & _8254C2_MASK, base + _8254C2);
#endif
		break;
	case IXPIO_8254CW:			/* 8254 Control Word */
#if _8254CW_MASK == 0xff
		outb(((unsigned char) reg->value), base + _8254CW);
#else
		outb(((unsigned char) reg->value) & _8254CW_MASK, base + _8254CW);
#endif
		break;

	case IXPIO_DA0CS:			/* DA 0 Chip Select */
#if DA0CS_MASK == 0xff
		outb(((unsigned char) reg->value), base + DA0CS);
#else
		outb(((unsigned char) reg->value) & DA0CS_MASK, base + DA0CS);
#endif
		break;
	case IXPIO_DA1CS:			/* DA 1 Chip Select */
#if DA1CS_MASK == 0xff
		outb(((unsigned char) reg->value), base + DA1CS);
#else
		outb(((unsigned char) reg->value) & DA1CS_MASK, base + DA1CS);
#endif
		break;
	case IXPIO_DA2CS:			/* DA 2 Chip Select */
#if DA2CS_MASK == 0xff
		outb(((unsigned char) reg->value), base + DA2CS);
#else
		outb(((unsigned char) reg->value) & DA2CS_MASK, base + DA2CS);
#endif
		break;
	case IXPIO_DA3CS:			/* DA 3 Chip Select */
#if DA3CS_MASK == 0xff
		outb(((unsigned char) reg->value), base + DA3CS);
#else
		outb(((unsigned char) reg->value) & DA3CS_MASK, base + DA3CS);
#endif
		break;

	case IXPIO_DAL:			/* Analog Output Low Byte */
#if DAL_MASK == 0xff
		outb(((unsigned char) reg->value), base + DAL);
#else
		outb(((unsigned char) reg->value) & DAL_MASK, base + DAL);
#endif
		break;
	case IXPIO_DAH:			/* Analog Output High Byte */
#if DAH_MASK == 0xff
		outb(((unsigned char) reg->value), base + DAH);
#else
		outb(((unsigned char) reg->value) & DAH_MASK, base + DAH);
#endif
		break;

	case IXPIO_DL:				/* Digital Output Low Byte */
#if DOL_MASK == 0xff
		outb(((unsigned char) reg->value), base + DOL);
#else
		outb(((unsigned char) reg->value) & DOL_MASK, base + DOL);
#endif
		break;
	case IXPIO_DH:				/* Digital Output High Byte */
#if DOH_MASK == 0xff
		outb(((unsigned char) reg->value), base + DOH);
#else
		outb(((unsigned char) reg->value) & DOH_MASK, base + DOH);
#endif
		break;
	case IXPIO_DO:				/* Digital Output */
	case IXPIO_DIO:
#if DOL_MASK == 0xff
		outb(((unsigned char) reg->value), base + DOL);
#else
		outb(((unsigned char) reg->value) & DOL_MASK, base + DOL);
#endif
#if DOH_MASK == 0xff
		outb(((unsigned char) reg->value) >> 8, base + DOH);
#else
		outb((((unsigned char) reg->value) >> 8) & DOH_MASK, base + DOH);
#endif
		break;

	default:
		return FAILURE;
	}
	return SUCCESS;
}

__inline__ int read_reg(ixpio_reg_t * reg, unsigned long int base)
{
	/* Read from register
	 *
	 * Arguments:
	 *   reg      pointer to structure ixpio_reg for register
	 *   base     base address for current device
	 *
	 * Returned:  SUCCESS or FAILURE */

	unsigned int lbyte, hbyte;

	switch (reg->id) {
	case IXPIO_RCR:			/* Register Control Register */
#if RCR_MASK == 0xff
		reg->value = inb(base + RCR);
#else
		reg->value = inb(base + RCR) & RCR_MASK;
#endif
		break;
	case IXPIO_ACR:			/* AUX Control Register */
#if ACR_MASK == 0xff
		reg->value = inb(base + ACR);
#else
		reg->value = inb(base + ACR) & ACR_MASK;
#endif
		break;
	case IXPIO_ADR:			/* AUX Data Register */
#if ADR_MASK == 0xff
		reg->value = inb(base + ADR);
#else
		reg->value = inb(base + ADR) & ADR_MASK;
#endif
		break;
	case IXPIO_IMCR:			/* INT Mask Control Register */
#if IMCR_MASK == 0xff
		reg->value = inb(base + IMCR);
#else
		reg->value = inb(base + IMCR) & IMCR_MASK;
#endif
		break;
	case IXPIO_APSR:			/* AUX Pin Status Register */
#if APSR_MASK == 0xff
		reg->value = inb(base + APSR);
#else
		reg->value = inb(base + APSR) & APSR_MASK;
#endif
		break;
	case IXPIO_IPCR:			/* INT Polarity Control Register */
#if IPCR_MASK == 0xff
		reg->value = inb(base + IPCR);
#else
		reg->value = inb(base + IPCR) & IPCR_MASK;
#endif
		break;

	case IXPIO_8254C0:			/* 8254 Counter 0 */
#if _8254C0_MASK == 0xff
		reg->value = inb(base + _8254C0);
#else
		reg->value = inb(base + _8254C0) & _8254C0_MASK;
#endif
		break;
	case IXPIO_8254C1:			/* 8254 Counter 1 */
#if _8254C1_MASK == 0xff
		reg->value = inb(base + _8254C1);
#else
		reg->value = inb(base + _8254C1) & _8254C1_MASK;
#endif
		break;
	case IXPIO_8254C2:			/* 8254 Counter 2 */
#if _8254C2_MASK == 0xff
		reg->value = inb(base + _8254C2);
#else
		reg->value = inb(base + _8254C2) & _8254C2_MASK;
#endif
		break;
	case IXPIO_8254CW:			/* 8254 Control Word */
#if _8254CW_MASK == 0xff
		reg->value = inb(base + _8254CW);
#else
		reg->value = inb(base + _8254CW) & _8254CW_MASK;
#endif
		break;

	case IXPIO_DL:				/* Digital Input Low Byte */
#if DIL_MASK == 0xff
		reg->value = inb(base + DIL);
#else
		reg->value = inb(base + DIL) & DIL_MASK;
#endif
		break;
	case IXPIO_DH:				/* Digital Input High Byte */
#if DIH_MASK == 0xff
		reg->value = inb(base + DIH);
#else
		reg->value = inb(base + DIH) & DIH_MASK;
#endif
		break;
	case IXPIO_DI:				/* Digital Input */
	case IXPIO_DIO:
#if DIL_MASK == 0xff
		lbyte = inb(base + DIL);
#else
		lbyte = inb(base + DIL) & DIL_MASK;
#endif
#if DIH_MASK == 0xff
		hbyte = inb(base + DIH);
#else
		hbyte = inb(base + DIH) & DIH_MASK;
#endif
		reg->value = (hbyte << 8) + lbyte;
		break;
        case IXPIO_CID:                          /* Digital Input Low Byte */
#if CID_MASK == 0xff
                reg->value = inb(base + CID);
#else
                reg->value = inb(base + CID) & CID_MASK;
#endif
                break;

	default:
		return FAILURE;
	}
	return SUCCESS;
}

__inline__ int analog_out(ixpio_analog_t * ap, unsigned long int base)
{
	unsigned long int chip;
	unsigned char hbyte;

	chip = base + 0xe0 + (ap->channel & 0x0c);

	hbyte = (ap->channel << 6) | (ap->data.u16 >> 8);

#if DAL_MASK == 0xff
	outb((unsigned char) ap->data.u16, base + DAL);
#else
	outb(((unsigned char) ap->data.u16) & DAL_MASK, base + DAL);
#endif
#if DAH_MASK == 0xff
	outb(hbyte, base + DAH);
#else
	outb(hbyte & DAH_MASK, base + DAH);
#endif
	outb(0, chip);

	return SUCCESS;
}

__inline__ __u16 digital_in(unsigned long int base)
{
	__u16 lbyte, hbyte;

#if DIL_MASK == 0xff
	lbyte = inb(base + DIL);
#else
	lbyte = inb(base + DIL) & DIL_MASK;
#endif
#if DIH_MASK == 0xff
	hbyte = inb(base + DIH);
#else
	hbyte = inb(base + DIH) & DIH_MASK;
#endif

	return (lbyte | (hbyte << 8));
}

__inline__ void digital_out(unsigned int dout, unsigned long int base)
{
#if DOL_MASK == 0xff
	outb((unsigned char) dout, base + DOL);
#else
	outb((unsigned char) dout & DOL_MASK, base + DOL);
#endif
#if DOH_MASK == 0xff
	outb((unsigned char) (dout >> 8), base + DOH);
#else
	outb((unsigned char) (dout >> 8) & DOH_MASK, base + DOH);
#endif
}

ixpio_devinfo_t *align_to_minor(int minor)
{
	ixpio_devinfo_t *p;
	for (p = dev; p && p->no != minor; p = p->next_f);
	return p;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
long ixpioda16_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param)
#else
int ixpioda16_ioctl(struct inode *inode, struct file *file, unsigned int ioctl_num, unsigned long ioctl_param)
#endif
{
	/* (export)
	 *
	 * This function is called by ixpio.o whenever a process tries to do and
	 * IO control on PIO device file
	 *
	 * Arguments: read <linux/fs.h> for (*ioctl) of struct file_operations
	 *
	 * Returned:  SUCCESS or FAILED */

	ixpio_devinfo_t *dp, devinfo;
        void __user *argp = (void __user *)ioctl_param;
        ixpio_signal_t signal;
        ixpio_reg_t reg;
	ixpio_analog_t analog;
	ixpio_digital_t dio;
        unsigned int sig_num, analog_pat;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
        int minor;

        if(file->private_data)
        {
                minor = *((int *)file->private_data);
        }
        else
        {
                return -EINVAL;
        }

        dp = align_to_minor(minor);
#else
        dp = align_to_minor(MINOR(inode->i_rdev));
#endif

	if (!dp || !dp->open) return -EINVAL;

	switch (ioctl_num) {
	case IXPIO_ANALOG_OUT:
#if 1
		if (copy_from_user(&analog, argp, sizeof(ixpio_analog_t)))
		{
			printk("XXX %s ... IXPIO_ANALOG_OUT ... copy_from_user fail\n", __FUNCTION__);
			return -EFAULT;
		}

		if (analog_out(&analog, dp->base))
			return FAILURE;
#else
		if (analog_out((ixpio_analog_t *) ioctl_param, dp->base))
			return FAILURE;
#endif
		break;
	case IXPIO_DIGITAL_IN:
#if 1
		if (copy_from_user(&dio, argp, sizeof(ixpio_digital_t)))
		{
			printk("XXX %s ... IXPIO_DIGITAL_IN ... copy_from_user fail\n", __FUNCTION__);
			return -EFAULT;
		}

		dio.data.u16 = digital_in(dp->base);

                if (copy_to_user(argp, &dio, sizeof(ixpio_digital_t)))
                {
                        printk("XXX %s ... IXPIO_DIGITAL_IN ... copy_to_user fail\n", __FUNCTION__);
                        return -EFAULT;
                }
#else
		((ixpio_digital_t *) ioctl_param)->data.u16 = digital_in(dp->base);
#endif
		break;
	case IXPIO_DIGITAL_OUT:
#if 1
		if (copy_from_user(&dio, argp, sizeof(ixpio_digital_t)))
		{
			printk("XXX %s ... IXPIO_DIGITAL_IN ... copy_from_user fail\n", __FUNCTION__);
			return -EFAULT;
		}

		digital_out(dio.data.u16, dp->base);
#else
		digital_out(((ixpio_digital_t *) ioctl_param)->data.u16, dp->base);
#endif
		break;
	case IXPIO_READ_REG: /* read from register */
#if 1
                if (copy_from_user(&reg, argp, sizeof(ixpio_reg_t)))
                {
                        printk("XXX %s ... IXPIO_REG_READ ... copy_from_user fail\n", __FUNCTION__);
                        return -EFAULT;
                }

                if (read_reg(&reg, dp->base))
                        return FAILURE;

                if (copy_to_user(argp, &reg, sizeof(ixpio_reg_t)))
                {
                        printk("XXX %s ... IXPIO_REG_READ ... copy_to_user fail\n", __FUNCTION__);
                        return -EFAULT;
                }
#else
		if (read_reg((ixpio_reg_t *) ioctl_param, dp->base))
			return FAILURE;
#endif
		break;
	case IXPIO_WRITE_REG: /* write to register */
#if 1
                if (copy_from_user(&reg, argp, sizeof(ixpio_reg_t)))
                {
                        printk("XXX %s ... IXPIO_REG_WRITE ... copy_from_user fail\n",__FUNCTION__);
                        return -EFAULT;
                }

                if (write_reg(&reg, dp->base))
                        return FAILURE;
#else
		if (write_reg((ixpio_reg_t *) ioctl_param, dp->base))
			return FAILURE;
#endif
		break;
	case IXPIO_ANALOG_OUT_PAT_START:
#if 1
                if (copy_from_user(&analog_pat, argp, sizeof(unsigned int)))
                {
                        printk("XXX %s ... IXPIO_ANALOG_OUT_PAT_START ... copy_from_user fail\n",__FUNCTION__);
                        return -EFAULT;
                }

                if (start_analog_pat(analog_pat, dp))
#else
		if (start_analog_pat((unsigned int) ioctl_param, dp))
#endif
			return FAILURE;
		break;
	case IXPIO_ANALOG_OUT_PAT_PAUSE:
	case IXPIO_ANALOG_OUT_PAT_STOP:
#if 1
                if (copy_from_user(&analog_pat, argp, sizeof(unsigned int)))
                {
                        printk("XXX %s ... IXPIO_ANALOG_OUT_PAT_STOP ... copy_from_user fail\n",__FUNCTION__);
                        return -EFAULT;
                }

                if (stop_analog_pat(analog_pat, dp))
#else
		if (stop_analog_pat((unsigned int) ioctl_param, dp))
#endif
			return FAILURE;
		break;
	case IXPIO_ANALOG_OUT_PAT_STATUS:
#if 1
                if (copy_from_user(&analog, argp, sizeof(ixpio_analog_t)))
                {
                        printk("XXX %s ... IXPIO_ANALOG_OUT_PAT_STOP ... copy_from_user fail\n",__FUNCTION__);
                        return -EFAULT;
                }

                if (analog_pat_status(&analog, dp))
#else
		if (analog_pat_status((ixpio_analog_t *) ioctl_param, dp))
#endif
			return FAILURE;
		break;
	case IXPIO_ANALOG_OUT_PAT_RETRIEVE:
#if 1
                if (copy_from_user(&analog, argp, sizeof(ixpio_analog_t)))
                {
                        printk("XXX %s ... IXPIO_ANALOG_OUT_PAT_RETRIEVE ... copy_from_user fail\n",__FUNCTION__);
                        return -EFAULT;
                }

                if (retrieve_analog_pat(&analog, dp))
#else
		if (retrieve_analog_pat((ixpio_analog_t *) ioctl_param, dp))
#endif
			return FAILURE;
		break;
	case IXPIO_ANALOG_OUT_PAT_DEL:
#if 1
                if (copy_from_user(&analog_pat, argp, sizeof(unsigned int)))
                {
                        printk("XXX %s ... IXPIO_ANALOG_OUT_PAT_DEL ... copy_from_user fail\n",__FUNCTION__);
                        return -EFAULT;
                }

                if (del_analog_pat(analog_pat, dp))
#else
		if (del_analog_pat((unsigned int) ioctl_param, dp))
#endif
			return FAILURE;
		break;
	case IXPIO_ANALOG_OUT_PAT:
		clear_data(dp);
#if 1
                if (copy_from_user(&analog, argp, sizeof(ixpio_analog_t)))
                {
                        printk("XXX %s ... IXPIO_ANALOG_OUT_PAT ... copy_from_user fail\n",__FUNCTION__);
                        return -EFAULT;
                }

                if (add_analog_pat(&analog, dp))
#else
		if (add_analog_pat((ixpio_analog_t *) ioctl_param, dp))
#endif
			return FAILURE;
		break;
	case IXPIO_ANALOG_OUT_PAT_ADD:
#if 1
                if (copy_from_user(&analog, argp, sizeof(ixpio_analog_t)))
                {
                        printk("XXX %s ... IXPIO_ANALOG_OUT_PAT_ADD ... copy_from_user fail\n",__FUNCTION__);
                        return -EFAULT;
                }

                if (add_analog_pat(&analog, dp))
#else
		if (add_analog_pat((ixpio_analog_t *) ioctl_param, dp))
#endif
			return FAILURE;
		break;
	case IXPIO_DATA_CLEAR:
		clear_data(dp);
		break;
	case IXPIO_DATA_START:
		start_data(dp);
		break;
	case IXPIO_DATA_STOP:
		stop_data(dp);
		break;
	case IXPIO_SIG:
		ixpio_clear_signals(dp);
#if 1
                if (copy_from_user(&signal, argp, sizeof(ixpio_signal_t)))
                {
                        printk("XXX %s ... copy_from_user fail\n", __FUNCTION__);
                        return -EFAULT;
                }

                if (ixpio_add_signal(&signal, dp))
                        return FAILURE;
#else
		if (ixpio_add_signal((ixpio_signal_t *) ioctl_param, dp))
			return FAILURE;
#endif
		break;
	case IXPIO_SIG_ADD:
#if 1
                if (copy_from_user(&signal, argp, sizeof(ixpio_signal_t)))
                {
                        printk("XXX %s ... copy_from_user fail\n", __FUNCTION__);
                        return -EFAULT;
                }

                if (ixpio_add_signal(&signal, dp))
                        return FAILURE;
#else
		if (ixpio_add_signal((ixpio_signal_t *) ioctl_param, dp))
			return FAILURE;
#endif
		break;
	case IXPIO_SIG_DEL:
#if 1
                if (copy_from_user(&sig_num, argp, sizeof(unsigned int)))
                {
                        printk("XXX %s ... IXPIO_SIG_DEL ... copy_from_user fail\n", __FUNCTION__);
                        return -EFAULT;
                }

                if (ixpio_del_signal(sig_num, dp))
#else
		if (ixpio_del_signal((unsigned int) ioctl_param, dp))
#endif
			return FAILURE;
		break;
	case IXPIO_SIG_DEL_ALL:
		ixpio_clear_signals(dp);
		break;
	case IXPIO_KEEP_ALIVE:
		on_flag(&dp->flags, IXPIO_FLAG_KEEP_ALIVE);
		break;
	case IXPIO_NO_KEEP_ALIVE:
		off_flag(&dp->flags, IXPIO_FLAG_KEEP_ALIVE);
		break;
	case IXPIO_GET_INFO:
#if 1
                if (copy_from_user(&devinfo, argp, sizeof(ixpio_devinfo_t)))
                {
                        printk("XXX %s ... IXPIO_GET_INFO ... copy_from_user fail\n",__FUNCTION__);
                        return -EFAULT;
                }

                ixpio_copy_devinfo(&devinfo, dp);

                if (copy_to_user(argp, &devinfo, sizeof(ixpio_devinfo_t)))
                {
                        printk("XXX %s ... IXPIO_GET_INFO ... copy_to_user fail\n",__FUNCTION__);
                        return -EFAULT;
                }
#else
		ixpio_copy_devinfo((ixpio_devinfo_t *) ioctl_param, dp);
#endif
		break;
	default:
		return -EINVAL;
	}
	return SUCCESS;
}

void _close(ixpio_devinfo_t * dp)
{
	/* disable all interrupts from board */
	outb(BOARD_IRQ_DISABLE, dp->base + IMCR);

	/* disable this board's DIO */
	outb(BOARD_IO_DISABLE, dp->base + RCR);

	/* release io region */
	release_region(dp->base, IO_RANGE);

	/* uninstall ISR */
	free_irq(dp->irq, dp);

	ixpio_clear_signals(dp);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
int ixpioda16_release(struct inode *inode, struct file *file)
#else
void ixpioda16_release(struct inode *inode, struct file *file)
#endif
{
	/* (export)
	 *
	 * This function is called by ixpio.o whenever a process attempts to
	 * closes the device file. It doesn't have a return value in kernel
	 * version 2.0.x because it can't fail (you must always be able to
	 * close a device).  In version 2.2.x it is allowed to fail.
	 *
	 * Arguments: read <linux/fs.h> for (*release) of struct file_operations
	 *
	 * Returned:  none */

	int minor;
	ixpio_devinfo_t *dp;

	minor = MINOR(inode->i_rdev);
	dp = align_to_minor(minor);

	if (dp) 
	{
		if(file->private_data)
                {
                        file->private_data = NULL;
                        kfree(file->private_data);
                }
	
		dp->open = 0;
		if (!get_flag(&dp->flags, IXPIO_FLAG_KEEP_ALIVE))
			_close(dp);
# if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0) && LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
                MOD_DEC_USE_COUNT;
#endif
	}

# if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
	return 0;
# endif
}

static struct file_operations fops = {
        /* kernel 2.6 prevent the module from unloading while there is a open file(kernel 2.4 use the funciton MOD_INC_USE_COUNT and MOD_DEC_USE_COUNT to protect module from unloading when someone is opening file), so driver writer must set the file_operation's field owner a value 'THIS_MODULE' */
# if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
        owner:THIS_MODULE,
#endif
	open:ixpioda16_open,
	release:ixpioda16_release,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
        unlocked_ioctl:ixpioda16_ioctl
#else
	ioctl:ixpioda16_ioctl
#endif
};

int _open(ixpio_devinfo_t * dp)
{
	/* disable all interrupt from board */
	//disable_irq(dp->irq);
	outb(BOARD_IRQ_DISABLE, dp->base + IMCR);

	/* install ISR */
	if (request_irq(dp->irq, irq_handler, SA_SHIRQ, dp->name, dp)) {
		--(dp->open);
		return -EBUSY;
	}

	/* request io region */
# if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	if (check_region(dp->base, IO_RANGE)) {
		free_irq(dp->irq, dp);
		--(dp->open);
		return -EBUSY;
	}
#endif
	request_region(dp->base, IO_RANGE, MODULE_NAME);

	// enable_irq(dp->irq);

	/* enable this board */
	outb(BOARD_IO_ENABLE, dp->base + RCR);

	return SUCCESS;
}

int ixpioda16_open(struct inode *inode, struct file *file)
{
	/* (export)
	 *
	 * This function is called by ixpio.o whenever a process attempts to open
	 * the device file of PIO-DA16
	 *
	 * Arguments: read <linux/fs.h> for (*open) of struct file_operations
	 *
	 * Returned:  none */

	int minor;
	ixpio_devinfo_t *dp;

	minor = MINOR(inode->i_rdev);
	dp = align_to_minor(minor);

	if (!dp) return -EINVAL;

	++(dp->open);  /* FIXME - should use spin-lock */
	if (dp->open > 1) {
		/* if already opened by someone, get out */
		--(dp->open);
		return -EBUSY;
	}

	if (!get_flag(&dp->flags, IXPIO_FLAG_KEEP_ALIVE))
		if (_open(dp)) return -EBUSY;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
        MOD_INC_USE_COUNT;
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
        ixpio_minor[minor - 1] = minor;
        file->private_data = &ixpio_minor[minor - 1];
#endif

	return SUCCESS;
}

void cleanup_module()
{
	/* cleanup this module */

	ixpio_devinfo_t *dp;

	printk(KERN_INFO FAMILY ": %s ", MODULE_NAME);

	/* uninstall file operations */
	for (dp = dev; dp; dp = dp->next_f) {
		if (get_flag(&dp->flags, IXPIO_FLAG_KEEP_ALIVE))
			_close(dp);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
          cdev_del(dp->cdev);
#else
          dp->fops = 0;
#endif
		clear_data(dp);
		printk(".");
	}

	printk(" has been removed.\n");
}

int init_module()
{
	/* initialize this module
	 *
	 * Arguments: none
	 *
	 * Returned:
	 *   integer 0 means ok, otherwise failed (module can't be load) */

	unsigned int channel;
	ixpio_devinfo_t *dp;
	ixpio_analog_t ao;

	printk(KERN_INFO FAMILY ": %s ", MODULE_NAME);

	/* align to first PIO-DA16 in ixpio list */
	for (dev = ixpio_dev; dev && (dev->csid != PIO_DA16) && (dev->csid != PIO_DA16_T3A) && (dev->csid != PIO_DA16_T3B) ; dev = dev->next);

	if ((dev->csid != PIO_DA16)&&(dev->csid != PIO_DA16_T3A)&&(dev->csid != PIO_DA16_T3B)) {
		printk("fail!\n");
		return FAILURE;
	}

	/* install file operations */
	for (dp = dev; dp; dp = dp->next_f) {

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
                dp->cdev->ops = &fops;
#else
                dp->fops = &fops;
#endif

		/* enable this board */
	        outb(BOARD_IO_ENABLE, dp->base + RCR);

		/* init AO channel is 0V */

	        for(channel = 0; channel < 16; channel++)
	        {
			ao.channel = channel;
		        ao.data.u16 = 0x1fff;
		        analog_out(&ao, dp->base);
	        }

		printk(".");
	}

	printk(" is ready.\n");

	return SUCCESS;
}
/* *INDENT-ON* */
