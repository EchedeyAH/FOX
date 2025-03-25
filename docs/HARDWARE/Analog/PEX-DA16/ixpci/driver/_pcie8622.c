/* PCIe-8622 Service Module

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

   v 0.0.0 24 Sep 2015 by Winson Chen
     create, blah blah... */

/* Mandatory */
#include <linux/kernel.h>		/* ta, kernel work */
#include <linux/module.h>		/* is a module */
#include <linux/delay.h>
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
#include "_pcie8622.h"
//#include "_pcie8622_cal.h"

ixpci_devinfo_t *dev;
 
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
int ixpci_minor[256] = {0};
#endif

#ifdef MODULE_LICENSE
MODULE_AUTHOR("Winson Chen <service@icpdas.com>");
MODULE_DESCRIPTION("ICPDAS PCI-series driver, PCIe-8622 service module");
MODULE_LICENSE(ICPDAS_LICENSE);
#endif

// EXPORT_NO_SYMBOLS; /* do not export symbols */
/*
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
EXPORT_SYMBOL_GPL(ixpcie8622_ioctl);
EXPORT_SYMBOL_GPL(ixpcie8622_release);
EXPORT_SYMBOL_GPL(ixpcie8622_open);
#else
EXPORT_SYMBOL_NOVERS(ixpcie8622_ioctl);
EXPORT_SYMBOL_NOVERS(ixpcie8622_release);
EXPORT_SYMBOL_NOVERS(ixpcie8622_open);
#endif
*/

struct task_struct *find_task_by_pid(pid_t nr, struct pid_namespace *ns);
int _write_reg(ixpci_reg_t *reg, void __iomem *ioaddr[]);
int _read_reg(ixpci_reg_t *reg, void __iomem *ioaddr[]);
int _read_eep(ixpci_eep_t *, ixpci_devinfo_t *);

void _disable_irq(ixpci_devinfo_t * dp)
{
	iowrite32(0x0 & RW_ICS_MASK, dp->ioaddr[0] + RW_ICS);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,31)
struct task_struct *find_task_by_pid(pid_t nr, struct pid_namespace *ns)
{
        return pid_task(find_pid_ns(nr, ns), PIDTYPE_PID);
}
#endif

#ifdef BOTTOM_HELF
void irq_handler_bh(void *data)
{
        /* bottom helf of irq handler
         *
         * Arguments:
         *   data     data pointer as <linux/tqueue.h> defined
         *
         * Returned:  none */

        ixpci_devinfo_t *dp;

        dp = (ixpci_devinfo_t *) data;

        /* reserved */
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
	/* Arguments:  as <linux/sched.h> defined
	 * 
	 * Returned:  none */

        ixpci_devinfo_t *dp;

#ifdef BOTTOM_HELF
        static struct tq_struct task = { NULL, 0, irq_handler_bh, NULL };

        task.data = dev_id;
#endif                                                  /* BOTTOM_HELF */

        dp = (ixpci_devinfo_t *) dev_id;
	
        if(!(ioread32(dp->ioaddr[0] + RW_ICS) & 0x2 ))
	{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
	return IRQ_HANDLED;
	//return FAILURE;
#else
        return;
#endif
	}

        /* User's signal condition */
        if (dp->sig.sid)
		send_sig(dp->sig.sid, dp->sig.task, 1);

	printk("Send signal\n");

	/* Clear all interrupt */
        iowrite32(0x0 & CI_MASK, dp->ioaddr[0] + CI);
	
	/* Disable interrupt */
	_disable_irq(dp);

#ifdef BOTTOM_HELF
        /* Scheduale bottom half to run */
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,2,0)
        queue_task(&task, &tq_immediate);
#else
        queue_task_irq(&task, &tq_immediate);
#endif
        mark_bh(IMMEDIATE_BH);
#endif                                                  /* BOTTOM_HELF */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
        return IRQ_HANDLED;
#endif
}

int _write_reg(ixpci_reg_t * reg, void __iomem *ioaddr[])
{
	/* Write to register
	 *
	 * Arguments:
	 *   reg      pointer to a structure ixpci_reg for register
	 *   ioaddr   mmio address for current device
	 *
	 * Returned:  SUCCESS or FAILURE */

	switch (reg->id) {
	case IXPCI_DO:  //0x24
		iowrite32((reg->value) & DIO_MASK, ioaddr[0] + DIO);
		break;
	case IXPCI_AIGR:  //0x0c
		iowrite32((reg->value) & RW_AICS_MASK, ioaddr[0] + RW_AICS);
		break;
	case IXPCI_AO:  //0x20
		iowrite32((reg->value) & RW_AOCS_MASK, ioaddr[0] + RW_AOCS);
		break;
	case IXPCI_AI_CF:  //0x04
		iowrite32((reg->value) & CF_MASK, ioaddr[0] + CF);
		break;
	case IXPCI_AI_ICCS:  //0x08
		iowrite32((reg->value) & RW_ICCS_MASK, ioaddr[0] + RW_ICCS);
		break;
	case IXPCI_ADST:  //0x10
		iowrite32((reg->value) & AIST_MASK, ioaddr[0] + AIST);
		break;
	case IXPCI_AI_DAS:  //0x14
		iowrite32((reg->value) & AI_DAS_MASK, ioaddr[0] + AI_DAS);
		break;
	case IXPCI_RS_IC:  //0x18
		iowrite32((reg->value) & RW_ICS_MASK, ioaddr[0] + RW_ICS);
		break;
	case IXPCI_CI:  //0x1c
		iowrite32((reg->value) & CI_MASK, ioaddr[0] + CI);
		break;
	case IXPCI_CN0CS:  //0x28
		iowrite32((reg->value) & CN0CS_MASK, ioaddr[0] + CN0CS);
		break;
	case IXPCI_CN0PCS:  //0x2c
		iowrite32((reg->value) & CN0PCS_MASK, ioaddr[0] + CN0PCS);
		break;
	case IXPCI_CN1CS:  //0x30
		iowrite32((reg->value) & CN1CS_MASK, ioaddr[0] + CN1CS);
		break;
	case IXPCI_CN1PCS:  //0x34
		iowrite32((reg->value) & CN1PCS_MASK, ioaddr[0] + CN1PCS);
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
	 *   reg      pointer to structure ixpci_reg for register
	 *   ioaddr   mmio address for current device
	 *
	 * Returned:  SUCCESS or FAILURE */
	switch (reg->id) {
	case IXPCI_DI:  //0x24
		reg->value = ioread32(ioaddr[0] + DIO) & DIO_MASK;
		break;
	case IXPCI_AI_FD:  //0x00
		reg->value = ioread32(ioaddr[0] + AI_FD) & AI_FD_MASK;
		break;
	case IXPCI_AI_FS:  //0x04
		reg->value = ioread32(ioaddr[0] + AI_FS) & AI_FS_MASK;
		break;
	case IXPCI_AO:  //0x20
		reg->value = ioread32(ioaddr[0] + RW_AOCS) & RW_AOCS_MASK;
		break;
	case IXPCI_GCID:  //0x3c
		reg->value = ioread32(ioaddr[0] + CID) & CID_MASK;
		break;
	case IXPCI_AI_L:  //0x10
		reg->value = ioread32(ioaddr[0] + AI_L) & AI_L_MASK;
		break;
	case IXPCI_AIGR:  //0x0c
		reg->value = ioread32(ioaddr[0] + RW_AICS) & RW_AICS_MASK;
                break;
	case IXPCI_AI_H:  //0x14
		reg->value = ioread32(ioaddr[0] + AI_H) & AI_H_MASK;
		break;
	case IXPCI_AI_ICCS:  //0x08
		reg->value = ioread32(ioaddr[0] + RW_ICCS) & RW_ICCS_MASK;
		break;
	case IXPCI_RS_IC:  //0x18
		reg->value = ioread32(ioaddr[0] + RW_ICS) & RW_ICS_MASK;
		break;
	case IXPCI_CN0CS:  //0x28
		reg->value = ioread32(ioaddr[0] + CN0CS) & CN0CS_MASK;
		break;
	case IXPCI_CN0PCS:  //0x2c
		reg->value = ioread32(ioaddr[0] + CN0PCS) & CN0PCS_MASK;
		break;
	case IXPCI_CN1CS:  //0x30
		reg->value = ioread32(ioaddr[0] + CN1CS) & CN1CS_MASK;
		break;
	case IXPCI_CN1PCS:  //0x34
		reg->value = ioread32(ioaddr[0] + CN1PCS) & CN1PCS_MASK;
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

int _set_signal(ixpci_signal_t * sig, ixpci_devinfo_t * dp)
{
	/* set user signal for interrupt
	 *
	 * Arguments:
	 *   sig      user signal
	 *   dp       current device
	 *
	 * Returned:  SUCCESS or FAILURE */

        if (sig->sid) {
                /* has signal id, set */
                if (sig->pid) {
                        /* user gives his process id */
                        dp->sig.sid = sig->sid;
                        dp->sig.pid = sig->pid;
                        /* user gives his process id */
                        #if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,31)
                        dp->sig.task = find_task_by_pid(sig->pid,&init_pid_ns);

                        #elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
                        dp->sig.task = find_task_by_pid_type_ns(PIDTYPE_PID,sig->pid,&init_pid_ns);
                        #else
                        dp->sig.task = find_task_by_pid(sig->pid);
                        #endif


                } else if (sig->task) 
		{
                        /* user gives his task structure */
                        dp->sig.sid = sig->sid;
                        dp->sig.task = sig->task;
			dp->sig.bedge = sig->bedge;
			dp->sig.edge = sig->edge;
                } else 
		{
                        /* wrong! */
                        return FAILURE;
                }
        } else {
                /* no signal id, clear */
                dp->sig.sid = 0;
                dp->sig.pid = 0;
                dp->sig.task = 0;
                dp->sig.is = 0;
                dp->sig.edge = 0;
                dp->sig.bedge = 0;
        }
        return SUCCESS;
}

int _read_eep(ixpci_eep_t * eep, ixpci_devinfo_t * dp)
{
	/* Read EEPROM calibration value
	 *
	 * Arguments:
	 *    eep	array for calibration
	 *    dp	current device
	 *
	 * Returned:  SUCCESS or FAILURE
	 */

	unsigned long CMD, CMDcount = 0, CMDdata, timeout = 0, temp, FlashData;
	unsigned short index;
	unsigned char read_eep[256], BufCRC_L, BufCRC_H;
	int t=0;
	//unsigned char BufData, IndexCRC;

	/* initialize Daul Port Ram */
/*	for(index = 0; index < 8; index++)
        {
                iowrite32(0x0, dp->ioaddr[0] + (index * 0x20) + 0x40c);  //CMD BUSY
                iowrite32(0x5a00, dp->ioaddr[0] + (index * 0x20) + 0x41c);  //PCIE CHK
        }
*/
	eep->offset = 0;
	eep->uBLOCK = 0;
	eep->count = 256;	//128*2

	CMD = CMD_READ|(eep->uBLOCK+1);
	CMDdata = eep->count - 1;
	CMDdata = (CMDdata << 24)&0xFFFFFFFF;

	/* Write to Flash */
	iowrite32(CMDcount, dp->ioaddr[0] + REG_COUNT);	//CommandCount
	iowrite32(CMD, dp->ioaddr[0] + REG_COMMAND);	//Command
	iowrite32(CMDdata, dp->ioaddr[0] + REG_DATA);	//CommandData
	iowrite32(STA_COMREADY, dp->ioaddr[0] + REG_STATUS_HOST);	//Command is ready

	mdelay(50);	//in dos code use delay(100) -> 100ms*4

	while(1)
	{
		//Read Command Status
		temp = ioread32(dp->ioaddr[0] + REG_STATUS_DEVICE);  //Read status of command
		
		if(temp == STA_COMVALID)  //Command valid
		{
			iowrite32(STA_COMBUSY, dp->ioaddr[0] + REG_STATUS_HOST);  //Command isn't ready
			break;
		}else if((temp != STA_BUSY) && (temp != STA_PCIECHK))  //Command invalid
		{
			//Clear Command setup
			iowrite32(STA_COMBUSY, dp->ioaddr[0] + REG_STATUS_HOST);  //Command isn't ready
			iowrite32(STA_PCIECHK, dp->ioaddr[0] + REG_STATUS_DEVICE);  //PCIe check finished
			switch(temp)
			{
				case STA_ERRCRC:	// Check CRC error
					return FAILURE;
					break;
				case STA_OVERADDRESS:	// Over Flash Address Definition
					return FAILURE;
					break;
				case STA_ERRCOM:	// Command Error
					return FAILURE;
					break;
				case STA_ERRFLASHCTL:	// Flash Control Error
					return FAILURE;
					break;
				default:
					return FAILURE;
					break;
			}
			break;
		}

		timeout++;
		if(timeout > 100000)	
		{
			//Clear Command setup
        	        iowrite32(STA_COMBUSY, dp->ioaddr[0] + REG_STATUS_HOST);  //Command isn't ready
                	iowrite32(STA_PCIECHK, dp->ioaddr[0] + REG_STATUS_DEVICE);  //PCIe check finished
			return FAILURE;
		}
	}

	// Read Flash Data
	BufCRC_L = BufCRC_H = 0xff;
	for(index = 0; index < eep->count; index+=4)
	{
		FlashData = ioread32(dp->ioaddr[0] + 0x600 + index);  //Read status of command
		//printk("FlashData value 0x%lx\n",FlashData);

		read_eep[index+0] = (unsigned char)(FlashData & 0xff);
		read_eep[index+1] = (unsigned char)((FlashData >> 8) & 0xff);
		read_eep[index+2] = (unsigned char)((FlashData >> 16) & 0xff);
		read_eep[index+3] = (unsigned char)((FlashData >> 24) & 0xff);

		if(t == 64)
			printk("\n\n\n\n\n");
		printk("eeprom 0x%x   0x%x 0x%x \n        0x%x   0x%x 0x%x\n",t,read_eep[index+1],read_eep[index+0],t+1,read_eep[index+3],read_eep[index+2]);
		t+=2;
/*
		BufData = (unsigned char)(FlashData & 0xff);
		IndexCRC = BufCRC_L ^ BufData;
		BufCRC_L = BufCRC_H ^ CRC16Hi[IndexCRC];
		BufCRC_H = CRC16Lo[IndexCRC];
		if(++index > (eep->count-1)) break;

		BufData = (unsigned char)((FlashData >> 8) & 0xff);
		IndexCRC = BufCRC_L ^ BufData;
		BufCRC_L = BufCRC_H ^ CRC16Hi[IndexCRC];
		BufCRC_H = CRC16Lo[IndexCRC];
		if(++index > (eep->count-1)) break;

		BufData = (unsigned char)((FlashData >> 16) & 0xff);
		IndexCRC = BufCRC_L ^ BufData;
		BufCRC_L = BufCRC_H ^ CRC16Hi[IndexCRC];
		BufCRC_H = CRC16Lo[IndexCRC];
		if(++index > (eep->count-1)) break;

		BufData = (unsigned char)((FlashData >> 24) & 0xff);
		IndexCRC = BufCRC_L ^ BufData;
		BufCRC_L = BufCRC_H ^ CRC16Hi[IndexCRC];
		BufCRC_H = CRC16Lo[IndexCRC];
		if(++index > (eep->count-1)) break;
*/
	}

	/* Clear Command setup */
	iowrite32(STA_COMBUSY, dp->ioaddr[0] + REG_STATUS_HOST);  //Command isn't ready
	iowrite32(STA_PCIECHK, dp->ioaddr[0] + REG_STATUS_DEVICE);  //PCIe check finished

	for(index = 0; index < 128; index++)
	{
		*(eep->read_eep+index) = (unsigned int)((read_eep[index*2+1]*256+read_eep[index*2+0])&0xffff);
		//printk("eep->read_eep[%d] = 0x%x\n",index,eep->read_eep[index]);
	}

	eep->AI_ch = dp->AI_ch;  //Send AI_ch value to user space
	eep->AO_ch = dp->AO_ch;

	printk("end for loop\n");

	return SUCCESS;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
long ixpcie8622_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param)
#else
int ixpcie8622_ioctl(struct inode *inode, struct file *file, unsigned int ioctl_num, unsigned long ioctl_param)
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
	ixpci_signal_t signal;
	ixpci_eep_t eep;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
        int minor;

        if(file->private_data)
                minor = *((int *)file->private_data);
        else
                return -EINVAL;

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

                if (_read_reg(&ixpci_reg, dp->ioaddr))
                        return FAILURE;

                if(copy_to_user(argp, &ixpci_reg, sizeof(ixpci_reg_t)))
                {
                        printk("XXX %s ... IXPCI_READ_REG ... copy_to_user fail\n",__FUNCTION__);
                        return -EFAULT;
                }
#else
		if (_read_reg((ixpci_reg_t *) ioctl_param, dp->ioaddr))
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

                if (_write_reg(&ixpci_reg, dp->ioaddr))
#else
		if (_write_reg((ixpci_reg_t *) ioctl_param, dp->ioaddr))
#endif
			return FAILURE;
		break;
	case IXPCI_SET_SIG:
#if 1
                if(copy_from_user(&signal, argp, sizeof(ixpci_signal_t)))
                {
                        printk("XXX %s ... IXPCI_SET_SIG ... copy_from_user fail\n",__FUNCTION__);
                        return -EFAULT;
                }

                if (_set_signal(&signal, dp))
#else
                if (_set_signal((ixpci_signal_t *) ioctl_param, dp))
#endif
                        return FAILURE;
		break;
	case IXPCI_READ_EEP:
#if 1
                if(copy_from_user(&eep, argp, sizeof(ixpci_eep_t)))
                {
                        printk("XXX %s ... IXPCI_READ_EEP ... copy_from_user fail\n",__FUNCTION__);
                        return -EFAULT;
                }

                if (_read_eep(&eep, dp))
			return FAILURE;

		if(copy_to_user(argp, &eep, sizeof(ixpci_eep_t)))
                {
                        printk("XXX %s ... IXPCI_READ_EEP ... copy_to_user fail\n",__FUNCTION__);
                        return -EFAULT;
                }
#else
		if (_read_eep((ixpci_eep_t *) ioctl_param, dp))
			return FAILURE;
#endif
                break;
	default:
		return -EINVAL;
	}
	return SUCCESS;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
int ixpcie8622_release(struct inode *inode, struct file *file)
#else
void ixpcie8622_release(struct inode *inode, struct file *file)
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
	ixpci_signal_t sig;

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
		_disable_irq(dp);
		
		/* clear signal setting */
		sig.sid = 0;
		_set_signal(&sig, dp);

		free_irq(dp->irq, dp);

		for (i = 0; i < PBAN; i++)
		{
			iounmap(dp->ioaddr[i]);
			if(dp->ioaddr[i])
				release_mem_region(dp->base[i], dp->range[i]);
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
	open:ixpcie8622_open,
	release:ixpcie8622_release,

# if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
	unlocked_ioctl:ixpcie8622_ioctl,
#else
	ioctl:ixpcie8622_ioctl,
#endif
};

int ixpcie8622_open(struct inode *inode, struct file *file)
{
	/* (export)
	 *
	 * This function is called by ixpci.o whenever a process attempts to
	 * open the device file of PCIe-8622
	 *
	 * Arguments: read <linux/fs.h> for (*open) of struct file_operations
	 *
	 * Returned:  none */

	int minor, i;
	ixpci_devinfo_t *dp;
	ixpci_signal_t sig;

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
	sig.sid = 0;
	_set_signal(&sig, dp);

	/* request mmio region */
	for (i = 0; i < PBAN; i++)
	{
# if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	  if (!dp->base[i] || !dp->range[i] || check_region(dp->base[i], dp->range[i]))
			continue;
#endif
 	  request_mem_region(dp->base[i], dp->range[i], MODULE_NAME);
	  dp->ioaddr[i] = ioremap(dp->base[i], dp->range[i]);
	}

	/* disable board interrupt */
	_disable_irq(dp);
        
	/* Clear all interrupt */
	iowrite32(0x0 & CI_MASK, dp->ioaddr[0] + CI);

	/* install ISR */
        if (request_irq(dp->irq, irq_handler, SA_SHIRQ, dp->name, dp)) {
                --(dp->open);
                return -EBUSY;
        }

	/* initialize Daul Port Ram */
        for(i = 0; i < 8; i++)
        {
                iowrite32(0x0, dp->ioaddr[0] + (i * 0x20) + 0x40c);  //CMD BUSY
                iowrite32(0x5a00, dp->ioaddr[0] + (i * 0x20) + 0x41c);  //PCIE CHK
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
//		_reset_dev(dp);
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

	/* align to first PCIe-8622 in ixpci list */
	for (dev = ixpci_dev; dev && dev->id != PCIe_8622; dev = dev->next);

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
		//_reset_dev(dp);
		printk(".");
	}

	printk(" ready.\n");
	return SUCCESS;
}
