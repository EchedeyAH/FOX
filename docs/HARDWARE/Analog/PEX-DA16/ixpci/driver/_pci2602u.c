/* PCI-2602U

   Author: Winson

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

   v 0.0.1 31 Aug 2023 by Winson Chen
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

/* irq service */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,11,0)
#include <linux/sched/signal.h>
#else
#include <linux/sched.h>
#endif

#include <linux/interrupt.h>
#include <asm/irq.h>

/* use I/O ports */
#include <asm/io.h>
#include <linux/ioport.h>

/* Local matter */
#include "_pci2602u.h"

ixpci_devinfo_t *dev;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
int ixpci_minor[256] = {0};
#endif

#ifdef MODULE_LICENSE
MODULE_AUTHOR("Winson Chen <service@icpdas.com>");
MODULE_DESCRIPTION("ICPDAS PCI-series driver, PCI-D96SU PCI-D128SU service module");
MODULE_LICENSE(ICPDAS_LICENSE);
#endif

// EXPORT_NO_SYMBOLS; /* do not export symbols */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
EXPORT_SYMBOL_GPL(ixpci2602_ioctl);
EXPORT_SYMBOL_GPL(ixpci2602_release);
EXPORT_SYMBOL_GPL(ixpci2602_open);
#else
EXPORT_SYMBOL_NOVERS(ixpci2602_ioctl);
EXPORT_SYMBOL_NOVERS(ixpci2602_release);
EXPORT_SYMBOL_NOVERS(ixpci2602_open);
#endif

/*	winson
void _disable_irq(ixpci_devinfo_t * dp)
{
	unsigned int value;
	
	value = ioread32(dp->ioaddr[0] + _PPM_CS) & _PPM_CS_MASK;
	iowrite32(value & 0xff, dp->ioaddr[0] + _PPM_CS);	//disable all interrupt
}

void _enable_irq(ixpci_devinfo_t * dp)
{
	unsigned int value;

	//Clear 0~3 DI Compare trigger
	value = ioread32(dp->ioaddr[0] + _PPM_CS) & _PPM_CS_MASK;
	iowrite32(value & 0xFFFFF0FF, dp->ioaddr[0] + _PPM_CS);
	value = ioread32(dp->ioaddr[0] + _PPM_CS) & _PPM_CS_MASK;
	iowrite32(value | 0x00000F00, dp->ioaddr[0] + _PPM_CS);

	//Clear PortA Change Status interrupt
	iowrite32(0x0, dp->ioaddr[0] + _PA_CCS); //off	
	//Clear PortB Change Status interrupt
	iowrite32(0x0, dp->ioaddr[0] + _PB_CCS); //off	

	value = ioread32(dp->ioaddr[0] + _PPM_CS) & _PPM_CS_MASK;
	iowrite32((value&0x3F)|((value&(0xF|(0xF<<4)))<<16), dp->ioaddr[0] + _PPM_CS);
}
*/

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,31)
struct task_struct *find_task_by_pid(pid_t nr, struct pid_namespace *ns)
{
        return pid_task(find_pid_ns(nr, ns), PIDTYPE_PID);
}
#endif

/* comment 
#ifdef BOTTOM_HELF
void irq_handler_bh(void *data)
{
      // * bottom helf of irq handler
         *
         * Arguments:
         *   data     data pointer as <linux/tqueue.h> defined
         *
      // * Returned:  none *

        ixpci_devinfo_t *dp;

        dp = (ixpci_devinfo_t *) data;
}
#endif

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
    //   * Arguments:  as <linux/sched.h> defined
         *
    //   * Returned:  none *

        ixpci_devinfo_t *dp;
	unsigned int flag, lIntEnVal, lActVal;//, value;
	int bIndex;

#ifdef BOTTOM_HELF
        static struct tq_struct task = { NULL, 0, irq_handler_bh, NULL };

        task.data = dev_id;
#endif                                                  // BOTTOM_HELF
        dp = (ixpci_devinfo_t *) dev_id;

	//Read Register 30H
	//flag = ioread32(dp->ioaddr[0] + _PPM_CS) & _PPM_CS_MASK;	//winson
	//Get interrupt port status
	lIntEnVal = (flag>>16)&0x3F;
	//Get all Active status (Include Comare trigger/Change Status.)
	lActVal = (flag>>8)&0x3F;

	if(!(lActVal&lIntEnVal))
	{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
        	return FAILURE;
#else
	        return;
#endif
	}

	// Clear all interrupt 
	//Check each port Compare trigger interrupt
	for (bIndex=0; bIndex<4; bIndex++)
	{
		//Clear portA/B/C/D Compare trigger Source
		if(((lActVal&0xF)>>bIndex)&0x1)
		{
			//Clear
			//value = ioread32(dp->ioaddr[0] + _PA_CVS + 0x4 * bIndex);	//winson
			//iowrite32(value, dp->ioaddr[0] + _PA_CVS + 0x4 * bIndex);	//winson
		}
	}

	//Clear PortA Change Status interrupt
	if((lActVal>>4)&0x1)
		iowrite32(0x0, dp->ioaddr[0] + _PA_CCS); //off

	//Clear PortB Change Status interrupt
	if((lActVal>>5)&0x1)
		iowrite32(0x0, dp->ioaddr[0] + _PB_CCS); //off

        // User's signal condition 
        if (dp->sig.sid)
                send_sig(dp->sig.sid, dp->sig.task, 1);

#ifdef BOTTOM_HELF
        // Scheduale bottom half to run 
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,2,0)
        queue_task(&task, &tq_immediate);
#else
        queue_task_irq(&task, &tq_immediate);
#endif
        mark_bh(IMMEDIATE_BH);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
        return IRQ_HANDLED;
#endif
}
*/

ixpci_devinfo_t *_align_minor(int minor)
{
	/* align to device by minor number */

	ixpci_devinfo_t *dp;
	for (dp = dev; dp && dp->no != minor; dp = dp->next_f);
	return dp;
}

int _write_reg(ixpci_reg_t * reg, void __iomem *ioaddr[])
{
        /* Write to register
         *
         * Arguments:
         *   reg:       pointer to a structure ixpci_reg for register
         *   ioaddr:	mmio address for current device
         *
         * Returned:    SUCCESS or FAILED */

        switch(reg->id)
	{

	case IXPCI_AI_RM:	//0x200 bit0 Reset AI Mode. Write a 1 and Write a 0 to resets AI Mode.
		iowrite32((reg->value) & _AI_RM_MASK, ioaddr[3] + _AI_RM);
		break;
	case IXPCI_AI_CF:	//0x200 bit1 Clear AI FIFO. Write a 1 and Write a 0 to clear AI FIFO.
		iowrite32((reg->value) & _AI_CF_MASK, ioaddr[3] + _AI_CF);
		break;
	case IXPCI_AI_RFOS:	//0x200 bit2 Reset AI FIFO Overflow Status. Write a 1 and Write a 0 to resets AI FIFO overflow status.
		iowrite32((reg->value) & _AI_RFOS_MASK, ioaddr[3] + _AI_RFOS);
		break;
	case IXPCI_CI:		//0x200 bit3 PCI Clear Interrupt. Write a 1 and Write a 0 to clear PCI interrupts.
		iowrite32((reg->value) & _CI_MASK, ioaddr[3] + _CI);
		break;
	case IXPCI_DI_RF:	//0x200 bit6 DI(PC) FIFO Reset. Write a 1 and Write a 0 to resets digital input FIFO.
		iowrite32((reg->value) & _DI_RF_MASK, ioaddr[3] + _DI_RF);
		break;
	case IXPCI_DO_RF:	//0x200 bit7 DO(PA) FIFO Reset. Write a 1 and Write a 0 to resets digital output FIFO. 
		iowrite32((reg->value) & _DO_RF_MASK, ioaddr[3] + _DO_RF);
		break;
	case IXPCI_AO0_RF:	//0x200 bit8 AO0 FIFO Reset. Write a 1 and Write a 0 to resets analog output channel 0 FIFO.
		iowrite32((reg->value) & _AO0_RF_MASK, ioaddr[3] + _AO0_RF);
		break;
	case IXPCI_DI_RFOS:	//0x200 bit9 in dos qc
		iowrite32((reg->value) & _DI_RFOS_MASK, ioaddr[3] + _DI_RFOS);
		break;
	case IXPCI_DIO_PABCD_C:	//0x210 bit0~bit3 PA/PB/PC/PD DIO Configure. Write a 1 indicates a DO mode. Write a 0 indicates a DI mode.
		iowrite32((reg->value) & _DIO_PABCD_C_MASK, ioaddr[3] + _DIO_PABCD_C);
		break;
	case IXPCI_DIO_PABCD:  //0x214 bit0~bit31 PA/PB/PC/PD DIO port write
		iowrite32((reg->value) & _DIO_PABCD_MASK, ioaddr[3] + _DIO_PABCD);
                break;
	case IXPCI_DIOPA:	//0x214 bit0~bit7
		iowrite32((reg->value) & _DIOPA_MASK, ioaddr[3] + _DIOPA);
		break;
	case IXPCI_DIOPB:	//0x214 bit8~bit15
		//reg->value = reg->value << 8;	//winson
		iowrite32((reg->value << 8) & _DIOPB_MASK, ioaddr[3] + _DIOPB);
		break;
	case IXPCI_DIOPC:	//0x214 bit16~bit23
		//reg->value = reg->value << 16;	//winson
		iowrite32((reg->value << 16) & _DIOPC_MASK, ioaddr[3] + _DIOPC);
		break;
	case IXPCI_DIOPD:	//0x214 bit24~bit31
		//reg->value = reg->value << 24;	//winson
		iowrite32((reg->value << 24) & _DIOPD_MASK, ioaddr[3] + _DIOPD);
		break;
	case IXPCI_AI_STC:  //0x294
		printk("Enter 0x294\n");	//winson
		iowrite32((reg->value) & _AI_STC_MASK, ioaddr[3] + _AI_STC);
		break;
	case IXPCI_AI_SA:  //0x298
		printk("Enter 0x298\n");	//winson
		iowrite32((reg->value) & _AI_SA_MASK, ioaddr[3] + _AI_SA);
		break;
	case IXPCI_AO_CFG:  //0x2b0
		iowrite32((reg->value) & _AO_CFG_MASK, ioaddr[3] + _AO_CFG);
		break;
	case IXPCI_AO0:  //0x2b4
		iowrite32((reg->value) & _AO0_MASK, ioaddr[3] + _AO0);
		break;
	case IXPCI_AO1:  //0x2b8
		iowrite32((reg->value) & _AO1_MASK, ioaddr[3] + _AO1);
		break;
	case IXPCI_AI_CC:  //0x2ec
		printk("Enter 0x2ec\n");	//winson
		iowrite32((reg->value) & _AI_CC_MASK, ioaddr[3] + _AI_CC);
		break;
	case IXPCI_SAI_C:  //0x2f0
		printk("Enter 0x2f0\n");	//winson
		iowrite32((reg->value) & _SAI_C_MASK, ioaddr[3] + _SAI_C);
		break;
        default:
                return FAILURE;
        }
        return SUCCESS;
}

int _read_reg(ixpci_reg_t * reg, void __iomem * ioaddr[])
{

        /* Read from register
         *
         * Arguments:
         *   reg	pointer to structure ixpci_reg fot register
         *   ioaddr	mmio address for current device
         *
         *   Returned:  SUCCESS or FAILURE */

        switch(reg->id){
	case IXPCI_GCID:  //0x200 bit0~3
                reg->value = ioread32(ioaddr[3] + _GCID) & _GCID_MASK;
                break;
	case IXPCI_DIO_PABCD_C:  //0x210 bit0~bit3 PA/PB/PC/PD DIO Configure. Write a 1 indicates a DO mode. Write a 0 indicates a DI mode.
                reg->value = ioread32(ioaddr[3] + _DIO_PABCD_C) & _DIO_PABCD_C_MASK;
                break;
	case IXPCI_DIO_PABCD:  //0x214 bit0~bit31 PA/PB/PC/PD DIO port read
                reg->value = ioread32(ioaddr[3] + _DIO_PABCD) & _DIO_PABCD_MASK;
                break;
	case IXPCI_DIOPA:  //0x214 bit0~bit7
                reg->value = ioread32(ioaddr[3] + _DIOPA) & _DIOPA_MASK;
                break;
	case IXPCI_DIOPB:  //0x214 bit8~bit15
                reg->value = (ioread32(ioaddr[3] + _DIOPB) & _DIOPB_MASK) >> 8;
                break;
	case IXPCI_DIOPC:  //0x214 bit16~bit23
                reg->value = (ioread32(ioaddr[3] + _DIOPC) & _DIOPC_MASK) >> 16;
                break;
	case IXPCI_DIOPD:  //0x214 bit24~bit31
                reg->value = (ioread32(ioaddr[3] + _DIOPD) & _DIOPD_MASK) >> 24;
                break;
	case IXPCI_DI_FS:  //0x21c bit16~21
                reg->value = ioread32(ioaddr[3] + _DI_FS) & _DI_FS_MASK;
                break;
	case IXPCI_AI_STS:  //0x294
		printk("(READ)Enter 0x294\n");	//winson
                reg->value = ioread32(ioaddr[3] + _AI_STS) & _AI_STS_MASK;
                break;
	case IXPCI_AO_CFG:  //0x2b0
                reg->value = ioread32(ioaddr[3] + _AO_CFG) & _AO_CFG_MASK;
                break;
	case IXPCI_AI_CC:  //0x2ec
                reg->value = ioread32(ioaddr[3] + _AI_CC) & _AI_CC_MASK;
                break;
	case IXPCI_SAI_C:  //0x2f0
                reg->value = ioread32(ioaddr[3] + _SAI_C) & _SAI_C_MASK;
                break;
        default:
                return FAILURE;
        }
        return SUCCESS;
}
/*
int set_signal(ixpci_signal_t * sig, ixpci_devinfo_t * dp)
{

//	 * set user signal for interrupt
         *
         * Arguments:
         *   sig      user signal
         *   di       current device
         *
//         * Returned:  SUCCESS or FAILURE *

         if (sig->sid) {
                // has signal id, set 
                if (sig->pid) {
                        // user gives his process id 
                        dp->sig.sid = sig->sid;
                        dp->sig.pid = sig->pid;
                        // user gives his process id 
                        #if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,31)
                        dp->sig.task = find_task_by_pid(sig->pid,&init_pid_ns);

                        #elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
                        dp->sig.task = find_task_by_pid_type_ns(PIDTYPE_PID,sig->pid,&init_pid_ns);
                        #else
                        dp->sig.task = find_task_by_pid(sig->pid);
                        #endif

                } else if (sig->task) {
                        // user gives his task structure 
                        dp->sig.sid = sig->sid;
                        dp->sig.task = sig->task;
                } else {
                        // wrong! 
                        return FAILURE;
                }
        } else {
                // no signal id, clear 
                dp->sig.sid = 0;
                dp->sig.pid = 0;
                dp->sig.task = 0;
                dp->sig.is = 0;
                dp->sig.edge = 0;
                dp->sig.bedge = 0;
        }
        return SUCCESS;
}
*/

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
long ixpci2602_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param)
#else
int ixpci2602_ioctl(struct inode *inode, struct file *file, unsigned int ioctl_num, unsigned long ioctl_param)
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
	ixpci_reg_t reg;
	//ixpci_signal_t signal;
	unsigned int card_id;

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
        case IXPCI_READ_REG:
		if (copy_from_user(&reg, argp, sizeof(ixpci_reg_t)))
                {
                        printk("XXX %s ... IXPCI_READ_REG ... copy_from_user fail\n", __FUNCTION__);
                        return -EFAULT;
                }

                if (_read_reg(&reg, dp->ioaddr))
                        return FAILURE;

                if (copy_to_user(argp, &reg, sizeof(ixpci_reg_t)))
                {
                        printk("XXX %s ... IXPCI_READ_REG ... copy_to_user fail\n", __FUNCTION__);
                        return -EFAULT;
                }
                break;
        case IXPCI_WRITE_REG:
		if (copy_from_user(&reg, argp, sizeof(ixpci_reg_t)))
                {
                        printk("XXX %s ... IXPCI_WRITE_REG ... copy_from_user fail\n", __FUNCTION__);
                        return -EFAULT;
                }

                if (_write_reg(&reg, dp->ioaddr))
			return FAILURE;
                break;
	case IXPCI_GCID:        //Get card id
		if(copy_from_user(&card_id, argp, sizeof(unsigned int)))
                {
                        printk("XXX %s ... IXPCI_GCID ... copy_from_user fail\n",__FUNCTION__);
                        return -EFAULT;
                }
		card_id = ioread32(dp->ioaddr[3] + _GCID) & _GCID_MASK;
                if(copy_to_user(argp, &card_id, sizeof(unsigned int)))
                {
                        printk("XXX %s ... IXPCI_GCID ... copy_to_user fail\n",__FUNCTION__);
                        return -EFAULT;
                }
                break;
/*
	case IXPCI_SET_SIG:
#if 1
                if(copy_from_user(&signal, argp, sizeof(ixpci_signal_t)))
                {
                        printk("XXX %s ... IXPCI_SET_SIG ... copy_from_user fail\n",__FUNCTION__);
                        return -EFAULT;
                }

                if (set_signal(&signal, dp))
#else
		if (set_signal((ixpci_signal_t *) ioctl_param, dp))
#endif
			return FAILURE;
		break;
*/
/*
	case IXPCI_IRQ_ENABLE:
		_enable_irq(dp);
		break;
	case IXPCI_IRQ_DISABLE:
		_disable_irq(dp);
		break;
*/
	default:
		return -EINVAL;
	}
	return SUCCESS;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
int ixpci2602_release(struct inode *inode, struct file *file)
#else
void ixpci2602_release(struct inode *inode, struct file *file)
#endif
{
	/* (export)
	 *
	 * This function is called by ixpci.o whenever a process attempts to
	 * closes the device file. It doesn't have a return value in kernel
	 * version 2.0.x because it can't fail (you must always be able to
	 * close a device).  In version 2.2.x it is allowed to fail.
	 *
	  Arguments: read <linux/fs.h> for (*release) of struct file_operations
	 *
	 * Returned:  none */

	int minor, i;
	ixpci_devinfo_t *dp;
	//ixpci_signal_t sig;

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
/*		winson
		_disable_irq(dp);
		sig.sid = 0;
		set_signal(&sig, dp);
		free_irq(dp->irq, dp);
*/
		for (i = 0; i < PBAN; i++)
		{
			if(i == 0 || i == 3)	//This card just request bar0/3
			{
				iounmap(dp->ioaddr[i]);
				if(dp->ioaddr[i])
					release_mem_region(dp->base[i], dp->range[i]);
			}

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
	open:ixpci2602_open,
	release:ixpci2602_release,

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
	unlocked_ioctl:ixpci2602_ioctl,
#else
	ioctl:ixpci2602_ioctl,
#endif
};

int ixpci2602_open(struct inode *inode, struct file *file)
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
	//ixpci_signal_t sig;
	

	minor = MINOR(inode->i_rdev);
	dp = _align_minor(minor);

	if (!dp) return -EINVAL;

	++(dp->open);
	if (dp->open > 1) {
		--(dp->open);
		return -EBUSY;
		/* if still opened by someone, get out */
	}

	/* clear signal setting */
//        sig.sid = 0;	winson
//        set_signal(&sig, dp);	winson

	/* request MMIO region */
	for (i = 0; i < PBAN; i++)
	{
# if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
		if (!dp->base[i] || !dp->range[i] || check_region(dp->base[i], dp->range[i]))
			continue;
#endif
		if(i == 0 || i == 3)	//bar0/3 use MMIO, bar1/2 use IO port, this card just use bar0/3, so escape to request bar1/2
		{
			request_mem_region(dp->base[i], dp->range[i], MODULE_NAME);
			dp->ioaddr[i] = ioremap(dp->base[i], dp->range[i]);
		}
	}

	/* disable board interrupt */
//	_disable_irq(dp);	winson

        /* install ISR */
/*	winson
        if (request_irq(dp->irq, irq_handler, SA_SHIRQ, dp->name, dp)) {
                --(dp->open);
                return -EBUSY;
        }
*/

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

	/* align to first PCI-D96SU or PCI-D128SU in ixpci list */
	for (dev = ixpci_dev; dev && (dev->id != PCI_2602U); dev = dev->next);

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
