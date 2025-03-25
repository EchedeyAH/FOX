/* PCI-D96SU/D128SU

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

   v 0.0.1 4 May 2020 by Winson Chen
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
#include "_pcid96_d128.h"

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
EXPORT_SYMBOL_GPL(ixpcid96_ioctl);
EXPORT_SYMBOL_GPL(ixpcid96_release);
EXPORT_SYMBOL_GPL(ixpcid96_open);
#else
EXPORT_SYMBOL_NOVERS(ixpcid96_ioctl);
EXPORT_SYMBOL_NOVERS(ixpcid96_release);
EXPORT_SYMBOL_NOVERS(ixpcid96_open);
#endif

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
	unsigned int flag, lIntEnVal, lActVal, value;
	int bIndex;

#ifdef BOTTOM_HELF
        static struct tq_struct task = { NULL, 0, irq_handler_bh, NULL };

        task.data = dev_id;
#endif                                                  /* BOTTOM_HELF */
        dp = (ixpci_devinfo_t *) dev_id;

	//Read Register 30H
	flag = ioread32(dp->ioaddr[0] + _PPM_CS) & _PPM_CS_MASK;
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

	/* Clear all interrupt */
	//Check each port Compare trigger interrupt
	for (bIndex=0; bIndex<4; bIndex++)
	{
		//Clear portA/B/C/D Compare trigger Source
		if(((lActVal&0xF)>>bIndex)&0x1)
		{
			//Clear
			value = ioread32(dp->ioaddr[0] + _PA_CVS + 0x4 * bIndex);
			iowrite32(value, dp->ioaddr[0] + _PA_CVS + 0x4 * bIndex);
		}
	}

	//Clear PortA Change Status interrupt
	if((lActVal>>4)&0x1)
		iowrite32(0x0, dp->ioaddr[0] + _PA_CCS); //off

	//Clear PortB Change Status interrupt
	if((lActVal>>5)&0x1)
		iowrite32(0x0, dp->ioaddr[0] + _PB_CCS); //off

        /* User's signal condition */
        if (dp->sig.sid)
                send_sig(dp->sig.sid, dp->sig.task, 1);

#ifdef BOTTOM_HELF
        /* Scheduale bottom half to run */
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

        switch(reg->id){
	case IXPCI_DIOPA_CR:	//Set the port's direction control	0-> DI, 1-> DO
#if _DIOP_CR_MASK == 0xffffffff
		iowrite32((reg->value), ioaddr[0] + _DIO_PA_CR);
#else
		iowrite32((reg->value) & _DIOP_CR_MASK, ioaddr[0] + _DIO_PA_CR);
#endif
                break;
	case IXPCI_DIOPA:	//portA DO output
#if _DIOP_MASK == 0xffffffff
		iowrite32((reg->value), ioaddr[0] + _DIO_PA);
#else
		iowrite32((reg->value) & _DIOP_MASK, ioaddr[0] + _DIO_PA);
#endif
		break;
	case IXPCI_DIOPB_CR:	//Set the port's direction control	0-> DI, 1-> DO
#if _DIOP_CR_MASK == 0xffffffff
		iowrite32((reg->value), ioaddr[0] + _DIO_PB_CR);
#else
		iowrite32((reg->value) & _DIOP_CR_MASK, ioaddr[0] + _DIO_PB_CR);
#endif
                break;
	case IXPCI_DIOPB:	//portB DO output
#if _DIOP_MASK == 0xffffffff
		iowrite32((reg->value), ioaddr[0] + _DIO_PB);
#else
		iowrite32((reg->value) & _DIOP_MASK, ioaddr[0] + _DIO_PB);
#endif
		break;
	case IXPCI_DIOPC_CR:	//Set the port's direction control	0-> DI, 1-> DO
#if _DIOP_CR_MASK == 0xffffffff
		iowrite32((reg->value), ioaddr[0] + _DIO_PC_CR);
#else
		iowrite32((reg->value) & _DIOP_CR_MASK, ioaddr[0] + _DIO_PC_CR);
#endif
                break;
	case IXPCI_DIOPC:	//portC DO output
#if _DIOP_MASK == 0xffffffff
		iowrite32((reg->value), ioaddr[0] + _DIO_PC);
#else
		iowrite32((reg->value) & _DIOP_MASK, ioaddr[0] + _DIO_PC);
#endif
		break;
	case IXPCI_DIOPD_CR:	//Set the port's direction control	0-> DI, 1-> DO	 PCI-D128 only
#if _DIOP_CR_MASK == 0xffffffff
		iowrite32((reg->value), ioaddr[0] + _DIO_PD_CR);
#else
		iowrite32((reg->value) & _DIOP_CR_MASK, ioaddr[0] + _DIO_PD_CR);
#endif
                break;
	case IXPCI_DIOPD:	//portD DO output	PCI-D128 only
#if _DIOP_MASK == 0xffffffff
		iowrite32((reg->value), ioaddr[0] + _DIO_PD);
#else
		iowrite32((reg->value) & _DIOP_MASK, ioaddr[0] + _DIO_PD);
#endif
		break;
	case IXPCI_PA_CVS:
#if _PORT_CVS_MASK == 0xffffffff
		iowrite32((reg->value), ioaddr[0] + _PA_CVS);
#else
		iowrite32((reg->value) & _PORT_CVS_MASK, ioaddr[0] + _PA_CVS);
#endif
		break;
	case IXPCI_PB_CVS:
#if _PORT_CVS_MASK == 0xffffffff
		iowrite32((reg->value), ioaddr[0] + _PB_CVS);
#else
		iowrite32((reg->value) & _PORT_CVS_MASK, ioaddr[0] + _PB_CVS);
#endif
		break;
	case IXPCI_PC_CVS:
#if _PORT_CVS_MASK == 0xffffffff
		iowrite32((reg->value), ioaddr[0] + _PC_CVS);
#else
		iowrite32((reg->value) & _PORT_CVS_MASK, ioaddr[0] + _PC_CVS);
#endif
		break;
	case IXPCI_PD_CVS:
#if _PORT_CVS_MASK == 0xffffffff
		iowrite32((reg->value), ioaddr[0] + _PD_CVS);
#else
		iowrite32((reg->value) & _PORT_CVS_MASK, ioaddr[0] + _PD_CVS);
#endif
		break;
	case IXPCI_PPM_CS:
#if _PPM_CS_MASK == 0xffffffff
		iowrite32((reg->value), ioaddr[0] + _PPM_CS);
#else
		iowrite32((reg->value) & _PPM_CS_MASK, ioaddr[0] + _PPM_CS);
#endif
		break;
	case IXPCI_PA_CCS:
#if _PORT_CCS_MASK == 0xffffffff
		iowrite32((reg->value), ioaddr[0] + _PA_CCS);
#else
		iowrite32((reg->value) & _PORT_CCS_MASK, ioaddr[0] + _PA_CCS);
#endif
		break;
	case IXPCI_PB_CCS:
#if _PORT_CCS_MASK == 0xffffffff
		iowrite32((reg->value), ioaddr[0] + _PB_CCS);
#else
		iowrite32((reg->value) & _PORT_CCS_MASK, ioaddr[0] + _PB_CCS);
#endif
		break;
	case IXPCI_PPC:
#if _PPC_MASK == 0xffffffff
		iowrite32((reg->value), ioaddr[0] + _PPC);
#else
		iowrite32((reg->value) & _PPC_MASK, ioaddr[0] + _PPC);
#endif
		break;
	case IXPCI_PPN:
#if _PPN_MASK == 0xffffffff
		iowrite32((reg->value), ioaddr[0] + _PPN);
#else
		iowrite32((reg->value) & _PPN_MASK, ioaddr[0] + _PPN);
#endif
		break;
	case IXPCI_PPO:
#if _PPO_MASK == 0xffffffff
		iowrite32((reg->value), ioaddr[0] + _PPO);
#else
		iowrite32((reg->value) & _PPO_MASK, ioaddr[0] + _PPO);
#endif
		break;
	case IXPCI_PDS:
#if _PDS_MASK == 0xffffffff
		iowrite32((reg->value), ioaddr[1] + _PDS + (reg->id_offset));
#else
		iowrite32((reg->value) & _PDS_MASK, ioaddr[1] + _PDS + (reg->id_offset));
#endif
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
	case IXPCI_DIOPA_CR:    //Read back the current port's direction control	0-> DI, 1-> DO
#if _DIOP_CR_MASK == 0xffffffff
		reg->value = ioread32(ioaddr[0] + _DIO_PA_CR);
#else
		reg->value = ioread32(ioaddr[0] + _DIO_PA_CR) & _DIOP_CR_MASK;
#endif
                break;
	case IXPCI_DIOPA:    //portA DI read back
#if _DIOP_MASK == 0xffffffff
		reg->value = ioread32(ioaddr[0] + _DIO_PA);
#else
		reg->value = ioread32(ioaddr[0] + _DIO_PA) & _DIOP_MASK;
#endif
                break;
        case IXPCI_DIOPB_CR:    //Read back the current port's direction control	0-> DI, 1-> DO
#if _DIOP_CR_MASK == 0xffffffff
		reg->value = ioread32(ioaddr[0] + _DIO_PB_CR);
#else
		reg->value =ioread32 (ioaddr[0] + _DIO_PB_CR) & _DIOP_CR_MASK;
#endif
                break;
	case IXPCI_DIOPB:    //portB DI read back
#if _DIOP_MASK == 0xffffffff
		reg->value = ioread32(ioaddr[0] + _DIO_PB);
#else
		reg->value = ioread32(ioaddr[0] + _DIO_PB) & _DIOP_MASK;
#endif
                break;
        case IXPCI_DIOPC_CR:    //Read back the current port's direction control	0-> DI, 1-> DO
#if _DIOP_CR_MASK == 0xffffffff
		reg->value = ioread32(ioaddr[0] + _DIO_PC_CR);
#else
		reg->value = ioread32(ioaddr[0] + _DIO_PC_CR) & _DIOP_CR_MASK;
#endif
                break;
	case IXPCI_DIOPC:    //portC DI read back
#if _DIOP_MASK == 0xffffffff
		reg->value = ioread32(ioaddr[0] + _DIO_PC);
#else
		reg->value = ioread32(ioaddr[0] + _DIO_PC) & _DIOP_MASK;
#endif
                break;
        case IXPCI_DIOPD_CR:    //Read back the current port's direction control	0-> DI, 1-> DO		PCI-D128 only
#if _DIOP_CR_MASK == 0xffffffff
		reg->value = ioread32(ioaddr[0] + _DIO_PD_CR);
#else
		reg->value = ioread32(ioaddr[0] + _DIO_PD_CR) & _DIOP_CR_MASK;
#endif
                break;
	case IXPCI_DIOPD:    //portD DI read back	PCI-D128 only
#if _DIOP_MASK == 0xffffffff
		reg->value = ioread32(ioaddr[0] + _DIO_PD);
#else
		reg->value = ioread32(ioaddr[0] + _DIO_PD) & _DIOP_MASK;
#endif
                break;
        case IXPCI_PA_CVS:
#if _PORT_CVS_MASK == 0xffffffff
		reg->value = ioread32(ioaddr[0] + _PA_CVS);
#else
		reg->value = ioread32(ioaddr[0] + _PA_CVS) & _PORT_CVS_MASK;
#endif
                break;
        case IXPCI_PB_CVS:
#if _PORT_CVS_MASK == 0xffffffff
		reg->value = ioread32(ioaddr[0] + _PB_CVS);
#else
		reg->value = ioread32(ioaddr[0] + _PB_CVS) & _PORT_CVS_MASK;
#endif
                break;
        case IXPCI_PC_CVS:
#if _PORT_CVS_MASK == 0xffffffff
		reg->value = ioread32(ioaddr[0] + _PC_CVS);
#else
		reg->value = ioread32(ioaddr[0] + _PC_CVS) & _PORT_CVS_MASK;
#endif
                break;
        case IXPCI_PD_CVS:
#if _PORT_CVS_MASK == 0xffffffff
		reg->value = ioread32(ioaddr[0] + _PD_CVS);
#else
		reg->value = ioread32(ioaddr[0] + _PD_CVS) & _PORT_CVS_MASK;
#endif
                break;
        case IXPCI_PPM_CS:
#if _PPM_CS_MASK == 0xffffffff
		reg->value = ioread32(ioaddr[0] + _PPM_CS);
#else
		reg->value = ioread32(ioaddr[0] + _PPM_CS) & _PPM_CS_MASK;
#endif
                break;
        case IXPCI_PPN:
#if _PPN_MASK == 0xffffffff
		reg->value = ioread32(ioaddr[0] + _PPN);
#else
		reg->value = ioread32(ioaddr[0] + _PPN) & _PPN_MASK;
#endif
                break;
        case IXPCI_PPO:
#if _PPO_MASK == 0xffffffff
		reg->value = ioread32(ioaddr[0] + _PPO);
#else
		reg->value = ioread32(ioaddr[0] + _PPO) & _PPO_MASK;
#endif
                break;
        case IXPCI_PDS:
#if _PDS_MASK == 0xffffffff
		reg->value = ioread32(ioaddr[1] + _PDS + (reg->id_offset));
#else
		reg->value = ioread32(ioaddr[1] + _PDS + (reg->id_offset)) & _PDS_MASK;
#endif
                break;
        default:
                return FAILURE;
        }
        return SUCCESS;
}

int set_signal(ixpci_signal_t * sig, ixpci_devinfo_t * dp)
{
	/* set user signal for interrupt
         *
         * Arguments:
         *   sig      user signal
         *   di       current device
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

                } else if (sig->task) {
                        /* user gives his task structure */
                        dp->sig.sid = sig->sid;
                        dp->sig.task = sig->task;
                } else {
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

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
long ixpcid96_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param)
#else
int ixpcid96_ioctl(struct inode *inode, struct file *file, unsigned int ioctl_num, unsigned long ioctl_param)
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
	ixpci_signal_t signal;
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
		card_id = ioread32(dp->ioaddr[0] + _GCID) & _GCID_MASK;
                if(copy_to_user(argp, &card_id, sizeof(unsigned int)))
                {
                        printk("XXX %s ... IXPCI_GCID ... copy_to_user fail\n",__FUNCTION__);
                        return -EFAULT;
                }
                break;
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
	case IXPCI_IRQ_ENABLE:
		_enable_irq(dp);
		break;
	case IXPCI_IRQ_DISABLE:
		_disable_irq(dp);
		break;
	default:
		return -EINVAL;
	}
	return SUCCESS;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
int ixpcid96_release(struct inode *inode, struct file *file)
#else
void ixpcid96_release(struct inode *inode, struct file *file)
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
		sig.sid = 0;
		set_signal(&sig, dp);
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
	open:ixpcid96_open,
	release:ixpcid96_release,

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
	unlocked_ioctl:ixpcid96_ioctl,
#else
	ioctl:ixpcid96_ioctl,
#endif
};

int ixpcid96_open(struct inode *inode, struct file *file)
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
        set_signal(&sig, dp);

	/* request io region */
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

        /* install ISR */
        if (request_irq(dp->irq, irq_handler, SA_SHIRQ, dp->name, dp)) {
                --(dp->open);
                return -EBUSY;
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

	/* align to first PCI-D96SU or PCI-D128SU in ixpci list */
	for (dev = ixpci_dev; dev && (dev->id != PCI_D96) && (dev->id != PCI_D128); dev = dev->next);

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
