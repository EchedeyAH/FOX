/* Signal condition utilities.

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

  v 0.1.1  24 Apr 2008 by Golden Wang
    Give support to linux kernel 2.6.22.
    
  v 0.1.0  28 Jan 2007 by Reed Lai
     Give support to linux kernel 2.6.x

  v 0.0.0  6 Aug 2003 by Reed Lai
    Hello world. */

/* *IDENT-OFF* */

#include <linux/version.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,11,0)
#include <linux/sched/signal.h>
#else
#include <linux/sched.h>
#endif

//#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)
#ifndef KBUILD_MODNAME
#define KBUILD_MODNAME ""
#endif

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,2,0) && LINUX_VERSION_CODE < KERNEL_VERSION(4,10,0)
#include <asm/uaccess.h>	/* copy_from_user() */
#else
#include <linux/uaccess.h>
#endif

#include "ixpio.h"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,31)
struct task_struct *find_task_by_pid(pid_t nr, struct pid_namespace *ns)
{
        return pid_task(find_pid_ns(nr, ns), PIDTYPE_PID);
}
#endif

__inline__ void ixpio_signaling(unsigned int is, ixpio_devinfo_t * dp)
{
	ixpio_signal_t *sig;

	sig = dp->sig;
	while (sig) {
		if (sig->sid) {  /* user signal is enabled */
			if (sig->is & is) {  /* an interrupt source */
				if (sig->bedge & is) {  /* both edges */
					send_sig(sig->sid, sig->task, 1);
				} else {  /* not both edges, then up to sig->edge */

					if (!((sig->edge & is) ^ (dp->is_edge & is))) {
						/* match edge */
						send_sig(sig->sid, sig->task, 1);
					}
				}
			}
		}
		sig = sig->next;
	}
}

void ixpio_clear_signals(ixpio_devinfo_t * dp)
{
	/* release all signals memory allocated for this device */

	ixpio_signal_t *sig, *prev;

	sig = dp->sig;

	while (sig) {
		prev = sig;
		sig = sig->next;
		kfree(prev);
	}
	dp->sig = 0;
}

int ixpio_del_signal(unsigned int id, ixpio_devinfo_t * dp)
{
	/* Delete signals with specified control id */

	ixpio_signal_t *sig, *found;

	found = 0; sig = dp->sig;

	while (sig) {
		if (sig->id == id) {
			found = sig;
			if (sig->prev) {
				sig->prev->next = sig->next;
				if (sig->next) sig->next->prev = sig->prev;
			} else {
				dp->sig = sig->next;
				dp->sig->prev = 0;
			}
			sig = sig->next;
			kfree(found);
		} else sig = sig->next;
	}
	return (found ? 0 : FAILURE);
}

int ixpio_add_signal(ixpio_signal_t * sp, ixpio_devinfo_t * dp)
{
	/* Add user's data to this device's signal list */
	ixpio_signal_t *sig, *prev;

	if (!sp || !sp->sid || !(sp->pid || sp->task))
		return FAILURE;

	prev = 0; sig = dp->sig;

	while (sig) {  /* to the list tail */
		prev = sig;
		sig = sig->next;
	}

	if (prev) {  /* signal list is already followed */
		sig = prev->next = kmalloc(sizeof(ixpio_signal_t), GFP_KERNEL);
		if (!sig) return FAILURE;
		memset(sig, 0, sizeof(ixpio_signal_t));
	} else {  /* signal list is empty, initiate */
		dp->sig = kmalloc(sizeof(ixpio_signal_t), GFP_KERNEL);
		if (!dp->sig) return FAILURE;
		memset(dp->sig, 0, sizeof(ixpio_signal_t));
		sig = (ixpio_signal_t *) dp->sig;
	}

	memcpy(sig, sp, sizeof(ixpio_signal_t));
	//printk("sig->sid is %d\n",sig->sid);
	/*if(copy_from_user(sig, sp, sizeof(ixpio_signal_t))) 
	{
		return -EFAULT;
	}*/

	sig->prev = prev;
	sig->next = 0;
	if (sig->pid) {
			/* user gives his process id */
			#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,31)
			sig->task = find_task_by_pid(sig->pid,&init_pid_ns);

			#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
			sig->task = find_task_by_pid_type_ns(PIDTYPE_PID,sig->pid,&init_pid_ns);
			#else
         	        sig->task = find_task_by_pid(sig->pid);
			#endif
	}
	return SUCCESS;
}
