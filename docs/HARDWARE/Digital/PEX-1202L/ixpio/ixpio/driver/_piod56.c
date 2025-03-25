/* PIO-D56 Service Module.

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

   v 0.3.0  28 Mar 2011 by Golden Wang
     Give support to linux kernel 2.6.37

   v 0.2.0  28 Jan 2007 by Reed Lai
     Give support to linux kernel 2.6.x

   v 0.1.0  8 Aug 2003 by Reed Lai
     Complex signal condition.

   v 0.0.5  3 Jul 2003 by Reed Lai
     Fixed a bug about align_to_minor() that caused the Segmentation fault
     when opening a fake device file.

   v 0.0.4 27 Mar 2003 by Reed Lai
     Fixed the bug that free_irq with different argument to the request_irq.
     That bug caused the kernel was unable to handle a NULL pointer when
     interrupting.

   v 0.0.3 18 Dec 2002 by Reed Lai
     Uses APSR instead of ASR.
     Abendons the use of dp->ipcr.

   v 0.0.2 26 Nov 2002 by Reed Lai
     Corrects the BOTTOM_HELF condition (#if, #ifdef)

   v 0.0.1 2002.11.07 by Reed Lai
     Fixes the interrupt sharing problem which causes the both interrupt
     sharing devices fail to work.  This problem is caused by an unnecesary
     disable_irq before the request_irq.

     Complies to the kernel module license check.
     Uses slab.h in place of malloc.h.
     Corrects the second argument of free_irq.
     Disables board IRQs before request_irq and free_irq.
     Checks the results of request_irq.
     Check_region before request_region.

   v 0.0.0 2002.10.31 by Reed Lai
     Steals body from _piod48.c, blah blah... */

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

/* function copy_from_user() & copy_to_user*/
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,2,0) && LINUX_VERSION_CODE < KERNEL_VERSION(4,10,0)
#include <asm/uaccess.h>
#else   // kernel >= 4.10.0
#include <linux/uaccess.h>
#endif


/* Additional */
#include <linux/fs.h>

/* use I/O ports */
#include <asm/io.h>
#include <linux/ioport.h>

/* need kmalloc */
#include <linux/slab.h>

/* irq service */
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <asm/irq.h>

/* Local */
#include "_piod56.h"
#include "_flags_inline.h"

ixpio_devinfo_t *dev;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
int ixpio_minor[256] = {0};
#endif

#ifdef MODULE_LICENSE
MODULE_AUTHOR("Reed Lai <reed@icpdas.com>");
MODULE_DESCRIPTION("ICPDAS PIO-series driver, PIO-D56/D24 service module");
MODULE_LICENSE(ICPDAS_LICENSE);
#endif							/* MODULE_LICENSE */

/* symbols exportation */
// EXPORT_NO_SYMBOLS; /* do not export symbols */
/*
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
EXPORT_SYMBOL_GPL(ixpiod56_ioctl);
EXPORT_SYMBOL_GPL(ixpiod56_release);
EXPORT_SYMBOL_GPL(ixpiod56_open);
#else
EXPORT_SYMBOL_NOVERS(ixpiod56_ioctl);
EXPORT_SYMBOL_NOVERS(ixpiod56_release);
EXPORT_SYMBOL_NOVERS(ixpiod56_open);
#endif
*/

unsigned int eoi(ixpio_devinfo_t * dp);
irqreturn_t irq_handler(int irq, void *dev_id);
int write_reg(ixpio_reg_t * reg, unsigned long int base);
int read_reg(ixpio_reg_t * reg, unsigned long int base);
ixpio_devinfo_t *align_to_minor(int minor);
void _close(ixpio_devinfo_t * dp);
int _open(ixpio_devinfo_t * dp);

unsigned int eoi(ixpio_devinfo_t * dp)
{
	/* edge of interrupt
	 *
	 * The interrupt output signal of PIO series behaves level-trigger and
	 * active-low on /INT of PCI bus.  This procedure detects the status of
	 * interrupt source (P2C0/1/2/3) from APSR (Aux Status Register,)
	 * and turns the level-triggered interrupt to P/N edge-triggered
	 * interrupt by toggles the bit of IPCR (INT polarity control register)
	 * to prevent interrupt storm.
	 *
	 * Arguments:
	 *   di       pointer to device information
	 *
	 * Returned:  an unsigned int value for interrupt source */

	unsigned int apsr;			/* AUX Status Register */
	unsigned int imcr;
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

#ifdef BOTTOM_HELF
	static struct tq_struct task = { NULL, 0, irq_handler_bh, NULL };

	task.data = dev_id;
#endif							/* BOTTOM_HELF */

	is = eoi((ixpio_devinfo_t *) dev_id);

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

int write_reg(ixpio_reg_t * reg, unsigned long int base)
{
	/* Write to register
	 *
	 * Arguments:
	 *   reg      pointer to a structure ixpio_reg for register
	 *   base     base address for current device
	 *
	 * Returned:  SUCCESS or FAILURE */

	unsigned int lbyte, hbyte;

	//printk("XXX %s ... reg->id = %d\n", __FUNCTION__, reg->id);
	//printk("XXX %s ... reg->value = %d\n", __FUNCTION__, reg->value);

	switch (reg->id) {
	case IXPIO_RCR:			/* Reset Control Register */
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
	case IXPIO_APSR:			/* AUX Status Register */
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
	case IXPIO_P0:				/* Port 0 */
#if PORT0_MASK == 0xff
		outb(((unsigned char) reg->value), base + PORT0);
#else
		outb(((unsigned char) reg->value) & PORT0_MASK, base + PORT0);
#endif
		break;
	case IXPIO_P1:				/* Port 1 */
#if PORT1_MASK == 0xff
		outb(((unsigned char) reg->value), base + PORT1);
#else
		outb(((unsigned char) reg->value) & PORT1_MASK, base + PORT1);
#endif
		break;
	case IXPIO_P2:				/* Port 2 */
#if PORT2_MASK == 0xff
		outb(((unsigned char) reg->value), base + PORT2);
#else
		outb(((unsigned char) reg->value) & PORT2_MASK, base + PORT2);
#endif
		break;
	case IXPIO_PC:				/* Port Configuration */
#if PORTCONF_MASK == 0xff
		outb(((unsigned char) reg->value), base + PORTCONF);
#else
		outb(((unsigned char) reg->value) & PORTCONF_MASK,
			 base + PORTCONF);
#endif
		break;
	case IXPIO_CON1L:			/* CON1 Low Byte */
#if CON1L_MASK == 0xff
		outb(((unsigned char) reg->value), base + CON1L);
#else
		outb(((unsigned char) reg->value) & CON1L_MASK, base + CON1L);
#endif
		break;
	case IXPIO_CON1H:			/* CON1 High Byte */
#if CON1H_MASK == 0xff
		outb(((unsigned char) reg->value), base + CON1H);
#else
		outb(((unsigned char) reg->value) & CON1H_MASK, base + CON1H);
#endif
		break;
	case IXPIO_DO:				/* CON1 Output */
	case IXPIO_DIO:
		lbyte = (unsigned char) reg->value;
		hbyte = (unsigned char) (reg->value >> 8);
#if CON1L_MASK == 0xff
		outb(lbyte, base + CON1L);
#else
		outb(lbyte & CON1L_MASK, base + CON1L);
#endif
#if CON1H_MASK == 0xff
		outb(hbyte, base + CON1H);
#else
		outb(hbyte & CON1H_MASK, base + CON1H);
#endif
		break;
	default:
		return FAILURE;
	}
	return SUCCESS;
}

int read_reg(ixpio_reg_t * reg, unsigned long int base)
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
	case IXPIO_APSR:			/* AUX Status Register */
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
	case IXPIO_P0:				/* Port 0 */
#if PORT0_MASK == 0xff
		reg->value = inb(base + PORT0);
#else
		reg->value = inb(base + PORT0) & PORT0_MASK;
#endif
		break;
	case IXPIO_P1:				/* Port 1 */
#if PORT1_MASK == 0xff
		reg->value = inb(base + PORT1);
#else
		reg->value = inb(base + PORT1) & PORT1_MASK;
#endif
		break;
	case IXPIO_P2:				/* Port 2 */
#if PORT2_MASK == 0xff
		reg->value = inb(base + PORT2);
#else
		reg->value = inb(base + PORT2) & PORT2_MASK;
#endif
		break;
	case IXPIO_PC:				/* Port Configuration */
#if PORTCONF_MASK == 0xff
		reg->value = inb(base + PORTCONF);
#else
		reg->value = inb(base + PORTCONF) & PORTCONF_MASK;
#endif
		break;
	case IXPIO_CON2L:			/* CON2 Low Byte */
#if CON2L_MASK == 0xff
		reg->value = inb(base + CON2L);
#else
		reg->value = inb(base + CON2L) & CON2L_MASK;
#endif
		break;
	case IXPIO_CON2H:			/* CON2 High Byte */
#if CON2H_MASK == 0xff
		reg->value = inb(base + CON2H);
#else
		reg->value = inb(base + CON2H) & CON2H_MASK;
#endif
		break;
	case IXPIO_DI:				/* CON2 Digital Input */
	case IXPIO_DIO:
		lbyte = (unsigned char) reg->value;
		hbyte = (unsigned char) (reg->value >> 8);
#if CON2H_MASK == 0xff
		lbyte = inb(base + CON2L);
#else
		lbyte = inb(base + CON2L) & CON2L_MASK;
#endif
#if CON2L_MASK == 0xff
		hbyte = inb(base + CON2H);
#else
		hbyte = inb(base + CON2H) & CON2H_MASK;
#endif
		reg->value = (hbyte << 8) + lbyte;
		break;
	default:
		return FAILURE;
	}
	//printk("XXX %s ... reg->id = %d\n", __FUNCTION__, reg->id);
	//printk("XXX %s ... reg->value = %d\n", __FUNCTION__, reg->value);
	return SUCCESS;
}

ixpio_devinfo_t *align_to_minor(int minor)
{
	ixpio_devinfo_t *p;
	for (p = dev; p && p->no != minor; p = p->next_f);
	return p;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
long ixpiod56_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param)
#else
int ixpiod56_ioctl(struct inode *inode, struct file *file, unsigned int ioctl_num, unsigned long ioctl_param)
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
	unsigned int sig_num;
	//SMMRegisters regs;
        //SMMRegisters __user *argp = (SMMRegisters __user *)arg;

/*
        if (copy_from_user(&regs, argp, sizeof(SMMRegisters)))
                return -EFAULT;

        if (!argp)
                return -EINVAL;
*/

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

        if (!argp)
                return -EINVAL;

	if (!dp || !dp->open)
		return -EINVAL;

	switch (ioctl_num) {
	case IXPIO_SIG:
		ixpio_clear_signals(dp);
#if 1
        	if (copy_from_user(&signal, argp, sizeof(ixpio_signal_t)))
		{
			printk("XXX %s ... IXPIO_SIG ... copy_from_user fail\n", __FUNCTION__);
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
			printk("XXX %s ... IXPIO_SIG_ADD ... copy_from_user fail\n",__FUNCTION__);
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
			printk("XXX %s ... IXPIO_SIG_DEL ... copy_from_user fail\n",__FUNCTION__);
                	return -EFAULT;
		}

		if (ixpio_del_signal(sig_num, dp))
			return FAILURE;
#else
		if (ixpio_del_signal((unsigned int) ioctl_param, dp))
			return FAILURE;
#endif
		break;
	case IXPIO_SIG_DEL_ALL:
		ixpio_clear_signals(dp);
		break;
	case IXPIO_REG_READ:
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
	case IXPIO_REG_WRITE:
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

	/* disable this board's digital IO */
	outb(BOARD_DIO_DISABLE, dp->base + RCR);

	/* release IO region */
	release_region(dp->base, IO_RANGE);

	/* uninstall ISR */
	free_irq(dp->irq, dp);

	ixpio_clear_signals(dp);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
int ixpiod56_release(struct inode *inode, struct file *file)
#else
void ixpiod56_release(struct inode *inode, struct file *file)
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
	open:ixpiod56_open,
	release:ixpiod56_release,

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
        unlocked_ioctl:ixpiod56_ioctl
#else
	ioctl:ixpiod56_ioctl
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
	outb(BOARD_DIO_ENABLE, dp->base + RCR);

	return SUCCESS;
}

int ixpiod56_open(struct inode *inode, struct file *file)
{
	/* (export)
	 *
	 * This function is called by ixpio.o whenever a process attempts to open
	 * the device file of PIO-D56
	 *
	 * Arguments: read <linux/fs.h> for (*open) of struct file_operations
	 *
	 * Returned:  none */

	int minor;
	ixpio_devinfo_t *dp;

	minor = MINOR(inode->i_rdev);
	dp = align_to_minor(minor);

	if (!dp) return -EINVAL;

	++(dp->open);
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
	/* cleanup this module
	 *
	 * Arguments: none
	 *
	 * Returned:  none */

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
		// ixpio_clear_signals(dp);
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

	ixpio_devinfo_t *dp;

	printk(KERN_INFO FAMILY ": %s ", MODULE_NAME);

	/* align to first PIO-D56 in ixpio list */
	for (dev = ixpio_dev; dev && (dev->csid != PIO_D56)&&(dev->csid != PIO_D56_T3A)&&(dev->csid != PIO_D56_T3B); dev = dev->next);

	if ((dev->csid != PIO_D56)&&(dev->csid != PIO_D56_T3A)&&(dev->csid != PIO_D56_T3B)) {
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
		printk(".");
	}

	printk(" is ready.\n");

	return SUCCESS;
}
