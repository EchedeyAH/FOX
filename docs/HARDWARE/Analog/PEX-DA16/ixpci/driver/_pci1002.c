/* PCI-1002 Service Module

   Author: Reed Lai

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General :ublic License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. */

/* File level history (record changes for this file here.)
   v 0.3.1 11 Nov 2014 by Winson Chen
     Give support to linux kernel 3.16.0.
     Remove EXPORT_SYMBOL_GPL().

   v 0.3.0 1 Apr 2011 by Golden Wang
     Give support to linux kernel 2.6.37.

   v 0.2.0 18 Feb 2011 by Golden Wang
     Fixed the error of getting dp->sig.task.

   v 0.1.1 16 May 2007 by Golden Wang
     Include "linux/cdev.h" after including "ixpci.h".

   v 0.1.0  8 Feb 2007 by Golden Wang
     Give support to linux kernel 2.6.x.

   v 0.0.3 22 Jul 2003 by Reed Lai
     Replaced the names disable_irq() and enbale_irq() with _disable_irq() and
	 _enable_irq() to avoid conflict when compiling.
   v 0.0.2  8 Jul 2003 by Reed Lai
     Fixed a bug about _align_minor().

   v 0.0.1 20 Mar 2003 by Reed Lai
     Fixed the bug that free_irq with different argument to the request_irq.
     That bug caused the kernel was unable to handle a NULL pointer when
     interrupting.

   v 0.0.0  7 Jan 2003 by Reed Lai
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
#include "_pci1002.h"

ixpci_devinfo_t *dev;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
int ixpci_minor[256] = {0};
#endif

#ifdef MODULE_LICENSE
MODULE_AUTHOR("Reed Lai <reed@icpdas.com>");
MODULE_DESCRIPTION("ICPDAS PCI-series driver, PCI-1002 service module");
MODULE_LICENSE(ICPDAS_LICENSE);
#endif

// EXPORT_NO_SYMBOLS; /* do not export symbols */
/*#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
EXPORT_SYMBOL_GPL(ixpci1002_ioctl);
EXPORT_SYMBOL_GPL(ixpci1002_release);
EXPORT_SYMBOL_GPL(ixpci1002_open);
#else
EXPORT_SYMBOL_NOVERS(ixpci1002_ioctl);
EXPORT_SYMBOL_NOVERS(ixpci1002_release);
EXPORT_SYMBOL_NOVERS(ixpci1002_open);
#endif
*/
void _disable_irq(ixpci_devinfo_t * dp)
{
	outb(BOARD_IRQ_DISABLE, dp->base[1] + _PICR);
}

void _enable_irq(ixpci_devinfo_t * dp)
{
	outb(BOARD_IRQ_ENABLE, dp->base[1] + _PICR);
}

void clear_int(ixpci_devinfo_t * dp)
{
	inb(dp->base[2] + _CI);
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
#endif							/* BOTTOM_HELF */

	dp = (ixpci_devinfo_t *) dev_id;
	if (!(inb(dp->base[1] + _PICR) & _PICR_INT_FLAG))
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
        return FAILURE;
#else
	return;
#endif

	clear_int(dp);
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
#endif							/* BOTTOM_HELF */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
        return IRQ_HANDLED;
#endif
}

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
	case IXPCI_PICR:
#if _PICR_MASK == 0xff
		outb(reg->value, base[1] + _PICR);
#else
		outb((reg->value) & _PICR_MASK, base[1] + _PICR);
#endif
		break;
	case IXPCI_8254C0:
#if _8254C0_MASK == 0xff
		outb(reg->value, base[2] + _8254C0);
#else
		outb((reg->value) & _8254C0_MASK, base[2] + _8254C0);
#endif
		break;
	case IXPCI_8254C1:
#if _8254C1_MASK == 0xff
		outb(reg->value, base[2] + _8254C1);
#else
		outb((reg->value) & _8254C1_MASK, base[2] + _8254C1);
#endif
		break;
	case IXPCI_8254C2:
#if _8254C2_MASK == 0xff
		outb(reg->value, base[2] + _8254C2);
#else
		outb((reg->value) & _8254C2_MASK, base[2] + _8254C2);
#endif
		break;
	case IXPCI_8254CR:
#if _8254CR_MASK == 0xff
		outb(reg->value, base[2] + _8254CR);
#else
		outb((reg->value) & _8254CR_MASK, base[2] + _8254CR);
#endif
		break;
	case IXPCI_AICR:
#if _AICR_MASK == 0xff
		outb(reg->value, base[2] + _AICR);
#else
		outb((reg->value), base[2] + _AICR);
#endif
		break;
	case IXPCI_AIGR:
#if _AIGR_MASK == 0xff
		outb(reg->value, base[2] + _AIGR);
#else
		outb((reg->value), base[2] + _AIGR);
#endif
		break;
	case IXPCI_CR:
#if _CR_MASK == 0xff
		outb(reg->value, base[2] + _CR);
#else
		outb((reg->value) & _CR_MASK, base[2] + _CR);
#endif
		break;
	case IXPCI_ADST:
#if _ADST_MASK == 0xff
		outb(reg->value, base[2] + _ADST);
#else
		outb((reg->value) & _ADST_MASK, base[2] + _ADST);
#endif
		break;
	case IXPCI_DO:
#if _DO_MASK == 0xffff
		outw(reg->value, base[2] + _DO);
#else
		outw((reg->value) & _DO_MASK, base[2] + _DO);
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
	case IXPCI_PICR:
#if _PICR_MASK == 0xff
		reg->value = inb(base[1] + _PICR);
#else
		reg->value = inb(base[1] + _PICR) & _PICR_MASK;
#endif
		break;
	case IXPCI_8254C0:
#if _8254C0_MASK == 0xff
		reg->value = inb(base[2] + _8254C0);
#else
		reg->value = inb(base[2] + _8254C0) & _8254C0_MASK;
#endif
		break;
	case IXPCI_8254C1:
#if _8254C1_MASK == 0xff
		reg->value = inb(base[2] + _8254C1);
#else
		reg->value = inb(base[2] + _8254C1) & _8254C1_MASK;
#endif
		break;
	case IXPCI_8254C2:
#if _8254C2_MASK == 0xff
		reg->value = inb(base[2] + _8254C2);
#else
		reg->value = inb(base[2] + _8254C2) & _8254C2_MASK;
#endif
		break;
	case IXPCI_SR:
#if _SR_MASK == 0xff
		reg->value = inb(base[2] + _SR);
#else
		reg->value = inb(base[2] + _SR) & _SR_MASK;
#endif
		break;
	case IXPCI_CI:
#if _CI_MASK == 0xff
		reg->value = inb(base[2] + _CI);
#else
		reg->value = inb(base[2] + _CI) & _CI_MASK;
#endif
		break;
	case IXPCI_DI:
#if _DI_MASK == 0xffff
		reg->value = inw(base[2] + _DI);
#else
		reg->value = inw(base[2] + _DI) & _DI_MASK;
#endif
		break;
	case IXPCI_AI:
		switch (reg->mode) {
		case IXPCI_RM_RAW:
#if _AI_MASK == 0xffff
			reg->value = inw(base[2] + _AI);
#else
			reg->value = inw(base[2] + _AI) & _AI_MASK;
#endif
			break;
		case IXPCI_RM_NORMAL:
			reg->value = inw(base[2] + _AI) & _AI_DATA_MASK;
			break;
		case IXPCI_RM_TRIGGER:
			outb(0, base[2] + _ADST);	/* software trigger */
			break;
		case IXPCI_RM_READY:
			while (!(inb(base[2] + _SR) & _AD_BUSY));
			/* FIXME - will cause system hang if ADC status failure */
			break;
		default:
			reg->value = inw(base[2] + _AI) & _AI_DATA_MASK;
			break;
		}
		break;
	case IXPCI_GCID:	//In my test one PEX-1002 card was failed.
#if _CID_MASK == 0xffff
		reg->value = inb(base[2] + _CID);
#else
		//reg->value = inb(base[2] + _CID) & _CID_MASK;
		reg->value = inb(base[2]);
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
	 * at the 4 MHz clock.
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

	if ((span > 16383) || (span == 0))
		return FAILURE;

	i = 0;
	sr = base[2] + _SR;

	counter = span * 4;
	lbyte = counter & 0xff;
	hbyte = (counter >> 8) & 0xff;

	outb(0xb0, base[2] + _8254CR);
	outb(lbyte, base[2] + _8254C2);
	outb(hbyte, base[2] + _8254C2);

	while (!(inb(sr) & _8254C2_STATUS)) {
		if (i > 1000000000)
			return FAILURE;
		/* XXX - fix me for timeout */
		++i;
	}
	return SUCCESS;
}

int _reset_dev(ixpci_devinfo_t * dp)
{
	/* stop timer 0 */
	outb(0x34, dp->base[2] + _8254CR);
	outb(0x01, dp->base[2] + _8254C0);
	outb(0x00, dp->base[2] + _8254C0);

	/* stop timer 1 */
	outb(0x74, dp->base[2] + _8254CR);
	outb(0x01, dp->base[2] + _8254C1);
	outb(0x00, dp->base[2] + _8254C1);

	/* stop timer 2 */
	outb(0xb0, dp->base[2] + _8254CR);
	outb(0x01, dp->base[2] + _8254C2);
	outb(0x00, dp->base[2] + _8254C2);

	/* reset control register to
	   general trigger mode,
	   no interrupt source (disable all interrupts)

	   There are two places to disable/enable the board interrupt,
	   one is _CR, another is _PICR. We touch only _CR here */
	outb(0, dp->base[2] + _CR);

	/* clear DO */
	outw(0, dp->base[2] + _DO);

	/* did I leak anything? */

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
long ixpci1002_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param)
#else
int ixpci1002_ioctl(struct inode *inode, struct file *file, unsigned int ioctl_num, unsigned long ioctl_param)
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
	unsigned int data;
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
	case IXPCI_IOCTL_DI:
#if 1
		if(copy_from_user(&data, argp, sizeof(unsigned int)))
		{
			printk("XXX %s ... IXPCI_IOCTL_DI ... copy_from_user fail\n",__FUNCTION__);
			return -EFAULT;
		}
		data = inw(dp->base[2] + _DI);

		if(copy_to_user(argp, &data, sizeof(unsigned int)))
		{
			printk("XXX %s ... IXPCI_IOCTL_DI ... copy_to_user fail\n",__FUNCTION__);
			return -EFAULT;
		}
#else
		*(unsigned int *) ioctl_param = inw(dp->base[2] + _DI);
#endif
		break;
	case IXPCI_IOCTL_DO:
#if 1
		if(copy_from_user(&data, argp, sizeof(unsigned int)))
		{
			printk("XXX %s ... IXPCI_IOCTL_DO ... copy_from_user fail\n",__FUNCTION__);
			return -EFAULT;
		}
		outw(data, dp->base[2] + _DO);
#else
		outw(*(unsigned int *) ioctl_param, dp->base[2] + _DO);
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
	default:
		return -EINVAL;
	}
	return SUCCESS;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
int ixpci1002_release(struct inode *inode, struct file *file)
#else
void ixpci1002_release(struct inode *inode, struct file *file)
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
		sig.sid = 0;
		set_signal(&sig, dp);
		free_irq(dp->irq, dp);
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
	open:ixpci1002_open,
	release:ixpci1002_release,
# if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
	unlocked_ioctl:ixpci1002_ioctl,
#else
	ioctl:ixpci1002_ioctl,
#endif
};

int ixpci1002_open(struct inode *inode, struct file *file)
{
	/* (export)
	 *
	 * This function is called by ixpci.o whenever a process attempts to
	 * open the device file of PCI-1002
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

	++(dp->open);				/* FIXME - should use spin-lock */
	if (dp->open > 1) {
		--(dp->open);
		return -EBUSY;
		/* if still opened by someone, get out */
	}

	/* clear signal setting */
	sig.sid = 0;
	set_signal(&sig, dp);

	/* disable board interrupt */
	outb(BOARD_IRQ_DISABLE, dp->base[1] + _PICR);

	/* install ISR */
	if (request_irq(dp->irq, irq_handler, SA_SHIRQ, dp->name, dp)) {
		--(dp->open);
		return -EBUSY;
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

	/* align to first PCI-1002 in ixpci list */
	for (dev = ixpci_dev; dev && dev->id != PCI_1002; dev = dev->next);

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
