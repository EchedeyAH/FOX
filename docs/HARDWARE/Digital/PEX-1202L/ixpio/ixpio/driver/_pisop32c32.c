/* PISO-P32C32 Service Module.

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

   v 0.2.1  15 May 2007 by Golden Wang
     Fixed a bug about init_module() that caused the fail
     when installing the PISO_P32C32_T3B Card.

   v 0.2.0  28 Jan 2007 by Reed Lai
     Give support to linux kernel 2.6.x

   v 0.1.3  3 Jul 2003 by Reed Lai
     Fixed a bug about align_to_minor() that caused the Segmentation fault
     when opening a fake device file.

   v 0.1.2 18 Dec 2002 by Reed Lai
     Removes the IPCR entity.

   v 0.1.1  7 Nov 2002 by Reed Lai
     Complies to the kernel module license check.
     Uses slab.h in place of malloc.h.
     Check_region before request_region.

   v 0.1.0  4 Jun 2002 by Reed Lai
     Refine some codes.

   v 0.0.0  3 May 2002 by Reed Lai
     Create. */

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

/* function copy_from_user() */
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

/* Local */
#include "_pisop32c32.h"

ixpio_devinfo_t *dev;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
int ixpio_minor[256] = {0};
#endif

#ifdef MODULE_LICENSE
MODULE_AUTHOR("Reed Lai <reed@icpdas.com>");
MODULE_DESCRIPTION("ICPDAS PIO-series driver, PISO-P32C32 service module");
MODULE_LICENSE(ICPDAS_LICENSE);
#endif							/* MODULE_LICENSE */

/* symbols exportation */
// EXPORT_NO_SYMBOLS; /* do not export symbols */
/*
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
EXPORT_SYMBOL_GPL(ixpisop32c32_ioctl);
EXPORT_SYMBOL_GPL(ixpisop32c32_release);
EXPORT_SYMBOL_GPL(ixpisop32c32_open);
#else
EXPORT_SYMBOL_NOVERS(ixpisop32c32_ioctl);
EXPORT_SYMBOL_NOVERS(ixpisop32c32_release);
EXPORT_SYMBOL_NOVERS(ixpisop32c32_open);
#endif
*/

int write_reg(ixpio_reg_t * reg, unsigned long int base);
int read_reg(ixpio_reg_t * reg, unsigned long int base);
ixpio_devinfo_t *align_to_minor(int minor);

int write_reg(ixpio_reg_t * reg, unsigned long int base)
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
	case IXPIO_DIO_A:			/* DO0 ~ DO7 */
#if DIO_A_MASK == 0xff
		outb(((unsigned char) reg->value), base + DIO_A);
#else
		outb(((unsigned char) reg->value) & DIO_A_MASK, base + DIO_A);
#endif
		break;
	case IXPIO_DIO_B:			/* DO8 ~ DO15 */
#if DIO_B_MASK == 0xff
		outb(((unsigned char) reg->value), base + DIO_B);
#else
		outb(((unsigned char) reg->value) & DIO_B_MASK, base + DIO_B);
#endif
		break;
	case IXPIO_DIO_C:			/* DO16 ~ DO23 */
#if DIO_C_MASK == 0xff
		outb(((unsigned char) reg->value), base + DIO_C);
#else
		outb(((unsigned char) reg->value) & DIO_C_MASK, base + DIO_C);
#endif
		break;
	case IXPIO_DIO_D:			/* DO24 ~ DO31 */
#if DIO_D_MASK == 0xff
		outb(((unsigned char) reg->value), base + DIO_D);
#else
		outb(((unsigned char) reg->value) & DIO_D_MASK, base + DIO_D);
#endif
		break;
	case IXPIO_DO:
	case IXPIO_DIO:			/* DO0 - DO31 */
#if DIO_A_MASK == 0xff
		outb((unsigned char) reg->value, base + DIO_A);
#else
		outb(((unsigned char) reg->value) & DIO_A_MASK, base + DIO_A);
#endif
#if DIO_B_MASK == 0xff
		outb((unsigned char) (reg->value >> 8), base + DIO_B);
#else
		outb(((unsigned char) (reg->value >> 8)) & DIO_B_MASK,
			 base + DIO_B);
#endif
#if DIO_C_MASK == 0xff
		outb((unsigned char) (reg->value >> 16), base + DIO_C);
#else
		outb(((unsigned char) (reg->value >> 16)) & DIO_C_MASK,
			 base + DIO_C);
#endif
#if DIO_D_MASK == 0xff
		outb((unsigned char) (reg->value >> 24), base + DIO_D);
#else
		outb(((unsigned char) (reg->value >> 24)) & DIO_D_MASK,
			 base + DIO_D);
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

	unsigned int abyte, bbyte, cbyte, dbyte;

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
	case IXPIO_DIO_A:			/* DI0 ~ DI7 */
#if DIO_A_MASK == 0xff
		reg->value = inb(base + DIO_A);
#else
		reg->value = inb(base + DIO_A) & DIO_A_MASK;
#endif
		break;
	case IXPIO_DIO_B:			/* DI8 ~ DI15 */
#if DIO_B_MASK == 0xff
		reg->value = inb(base + DIO_B);
#else
		reg->value = inb(base + DIO_B) & DIO_B_MASK;
#endif
		break;
	case IXPIO_DIO_C:			/* DI16 ~ DI23 */
#if DIO_C_MASK == 0xff
		reg->value = inb(base + DIO_C);
#else
		reg->value = inb(base + DIO_C) & DIO_C_MASK;
#endif
		break;
	case IXPIO_DIO_D:			/* DI24 ~ DI31 */
#if DIO_D_MASK == 0xff
		reg->value = inb(base + DIO_D);
#else
		reg->value = inb(base + DIO_D) & DIO_D_MASK;
#endif
		break;
	case IXPIO_DI:
	case IXPIO_DIO:			/* DI0 - DI15 */
#if DIO_A_MASK == 0xff
		abyte = inb(base + DIO_A);
#else
		abyte = inb(base + DIO_A) & DIO_A_MASK;
#endif
#if DIO_B_MASK == 0xff
		bbyte = inb(base + DIO_B);
#else
		bbyte = inb(base + DIO_B) & DIO_B_MASK;
#endif
#if DIO_C_MASK == 0xff
		cbyte = inb(base + DIO_C);
#else
		cbyte = inb(base + DIO_C) & DIO_C_MASK;
#endif
#if DIO_D_MASK == 0xff
		dbyte = inb(base + DIO_D);
#else
		dbyte = inb(base + DIO_D) & DIO_D_MASK;
#endif
		reg->value = (dbyte << 24) + (cbyte << 16) + (bbyte << 8) + abyte;
		break;
	default:
		return FAILURE;
	}
	return SUCCESS;
}

ixpio_devinfo_t *align_to_minor(int minor)
{
	ixpio_devinfo_t *p;
	for (p = dev; p && p->no != minor; p = p->next_f);
	return p;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
long ixpisop32c32_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param)
#else
int ixpisop32c32_ioctl(struct inode *inode, struct file *file, unsigned int ioctl_num, unsigned long ioctl_param)
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
	ixpio_reg_t reg;

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

	if (!dp || !dp->open)
		return -EINVAL;

	switch (ioctl_num) {
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
	case IXPIO_READ_REG:
#if 1
		if (copy_from_user(&reg, argp, sizeof(ixpio_reg_t)))
		{
			printk("XXX %s ... IXPIO_READ_REG ... copy_from_user fail\n", __FUNCTION__);
			return -EFAULT;
		}

		if (read_reg(&reg, dp->base))
			return FAILURE;

		if (copy_to_user(argp, &reg, sizeof(ixpio_reg_t)))
		{
			printk("XXX %s ... IXPIO_READ_REG ... copy_to_user fail\n", __FUNCTION__);
			return -EFAULT;
		}
#else
		if (read_reg((ixpio_reg_t *) ioctl_param, dp->base))
			return FAILURE;
#endif
		break;
	case IXPIO_WRITE_REG:
#if 1
		if (copy_from_user(&reg, argp, sizeof(ixpio_reg_t)))
		{
			printk("XXX %s ... IXPIO_WRITE_REG ... copy_from_user fail\n",__FUNCTION__);
			return -EFAULT;
		}

		if (write_reg(&reg, dp->base))
			return FAILURE;
#else
		if (write_reg((ixpio_reg_t *) ioctl_param, dp->base))
			return FAILURE;
#endif
		break;
	default:
		return -EINVAL;
	}
	return SUCCESS;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
int ixpisop32c32_release(struct inode *inode, struct file *file)
#else
void ixpisop32c32_release(struct inode *inode, struct file *file)
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
		outb(BOARD_DIO_DISABLE, dp->base + RCR);
		release_region(dp->base, IO_RANGE);
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
	open:ixpisop32c32_open,
	release:ixpisop32c32_release,

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
        unlocked_ioctl:ixpisop32c32_ioctl
#else
	ioctl:ixpisop32c32_ioctl
#endif
};

int ixpisop32c32_open(struct inode *inode, struct file *file)
{
	/* (export)
	 *
	 * This function is called by ixpio.o whenever a process attempts to open
	 * the device file of PISO-P32C32
	 *
	 * Arguments: read <linux/fs.h> for (*open) of struct file_operations
	 *
	 * Returned:  none */

	int minor;
	ixpio_devinfo_t *dp;

	minor = MINOR(inode->i_rdev);
	dp = align_to_minor(minor);

	if (!dp)
		return -EINVAL;

	++(dp->open);
	if (dp->open > 1) {
		/* if already opened by someone, get out */
		--(dp->open);
		return -EBUSY;
	}

	/* request io region */
# if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	if (check_region(dp->base, IO_RANGE)) {
		--(dp->open);
		return -EBUSY;
	}
#endif
	request_region(dp->base, IO_RANGE, MODULE_NAME);

	/* enable this board's DIO */
	outb(BOARD_DIO_ENABLE, dp->base + RCR);

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
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
          cdev_del(dp->cdev);
#else
          dp->fops = 0;
#endif
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
	 *     integer 0 means ok, otherwise failed (module can't be load) */

	ixpio_devinfo_t *dp;

	printk(KERN_INFO FAMILY ": %s ", MODULE_NAME);

	/* align to first PISO-P32C32 in ixpio list */
	for (dev = ixpio_dev; dev && (dev->csid != PISO_P32C32)&&(dev->csid != PISO_P32C32_T3A)&&(dev->csid != PISO_P32C32_T3B);
		 dev = dev->next);

	if ((dev->csid != PISO_P32C32)&&(dev->csid != PISO_P32C32_T3A)&&(dev->csid != PISO_P32C32_T3B))
	{

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
