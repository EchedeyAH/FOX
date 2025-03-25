/* PCI-822 Service Module 
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

/* File level history (record changes for this file here.)
   v 0.0.1 11 Nov 2014 by Winson Chen
     Give support to linux kernel 3.16.0.
     Remove EXPORT_SYMBOL_GPL().

   v 0.0.0 31 Oct 2013 by Winson Chen
     create */

/* Mandatory */
#include <linux/kernel.h>	/* ta, kernel work */
#include <linux/module.h>	/* is a module */
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

/* irq service */
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <asm/irq.h>

/* use I/O ports */
#include <asm/io.h>
#include <linux/ioport.h>

/* Local matter */
#include "_pci822.h"

ixpci_devinfo_t *dev;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
int ixpci_minor[256] = {0};
#endif

#ifdef MODULE_LICENSE
MODULE_AUTHOR("Winson Chen <service@icpdas.com>");
MODULE_DESCRIPTION("ICPDAS PCI-series deiver, PCI-822 service module");
MODULE_LICENSE(ICPDAS_LICENSE);
#endif

// EXPORT_NO_SYMBOLS; /* do not export symbols */
/*
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
EXPORT_SYMBOL_GPL(ixpci822_ioctl);
EXPORT_SYMBOL_GPL(ixpci822_release);
EXPORT_SYMBOL_GPL(ixpci822_open);
#else
EXPORT_SYMBOL_NOVERS(ixpci822_ioctl);
EXPORT_SYMBOL_NOVERS(ixpci822_release);
EXPORT_SYMBOL_NOVERS(ixpci822_open);
#endif
*/

static struct file_operations fops = {
        /* kernel 2.6 prevent the module from unloading while there is a open file(kernel 2.4 use the funciton MOD_INC_USE_COUNT and MOD_DEC_USE_COUNT to protect module from unloading when someone is opening file), so driver writer must set the file_operation's field owner a value 'THIS_MODULE' */
# if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
        owner:THIS_MODULE,
#endif
        open:ixpci822_open,
        release:ixpci822_release,

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
        unlocked_ioctl:ixpci822_ioctl,
#else
        ioctl:ixpci822_ioctl,
#endif
};

ixpci_devinfo_t *_align_minor(int minor)
{
	/* align to device by minor number */

	ixpci_devinfo_t *dp;
	for(dp = dev; dp && dp->no != minor; dp = dp ->next_f);
	return dp;
}

int _write_reg(ixpci_reg_t * reg, unsigned long int base[])
{
	/* Write to register
 	 *
 	 * Arguments:
 	 *   reg:	pointer to a structure ixpci_reg for register
 	 *   base:	base address for current device
 	 *
 	 * Returned:	SUCCESS or FAILED */ 

	switch(reg->id){
	case IXPCI_DIOPA:
#if _DIO_MASK == 0xffff
                outw((reg->value), base[1] + _DIO_PA);
#else
                outw((reg->value) & _DIO_MASK, base[1] + _DIO_PA);
#endif
                break;
	case IXPCI_DIOPB:
#if _DIO_MASK == 0xffff
		outw((reg->value), base[1] + _DIO_PB);
#else
		outw((reg->value) & _DIO_MASK, base[1] + _DIO_PB);
#endif
		break;
	case IXPCI_EEP:
#if _EEP_MASK == 0xffff
		outw((reg->value), base[1] + _EEP);
#else
		outw((reg->value) & _EEP_MASK, base[1] + _EEP);
#endif
		break;
	case IXPCI_PAB_CONFIG:
#if _PAB_CONFIG_MASK == 0xffff
		outw((reg->value), base[1] + _PAB_CONFIG);
#else
		outw((reg->value) & _PAB_CONFIG_MASK, base[1] + _PAB_CONFIG);
#endif
		break;
	case IXPCI_RW_DA:
#if _RW_DA_MASK == 0xffff
		outw((reg->value), base[2] + _RW_DA);
#else
		outw((reg->value) & _RW_DA_MASK, base[2] + _RW_DA);
#endif
		break;
	case IXPCI_RS_DA_CS:
#if _RS_DA_CS_MASK == 0xff
		outw((reg->value), base[2] + _RS_DA_CS);
#else
		outw((reg->value) & _RS_DA_CS_MASK, base[2] + _RS_DA_CS);
#endif
		break;
	case IXPCI_ED_DA_CH:
#if _ED_DA_CH_MASK == 0xff
		outw((reg->value), base[2] + _ED_DA_CH);
#else
		outw((reg->value) & _ED_DA_CH_MASK, base[2] + _ED_DA_CH);
#endif
		break;
	case IXPCI_ADPR:
#if _RW_AD_PC_MASK == 0xffff
		outw((reg->value), base[3] + _RW_AD_PC);
#else		
		outw((reg->value) & _RW_AD_PC_MASK, base[3] + _RW_AD_PC);
#endif
		break;
	case IXPCI_ADST:
#if _AD_TP_MASK == 0xffff
		outw((reg->value), base[3] + _AD_TP);
#else
		outw((reg->value) & _AD_TP_MASK, base[3] + _AD_TP);
#endif
		break;
	case IXPCI_RS_SR:
#if _RS_SR_MASK == 0xffff
		outw((reg->value), base[3] + _RS_SR);
#else
		outw((reg->value) & _RS_SR_MASK, base[3] + _RS_SR);
#endif
		break;
	case IXPCI_RS_CN_MS:
#if _RS_CN_MS_MASK == 0xffff
		outw((reg->value), base[3] + _RS_CN_MS);
#else
		outw((reg->value) & _RS_CN_MS_MASK, base[3] + _RS_CN_MS);
#endif
		break;
	case IXPCI_AI_PC:
#if _AI_PC_MASK == 0xffff
		outw((reg->value), base[3] + _AI_PC);
#else
		outw((reg->value) & _AI_PC_MASK, base[3] + _AI_PC);
#endif
		break;
	case IXPCI_RW_MS_BF_CS:
#if _RW_MS_BF_CS_MASK == 0xffff
		outw((reg->value), base[3] + _RW_MS_BF_CS);
#else
		outw((reg->value) & _RW_MS_BF_CS_MASK, base[3] + _RW_MS_BF_CS);
#endif
		break;
	case IXPCI_SS_MS:
#if _SS_MS_MASK == 0xffff
		outw((reg->value), base[3] + _SS_MS);
#else
		outw((reg->value) & _SS_MS_MASK, base[3] + _SS_MS);
#endif
		break;
	case IXPCI_RS_IC:
#if _RS_IC_MASK == 0xffff
		outw((reg->value), base[3] + _RS_IC);
#else
		outw((reg->value) & _RS_IC_MASK, base[3] + _RS_IC);
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
 	 *   reg	pointer to structure ixpci_reg fot register
 	 *   base	base address for current device
 	 *
 	 *   Returned:	SUCCESS or FAILURE */

	switch(reg->id){
	case IXPCI_DIOPA:
#if _DIO_MASK == 0xffff
		reg->value = inw(base[1] + _DIO_PA);
#else
		reg->value = inw(base[1] + _DIO_PA) & _DIO_MASK;
#endif
		break;
	case IXPCI_DIOPB:
#if _DIO_MASK == 0xffff
                reg->value = inw(base[1] + _DIO_PB);
#else
                reg->value = inw(base[1] + _DIO_PB) & _DIO_MASK;
#endif
		break;
	case IXPCI_EEP:
#if _EEP_MASK == 0xffff
		reg->value = inw(base[1] + _EEP);
#else
		reg->value = inw(base[1] + _EEP) & _EEP_MASK;
#endif
		break;
	case IXPCI_GDIO_JS:
#if _GDIO_JS_MASK == 0xff
		reg->value = inw(base[1] + _GDIO_JS);
#else
		reg->value = inw(base[1] + _GDIO_JS) >> 8 & _GDIO_JS_MASK;
#endif
		break;
	case IXPCI_GCID:
#if _GCID_MASK == 0xff
		reg->value = inw(base[1] + _GCID);
#else
		reg->value = inw(base[1] + _GCID) & _GCID_MASK;
#endif
		break;
	case IXPCI_RW_DA:
#if _RW_DA_MASK == 0xffff
		reg->value = inw(base[2] + _RW_DA);
#else
		reg->value = inw(base[2] + _RW_DA) & _RW_DA_MASK;
#endif
		break;
	case IXPCI_RS_DA_CS:
#if _RS_DA_CS_MASK == 0xff
		reg->value = inw(base[2] + _RS_DA_CS);
#else
		reg->value = inw(base[2] + _RS_DA_CS) & _RS_DA_CS_MASK;
#endif
		break;
	case IXPCI_ADPR:
#if _RW_AD_PC_MASK == 0xffff
		reg->value = inw(base[3] + _RW_AD_PC);
#else
		reg->value = inw(base[3] + _RW_AD_PC) & _RW_AD_PC_MASK;
#endif
		break;
	case IXPCI_AI:
#if _AI_MASK == 0xffff
		reg->value = inw(base[3] + _AI);
#else
		reg->value = inw(base[3] + _AI) & _AI_MASK;
#endif
		break;
	case IXPCI_RS_SR:
#if _RS_SR_MASK == 0xffff
		reg->value = inw(base[3] + _RS_SR);
#else
		reg->value = inw(base[3] + _RS_SR) & _RS_SR_MASK;
#endif
		break;
	case IXPCI_RS_CN_MS:
#if _RS_CN_MS_MASK == 0xffff
                reg->value = inw(base[3] + _RS_CN_MS);
#else
                reg->value = inw(base[3] + _RS_CN_MS) & _RS_CN_MS_MASK;
#endif
                break;
	case IXPCI_RAI_PC:
#if _RAI_PC_MASK == 0xffff
		reg->value = inw(base[3] + _RAI_PC);
#else
		reg->value = inw(base[3] + _RAI_PC) & _RAI_PC_MASK;
#endif
		break;
	case IXPCI_RW_MS_BF_CS:
#if _RW_MS_BF_CS_MASK == 0xffff
                reg->value = inw(base[3] + _RW_MS_BF_CS);
#else
                reg->value = inw(base[3] + _RW_MS_BF_CS) & _RW_MS_BF_CS_MASK;
#endif
                break;
	case IXPCI_CI:
#if _CI_MASK == 0xffff
		reg->value = inw(base[3] + _CI);
#else
		reg->value = inw(base[3] + _CI) & _CI_MASK;
#endif
		break;
	case IXPCI_RS_IC:
#if _RS_IC_MASK == 0xffff
		reg->value = inw(base[3] + _RS_IC);
#else
		reg->value = inw(base[3] + _RS_IC) & _RS_IC_MASK;
#endif
		break;
	default:
		return FAILURE;
	}
	return SUCCESS;
}

int ixpci822_open(struct inode *inode, struct file *file)
{
	/* (export)
 	 *
 	 * This function is called by ixpci.o whenever a process attempts to 
 	 * open the device file of PCI-822
 	 *
 	 * Arguments: read <linux/fs.h> gor (*open) of struct file_operations
 	 *
 	 * Returned: none */

	int minor,i;
	ixpci_devinfo_t *dp;

	minor = MINOR(inode->i_rdev);
	dp = _align_minor(minor);

	if(!dp) return -EINVAL;

	++(dp->open);
	if(dp->open > 1){
		--(dp->open);
		return -EBUSY;
		/* if still opened by someone, get out */
	}

	/* request io region */
	for(i = 0; i < PBAN; i++)
	{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
		if(!dp->base[i] || !dp->range[i] || check_region(dp->base[i],dp->range[i]))
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

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
long ixpci822_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param)
#else
int ixpci822_ioctl(struct inode *inode, struct file *file, unsigned int ioctl_num, unsigned long ioctl_param)
#endif
{
	/* (export)
 	 *
 	 * This function is called by ixpci.o whenever a process tries
 	 * to do and IO control on IXPCI device file
 	 *
 	 * Argument: read <linux/fs.h> for (*ioctl) of struct file_operation
 	 *
 	 * Returned: SUCCESS or FAILED */

	ixpci_devinfo_t *dp;
	void __user *argp = (void __user*)ioctl_param;
	ixpci_reg_t ixpci_reg;

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
	
	if(!dp || !dp->open) return -EINVAL;

	switch (ioctl_num){
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
		if(_read_reg((ixpci_reg_t *) ioctl_param,dp->base))
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
		if(_write_reg((ixpci_reg_t *) ioctl_param, dp->base))
#endif
			return FAILURE;
		break;
	default:
		return -EINVAL;
	}
	return SUCCESS;
}

int ixpci822_release(struct inode *inode, struct file *file)
{
	/* (expoet)
 	 *
 	 * This function is called by ixpci.o whenever a process attempts to
 	 * closes the device file. It doesn't have a return value in kernel
 	 * version 2.0.x because it can't fail (you must always be able to
 	 * close a device). In version 2.2.x it is allowed to fail,
 	 *
 	 * Arguments: read<linux/fs.h> for (*release) of struct file_operations
 	 *
 	 * Returned: none*/

	int minor,i;
	ixpci_devinfo_t *dp;

	minor = MINOR(inode->i_rdev);
	dp = _align_minor(minor);

	if(dp)
	{
		if(file->private_data)
		{
			file->private_data = NULL;
			kfree(file->private_data);
		}
		
		dp->open = 0;

		for(i = 0; i < PBAN; i++)
		{
			if(dp->base[i])
				release_region(dp->base[i], dp->range[i]);
		}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0) && LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	MOD_DEC_USE_COUNT;
#endif
	}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
	return 0;
#endif
}

void cleanup_module()
{
	/* cleanup this module  */

	ixpci_devinfo_t *dp;

	KMSG("%s ", MODULE_NAME);
	
	for(dp = dev; dp; dp = dp->next_f){

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
		cdev_del(dp->cdev);
#else
		dp->fops = 0;
#endif
		/* remove file_operations */
		printk(".");
	}
	printk(" has been removed\n");
}

int init_module()
{
	/* initialize this module
	 * 
	 * Arguments; none
	 *
	 * Returnd;
	 * integer 0 for ok, otherwise failed (module can't be load) */

	ixpci_devinfo_t *dp;
	
	KMSG("%s ", MODULE_NAME);

	/* align to first PCI-822 in ixpci list  */
	for(dev = ixpci_dev; dev && dev->id != PCI_822; dev = dev->next);

	if(!dev){
		printk("failed\n");
		return FAILURE;
	}

	for(dp = dev; dp; dp = dp->next_f){
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
