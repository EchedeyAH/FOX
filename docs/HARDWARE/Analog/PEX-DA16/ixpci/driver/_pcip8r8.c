/* PCI-P8R8

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

   v 0.2.1 11 Nov 2014 by Winson Chen
     Give support to linux kernel 3.16.0.
     Remove EXPORT_SYMBOL_GPL().

   v 0.2.0 1 Apr 2011 by Golden Wang
     Give support to linux kernel 2.6.37.

   v 0.1.1 16 May 2007 by Golden Wang
     Include "linux/cdev.h" after including "ixpci.h".

   v 0.1.0  8 Feb 2007 by Golden Wang
     Give support to linux kernel 2.6.x.

   v 0.0.2  8 Jul 2003 by Reed Lai
     Fixed a bug about _align_minor().

   v 0.0.1 16 Jan 2003 by Reed Lai
     Fixed the request io region bug that hanged the system.

   v 0.0.0  9 Jan 2003 by Reed Lai
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
#include "_pcip8r8.h"

ixpci_devinfo_t *dev;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
int ixpci_minor[256] = {0};
#endif

#ifdef MODULE_LICENSE
MODULE_AUTHOR("Reed Lai <reed@icpdas.com>");
MODULE_DESCRIPTION("ICPDAS PCI-series driver, PCI-P8R8, PEX-P8POR8 service module");
MODULE_LICENSE(ICPDAS_LICENSE);
#endif

// EXPORT_NO_SYMBOLS; /* do not export symbols */
/*
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
EXPORT_SYMBOL_GPL(ixpcip8r8_ioctl);
EXPORT_SYMBOL_GPL(ixpcip8r8_release);
EXPORT_SYMBOL_GPL(ixpcip8r8_open);
#else
EXPORT_SYMBOL_NOVERS(ixpcip8r8_ioctl);
EXPORT_SYMBOL_NOVERS(ixpcip8r8_release);
EXPORT_SYMBOL_NOVERS(ixpcip8r8_open);
#endif
*/

ixpci_devinfo_t *_align_minor(int minor)
{
	/* align to device by minor number */

	ixpci_devinfo_t *dp;
	for (dp = dev; dp && dp->no != minor; dp = dp->next_f);
	return dp;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
long ixpcip8r8_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param)
#else
int ixpcip8r8_ioctl(struct inode *inode, struct file *file, unsigned int ioctl_num, unsigned long ioctl_param)
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
	void __user *argp = (void __user *)ioctl_param;
	unsigned int reg;

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
		break;
	case IXPCI_IOCTL_DI:	//Digital input
		if(copy_from_user(&reg, argp, sizeof(unsigned int)))
		{
			printk("XXX %s ... IXPCI_IOCTL_DI ... copy_from_user fail\n",__FUNCTION__);
			return -EFAULT;
		}

		reg = inb(dp->base[2]) & _DI_MASK;

		if(copy_to_user(argp, &reg, sizeof(unsigned int)))
		{
			printk("XXX %s ... IXPCI_IOCTL_DI ... copy_to_user fail\n",__FUNCTION__);
			return -EFAULT;
		}
		break;
	case IXPCI_IOCTL_DO:	//Digital output
		if(copy_from_user(&reg, argp, sizeof(unsigned int)))
                {
                        printk("XXX %s ... IXPCI_IOCTL_DO ... copy_from_user fail\n",__FUNCTION__);
                        return -EFAULT;
                }
                outb(reg & _DO_MASK, dp->base[2]);

		break;
	case IXPCI_DORB:        //DO readback
                if(copy_from_user(&reg, argp, sizeof(unsigned int)))
                {
                        printk("XXX %s ... IXPCI_DORB ... copy_from_user fail\n",__FUNCTION__);
                        return -EFAULT;
                }

                reg = inb(dp->base[2] + _DORB) & _DORB_MASK;

                if(copy_to_user(argp, &reg, sizeof(unsigned int)))
                {
                        printk("XXX %s ... IXPCI_DORB ... copy_to_user fail\n",__FUNCTION__);
                        return -EFAULT;
                }
                break;
	 case IXPCI_GCID:        //Get card id
                if(copy_from_user(&reg, argp, sizeof(unsigned int)))
                {
                        printk("XXX %s ... IXPCI_IXPCI_GCID ... copy_from_user fail\n",__FUNCTION__);
                        return -EFAULT;
                }

                reg = inb(dp->base[2] + _GCID) & _GCID_MASK;

                if(copy_to_user(argp, &reg, sizeof(unsigned int)))
                {
                        printk("XXX %s ... IXPCI_GCID ... copy_to_user fail\n",__FUNCTION__);
                        return -EFAULT;
                }
                break;
	default:
		return -EINVAL;
	}
	return SUCCESS;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
int ixpcip8r8_release(struct inode *inode, struct file *file)
#else
void ixpcip8r8_release(struct inode *inode, struct file *file)
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
		for (i = 1; i < PBAN; i++)
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
	open:ixpcip8r8_open,
	release:ixpcip8r8_release,

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
	unlocked_ioctl:ixpcip8r8_ioctl,
#else
	ioctl:ixpcip8r8_ioctl,
#endif
};

int ixpcip8r8_open(struct inode *inode, struct file *file)
{
	/* (export)
	 *
	 * This function is called by ixpci.o whenever a process attempts to
	 * open the device file of PCI-P8R8, PEX-P8POR8
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
	for (i = 1; i < PBAN; i++)
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
	 * integer 0 for ok, otherwise failed (module can't be load) */

	ixpci_devinfo_t *dp;

	KMSG("%s ", MODULE_NAME);

	/* align to first PCI-P8R8 or PEX-P8POR8 in ixpci list */
	for (dev = ixpci_dev; dev && dev->id != PCI_P8R8; dev = dev->next);

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
		printk(".");
	}

	printk(" ready.\n");
	return SUCCESS;
}
