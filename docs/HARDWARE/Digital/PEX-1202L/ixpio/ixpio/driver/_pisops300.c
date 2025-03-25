/* PISO-PS300 Service Module.

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
#include "_pisops300.h"
#include "_flags_inline.h"

ixpio_devinfo_t *dev;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
int ixpio_minor[256] = {0};
#endif

#ifdef MODULE_LICENSE
MODULE_AUTHOR("Reed Lai <reed@icpdas.com>");
MODULE_DESCRIPTION("ICPDAS PIO-series driver, PISO-PS300 service module");
MODULE_LICENSE(ICPDAS_LICENSE);
#endif							/* MODULE_LICENSE */

/* symbols exportation */
// EXPORT_NO_SYMBOLS; /* do not export symbols */
/*
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
EXPORT_SYMBOL_GPL(ixpisops300_ioctl);
EXPORT_SYMBOL_GPL(ixpisops300_release);
EXPORT_SYMBOL_GPL(ixpisops300_open);
#else
EXPORT_SYMBOL_NOVERS(ixpisops300_ioctl);
EXPORT_SYMBOL_NOVERS(ixpisops300_release);
EXPORT_SYMBOL_NOVERS(ixpisops300_open);
#endif
*/

int write_reg(ixpio_reg_t * reg, unsigned long int base);
int read_reg(ixpio_reg_t * reg, unsigned long int base);
ixpio_devinfo_t *align_to_minor(int minor);
void _close(ixpio_devinfo_t * dp);
int _open(ixpio_devinfo_t * dp);

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
	case IXPIO_FIFO1:
#if FIFO1_MASK == 0xff				/* FIFO1 Register */
		outb(((unsigned char) reg->value), base +FIFO1);
#else
		outb(((unsigned char) reg->value) & FIFO1_MASK, base + FIFO1);
#endif
		break;
	
	case IXPIO_RR:
#if RR_MASK == 0xff				/* RSTFIFO1 Register */
		outb(((unsigned char) reg->value), base + RR);
#else
		outb(((unsigned char) reg->value) & RR_MASK, base + RR);
#endif
		break;

	case IXPIO_DO:				/* DO */
#if DO_MASK == 0xff
		outb(((unsigned char) reg->value), base + DO);
#else
		outb(((unsigned char) reg->value) & DO_MASK, base + DO);
#endif
		break;

	case IXPIO_TJ_CNTL:			/* set reset and others */
		outb(((unsigned char) reg->value), base + TJ_CNTL);
		break;

	case IXPIO_TJ_AUXC:			/* AUX pin control register 1:output */
		outb(((unsigned char) reg->value), base + TJ_AUXC);
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

	switch (reg->id) {
	case IXPIO_FIFO2:
#if FIFO2_MASK == 0xff			/* FIFO2 Register */
		reg->value = inb(base + FIFO2);
#else
		reg->value = inb(base +FIFO2) & FIFO2_MASK;
#endif
		break;

	case IXPIO_MR:			/* MSC Register */
#if MR_MASK == 0xff
		reg->value = inb(base + MR);
#else
		reg->value = inb(base + MR) & MR_MASK;
#endif
		break;
	case IXPIO_DI:			/* DI */
#if DI_MASK == 0xff
		reg->value = inb(base + DI);
#else
		reg->value = inb(base + DI) & DI_MASK;
#endif
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
long ixpisops300_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param)
#else
int ixpisops300_ioctl(struct inode *inode, struct file *file, unsigned int ioctl_num, unsigned long ioctl_param)
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

	/* disable this board's digital IO */
	outb(BOARD_DIO_DISABLE, dp->base);

	/* release IO region */
	release_region(dp->base, IO_RANGE);

	/* uninstall ISR */
	free_irq(dp->irq, dp);

}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
int ixpisops300_release(struct inode *inode, struct file *file)
#else
void ixpisops300_release(struct inode *inode, struct file *file)
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
	open:ixpisops300_open,
	release:ixpisops300_release,

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
        unlocked_ioctl:ixpisops300_ioctl
#else
	ioctl:ixpisops300_ioctl
#endif
};

int _open(ixpio_devinfo_t * dp)
{
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
	outb(BOARD_DIO_ENABLE, dp->base);

	return SUCCESS;
}

int ixpisops300_open(struct inode *inode, struct file *file)
{
	/* (export)
	 *
	 * This function is called by ixpio.o whenever a process attempts to open
	 * the device file of PISO-PS300
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

	/* request io region */
# if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	if (check_region(dp->base, IO_RANGE)) {
		--(dp->open);
		return -EBUSY;
	}
#endif
	request_region(dp->base, IO_RANGE, MODULE_NAME);

	/* enable this board's DIO */
	outb(BOARD_DIO_ENABLE, dp->base);

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
	 *   integer 0 means ok, otherwise failed (module can't be load) */

	ixpio_devinfo_t *dp;

	printk(KERN_INFO FAMILY ": %s ", MODULE_NAME);

	/* align to first PISO-PS300 in ixpio list */
/*	for (dev = ixpio_dev; dev && dev->csid != PISO_PS300; dev = dev->next);

	if (dev->csid != PISO_PS300) {*/
	for (dev = ixpio_dev; dev && (dev->csid != PISO_PS300)&&(dev->csid != PISO_PS300_T3A)&&(dev->csid != PISO_PS300_T3B) ; dev = dev->next);

	if ((dev->csid != PISO_PS300)&&(dev->csid != PISO_PS300_T3A)&&(dev->csid != PISO_PS300_T3B)) {		
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
		dp->hid = (~(inb(dp->base + 0x07))) & 0x0f;
		printk(".");
	}

	printk(" is ready.\n");

	return SUCCESS;
}
