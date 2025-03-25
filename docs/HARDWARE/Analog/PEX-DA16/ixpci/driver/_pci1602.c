/* PCI-1602 Service Module

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
   v 0.6.1 11 Nov 2014 by Winson Chen
     Give support to linux kernel 3.16.0.
     Remove EXPORT_SYMBOL_GPL().

   v 0.6.0 1 Apr 2011 by Golden Wang
     Give support to linux kernel 2.6.37.

   v 0.5.1 16 May 2007 by Golden Wang
     Include "linux/cdev.h" after including "ixpci.h".

   v 0.5.0  8 Feb 2007 by Golden Wang
     Give support to linux kernel 2.6.x.

   v 0.4.0  3 Sep 2003 by Emmy Tsai
     Gives support to PCI-1602 (new version).

   v 0.3.2  8 Jul 2003 by Reed Lai
     Fixed a bug about _align_minor().

   v 0.3.1 16 Jan 2003 by Reed Lai
     Fixed the request io region bug that hanged the system.

   v 0.3.0  8 Nov 2003 by Reed Lai
     Uses dp->range[] instead of IO_RANGE.

   v 0.2.0 11 Nov 2002 by Reed Lai
     Checks IO region before request.
     Uses slab.h in place of malloc.h.
     Complies to the kernel module license check.

   v 0.1.1 16 May 2002 by Reed Lai
     Remove unused items: msg_ptr and msg[]

   v 0.1.0 25 Oct 2001 by Reed Lai
     Re-filename to _pci1602.c (from pdaq1602.c).
     Change all "pdaq" to "ixpci."

   v 0.0.0 24 May 2001 by Reed Lai
     create, blah blah... */

/* Mandatory */
#include <linux/kernel.h>		/* ta, kernel work */
#include <linux/module.h>		/* is a module */
#include "ixpci.h"

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

/* User space memory access */
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

/* Local matter */
#include "_pci1602.h"

ixpci_devinfo_t *dev;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
int ixpci_minor[256] = {0};
#endif

#ifdef MODULE_LICENSE
MODULE_AUTHOR("Reed Lai <reed@icpdas.com>");
MODULE_DESCRIPTION
	("ICPDAS PCI-series driver, PCI-1602 service module");
MODULE_LICENSE(ICPDAS_LICENSE);
#endif

// EXPORT_NO_SYMBOLS; /* do not export symbols */
/*
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
EXPORT_SYMBOL_GPL(ixpci1602_ioctl);
EXPORT_SYMBOL_GPL(ixpci1602_release);
EXPORT_SYMBOL_GPL(ixpci1602_open);
#else
EXPORT_SYMBOL_NOVERS(ixpci1602_ioctl);
EXPORT_SYMBOL_NOVERS(ixpci1602_release);
EXPORT_SYMBOL_NOVERS(ixpci1602_open);
#endif
*/
int _write_reg(ixpci_reg_t * reg, unsigned long int base[])
{
	/* Write to register
	 *
	 * Arguments:
	 *   reg      pointer to a structure ixpci_reg for register
	 *   base     base address for current device
	 *
	 * Returned:  SUCCESS or FAILURE */

	switch (reg->id) {
	case IXPCI_8254C0:
#if _8254C0_MASK == 0xffff
		outw((reg->value), base[1] + _8254C0);
#else
		outw((reg->value) & _8254C0_MASK, base[1] + _8254C0);
#endif
		break;
	case IXPCI_8254C1:
#if _8254C1_MASK == 0xffff
		outw((reg->value), base[1] + _8254C1);
#else
		outw((reg->value) & _8254C1_MASK, base[1] + _8254C1);
#endif
		break;
	case IXPCI_8254C2:
#if _8254C2_MASK == 0xffff
		outw((reg->value), base[1] + _8254C2);
#else
		outw((reg->value) & _8254C2_MASK, base[1] + _8254C2);
#endif
		break;
	case IXPCI_8254CR:
#if _8254CR_MASK == 0xffff
		outw((reg->value), base[1] + _8254CR);
#else
		outw((reg->value) & _8254CR_MASK, base[1] + _8254CR);
#endif
		break;
	case IXPCI_CR:
#if _CR_MASK == 0xffff
		outw((reg->value), base[2] + _CR);
#else
		outw((reg->value) & _CR_MASK, base[2] + _CR);
#endif
		break;
	case IXPCI_ADST:
#if _ADST_MASK == 0xffff
		outw((reg->value), base[2] + _ADST);
#else
		outw((reg->value) & _ADST_MASK, base[2] + _ADST);
#endif
		break;
	case IXPCI_DO:
#if _DO_MASK == 0xffff
		outw((reg->value), base[3] + _DO);
#else
		outw((reg->value) & _DO_MASK, base[3] + _DO);
#endif
		break;
	case IXPCI_DA1:
#if _DA1_MASK == 0xffff
		outw((reg->value), base[4] + _DA1);
#else
		outw((reg->value) & _DA1_MASK, base[4] + _DA1);
#endif
		break;
	case IXPCI_DA2:
#if _DA2_MASK == 0xffff
		outw((reg->value), base[4] + _DA2);
#else
		outw((reg->value) & _DA2_MASK, base[4] + _DA2);
#endif
		break;
	default:
		return FAILURE;
	}
	return SUCCESS;
}

int _read_reg(ixpci_reg_t * reg, unsigned long int base[])
{
	/* Read from register
	 *
	 * Arguments:
	 *   reg      pointer to structure ixpci_reg for register
	 *   base     base address for current device
	 *
	 * Returned:  SUCCESS or FAILURE */

	switch (reg->id) {
	case IXPCI_8254C0:
#if _8254C0_MASK == 0xffff
		reg->value = inw(base[1] + _8254C0);
#else
		reg->value = inw(base[1] + _8254C0) & _8254C0_MASK;
#endif
		break;
	case IXPCI_8254C1:
#if _8254C1_MASK == 0xffff
		reg->value = inw(base[1] + _8254C1);
#else
		reg->value = inw(base[1] + _8254C1) & _8254C1_MASK;
#endif
		break;
	case IXPCI_8254C2:
#if _8254C2_MASK == 0xffff
		reg->value = inw(base[1] + _8254C2);
#else
		reg->value = inw(base[1] + _8254C2) & _8254C2_MASK;
#endif
		break;
	case IXPCI_SR:
#if _SR_MASK == 0xffff
		reg->value = inw(base[2] + _SR);
#else
		reg->value = inw(base[2] + _SR) & _SR_MASK;
#endif
		break;
	case IXPCI_DI:
#if _DI_MASK == 0xffff
		reg->value = inw(base[3] + _DI);
#else
		reg->value = inw(base[3] + _DI) & _DI_MASK;
#endif
		break;
	case IXPCI_AD:
#if _AD_MASK == 0xffff
		reg->value = inw(base[4] + _AD);
#else
		reg->value = inw(base[4] + _AD) & _AD_MASK;
#endif
		break;
	default:
		return FAILURE;
	}
	return SUCCESS;
}

ixpci_devinfo_t *_align_minor(int minor)
{
	/* align to device by minor number */

	ixpci_devinfo_t *dp;
	for (dp = dev; dp && dp->no != minor; dp = dp->next_f);
	return dp;
}

int _time_span(int span, unsigned long base[])
{
	/* Use the 8254 counter-2 to be the machine independent timer
	 * at the 8 MHz clock.
	 *
	 * Arguments:
	 *   span     micro-second (us) to be spanned
	 *   base     base address for current device
	 *
	 * Returned:  SUCCESS or FAILURE */

	unsigned long sr;
	unsigned counter;
	unsigned char lbyte, hbyte;
	int i;

	if ((span > 8190) || (span == 0))
		return FAILURE;

	i = 0;
	sr = base[2] + _SR;

	counter = span * 8;
	lbyte = counter & 0xff;
	hbyte = (counter >> 8) & 0xff;

	outb(0xb0, base[1] + _8254CR);
	outb(lbyte, base[1] + _8254C2);
	outb(hbyte, base[1] + _8254C2);

	while (inb(sr) & 0x01) {
		if (i > 100000000)
			return FAILURE;
		/* XXX - fix me for timeout */
		++i;
	}
	return SUCCESS;
}

int _reset_dev(ixpci_devinfo_t * dp)
{
	/* stop timer 0 */
	outb(0x34, dp->base[1] + _8254CR);
	outb(0x01, dp->base[1] + _8254C0);
	outb(0x00, dp->base[1] + _8254C0);

	/* stop timer 1 */
	outb(0x74, dp->base[1] + _8254CR);
	outb(0x01, dp->base[1] + _8254C1);
	outb(0x00, dp->base[1] + _8254C1);

	/* stop timer 2 */
	outb(0xb0, dp->base[1] + _8254CR);
	outb(0x01, dp->base[1] + _8254C2);
	outb(0x00, dp->base[1] + _8254C2);

	/* reset control register to
	   A/D channel 0
	   Gain control PGA = 1
	   Input range control = PGA (+/- 5V)
	   Reset the MagicScan controller
	   Assert the MagicScan handshake control bit (bit 13)
	   Clear FIFO */
	outw(0x2000, dp->base[2] + _CR);

	/* clear DO */
	outw(0, dp->base[3] + _DO);

	/* clear DA */
	outw(0, dp->base[4] + _DA1);
	outw(0, dp->base[4] + _DA2);

	/* did I leak anything? */

	return SUCCESS;
}

#if 0							/* The mysterious old initial codes for Win/DOS */
int _reset_dev(ixpci_devinfo_t * dp)
{
	int i;
	unsigned long cr, sr;

	cr = dp->base[2] + _CR;
	sr = dp->base[2] + _SR;

	/* check PIC, send recovery */
	if ((inb(sr) & 0x04) == 0)
		outw(0xffff, cr);
	i = 0;
	while ((inb(sr) & 0x04) == 0) {
		/* XXX - fix me for timeout */
		if (i++ > 65535) {
			printk("*** PIC recovery timeout ***\n");
			return FAILURE;
		}
	}

#if 0
	/* This section always timeout, so comment out */
	/* set PIC low */
	outw(0xc000, cr);
	i = 0;
	while ((inb(sr) & 0x04) != 0) {
		/* XXX - fix me for timeout */
		if (i++ > 65535) {
			printk("*** PIC low timeout ***\n");
			//break;
			return FAILURE;
		}
	}
#endif

	/* set PIC high */
	outw(0xe000, cr);
	i = 0;
	while ((inb(sr) & 0x04) == 0) {
		/* XXX - fix me for timeout */
		if (i++ > 65535) {
			printk("*** PIC high timeout ***\n");
			return FAILURE;
		}
	}
	return SUCCESS;
}
#endif							/* The mysterious old initial codes for Win/DOS */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
long ixpci1602_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param)
#else
int ixpci1602_ioctl(struct inode *inode, struct file *file, unsigned int ioctl_num, unsigned long ioctl_param)
#endif
{
	/* (export)
	 *
	 * This function is called by ixpci.o whenever a process tries
	 * to do and IO control on IXPCI device file
	 *
	 * Arguments: read <linux/fs.h> for (*ioctl) of struct file_operations
	 *
	 * Returned:  SUCCESS or FAILED */

	ixpci_devinfo_t *dp, devinfo;
	void __user *argp = (void __user*)ioctl_param;
	ixpci_reg_t ixpci_reg;
	int span;

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

        dp = _align_minor(minor);
#else
        dp = _align_minor(MINOR(inode->i_rdev));
#endif

	if (!dp || !dp->open) return -EINVAL;

	switch (ioctl_num) {
	case IXPCI_GET_INFO:
#if 1
                if(copy_from_user(&devinfo, argp, sizeof(ixpci_devinfo_t)))
                {
                        printk("XXX %s ... IXPCI_GET_INFO ... copy_from_user fail\n",__FUNCTION__);
                        return -EFAULT;
                }

                ixpci_copy_devinfo(&devinfo, dp);

                if(copy_to_user(argp, &devinfo, sizeof(ixpci_devinfo_t)))
                {
                        printk("XXX %s ... IXPCI_GET_INFO ... copy_to_user fail\n",__FUNCTION__);
                        return -EFAULT;
                }
#else
		ixpci_copy_devinfo((ixpci_devinfo_t *) ioctl_param, dp);
#endif
		break;
	case IXPCI_READ_REG:
#if 1
                if(copy_from_user(&ixpci_reg, argp, sizeof(ixpci_reg_t)))
                {
                        printk("XXX %s ... IXPCI_READ_REG ... copy_from_user fail\n",__FUNCTION__);
                        return -EFAULT;
                }

                if (_read_reg(&ixpci_reg, dp->base))
                        return FAILURE;

                if(copy_to_user(argp, &ixpci_reg, sizeof(ixpci_reg_t)))
                {
                        printk("XXX %s ... IXPCI_READ_REG ... copy_to_user fail\n",__FUNCTION__);
                        return -EFAULT;
                }
#else
		if (_read_reg((ixpci_reg_t *) ioctl_param, dp->base))
			return FAILURE;
#endif
		break;
	case IXPCI_WRITE_REG:
#if 1
                if(copy_from_user(&ixpci_reg, argp, sizeof(ixpci_reg_t)))
                {
                        printk("XXX %s ... IXPCI_WRITE_REG ... copy_from_user fail\n",__FUNCTION__);
                        return -EFAULT;
                }

                if (_write_reg(&ixpci_reg, dp->base))
#else
		if (_write_reg((ixpci_reg_t *) ioctl_param, dp->base))
#endif
			return FAILURE;
		break;
	case IXPCI_TIME_SPAN:
#if 1
                if(copy_from_user(&span, argp, sizeof(int)))
                {
                        printk("XXX %s ... IXPCI_WRITE_REG ... copy_from_user fail\n",__FUNCTION__);
                        return -EFAULT;
                }

                if (_time_span(span, dp->base))
#else
		if (_time_span((int) ioctl_param, dp->base))
#endif
			return FAILURE;
		break;
	case IXPCI_RESET:
		if (_reset_dev(dp))
			return FAILURE;
		break;
	default:
		return -EINVAL;
	}
	return SUCCESS;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
int ixpci1602_release(struct inode *inode, struct file *file)
#else
void ixpci1602_release(struct inode *inode, struct file *file)
#endif
{
	/* (export)
	 *
	 * This function is called by ixpci.o whenever a process attempts to
	 * closes the device file. It doesn't have a return value in kernel
	 * version 2.0.x because it can't fail (you must always be able to
	 * close a device).  In version 2.2.x it is allowed to fail.
	 *
	 * Arguments: read <linux/fs.h> for (*release) of struct file_operations
	 *
	 * Returned:  none */

	int minor, i;
	ixpci_devinfo_t *dp;

	minor = MINOR(inode->i_rdev);
	dp = _align_minor(minor);
	
	if (dp) 
	{
		if(file->private_data)
                {
                        file->private_data = NULL;
                        kfree(file->private_data);
                }

		dp->open = 0;
		for (i = 0; i < PBAN; i++)
		{
			if(dp->base[i])
				release_region(dp->base[i], dp->range[i]);
		}
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
	open:ixpci1602_open,
	release:ixpci1602_release,


# if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
	unlocked_ioctl:ixpci1602_ioctl,
#else
	ioctl:ixpci1602_ioctl,
#endif
};

int ixpci1602_open(struct inode *inode, struct file *file)
{
	/* (export)
	 *
	 * This function is called by ixpci.o whenever a process attempts to
	 * open the device file of PCI-1602
	 *
	 * Arguments: read <linux/fs.h> for (*open) of struct file_operations
	 *
	 * Returned:  none */

	int minor, i;
	ixpci_devinfo_t *dp;

	minor = MINOR(inode->i_rdev);
	dp = _align_minor(minor);

	if (!dp) return -EINVAL;

	++(dp->open);
	if (dp->open > 1) {
		--(dp->open);
		return -EBUSY;
		/* if still opened by someone, get out */
	}

	/* request io region */
	for (i = 0; i < PBAN; i++)
	{
# if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	  if (!dp->base[i] || !dp->range[i] || check_region(dp->base[i], dp->range[i]))
			continue;
#endif
 	  request_region(dp->base[i], dp->range[i], MODULE_NAME);
	}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
        MOD_INC_USE_COUNT;
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
        ixpci_minor[minor - 1] = minor;
        file->private_data = &ixpci_minor[minor - 1];
#endif

	return SUCCESS;
}

void cleanup_module()
{
	/* cleanup this module */

	ixpci_devinfo_t *dp;

	KMSG("%s ", MODULE_NAME);

	for (dp = dev; dp; dp = dp->next_f) {
		_reset_dev(dp);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
          cdev_del(dp->cdev);
#else
          dp->fops = 0;
#endif
		/* remove file operations */
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
	 * integer 0 for ok, otherwise failed (module can't be load) */

	ixpci_devinfo_t *dp;

	KMSG("%s ", MODULE_NAME);

	/* align to first PCI-1602 in ixpci list */
	for (dev = ixpci_dev; dev && dev->id != PCI_1602_A; dev = dev->next);

	if (!dev) {
		printk("fail!\n");
		return FAILURE;
	}

	/* initiate for each device (card) in family */
	for (dp = dev; dp; dp = dp->next_f) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
                dp->cdev->ops = &fops;
#else
                dp->fops = &fops;
#endif
		_reset_dev(dp);
		printk(".");
	}

	printk(" ready.\n");
	return SUCCESS;
}
