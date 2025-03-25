/* Procfs interface for the PCI series device driver.

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

  v 0.1.1 26 Feb 2014 by Winson Chen
    To remove code for PISO-CAN/200/400 serial cards.

  v 0.1.0  5 DEC 2013 by Winson Chen
     Give support to linux kernel 3.12.0

  v 0.0.2  11 Sep 2002 by Reed Lai
     ixpci_cardname() ==> _pci_cardname()

  v 0.0.1  28 Dec 2001 by Reed Lai
     Fixed a bug that forgot to increase the present counter when
     a card had been found at kernel 2.4.

  v 0.0.0  2 Nov 2001 by Reed Lai
     Support Kernel 2.4.
     Separated from _pci.c.
     Create. */

#include <linux/proc_fs.h>
#include <linux/init.h>
#include <linux/seq_file.h>
#include "ixpci.h"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
int ixpci_get_info(char *buf, char **start, off_t offset, int buf_size)
#else
int ixpci_get_info(char *buf, char **start, off_t offset, int buf_size,
				   int unused)
#endif
{
	/* read file /proc/ixpci
	 *
	 * Arguments: as /proc filesystem, read <linux/proc_fs.h>
	 *
	 * Returned:  number of written bytes */

	char *p, *q, *l;
	unsigned int i, n, a, b, c, d;
	ixpci_devinfo_t *r;
	char my_buf[128];

	if (offset > 0)
		return 0;
	/* here, we assume the buf is always large
	   enough to hold all of our info data at
	   one fell swoop */

	p = buf;
	n = buf_size;

	/* export major number */
	sprintf(my_buf, "maj: %d\n", ixpci_major);
	q = my_buf;
	for (; n > 0 && *q != '\0'; --n) {	/* export characters */
		*p++ = *q++;
	}

	/* export module names */
	i = 0;
	l = "mod:";
	for (; n > 0 && *l != '\0'; --n) {
		*p++ = *l++;
	}
	while (ixpci_card[i].id) {	/* scan card list */
		if (ixpci_card[i].present) {	/* find present card */
			if (n > 0) {
				*p++ = ' ';
				--n;
			}
			q = ixpci_card[i].module;
			for (; n > 0 && *q != '\0'; --n) {	/* export card's module name */
				*p++ = *q++;
			}
		}
		++i;
	}
	if (n > 0) {				/* separator */
		*p++ = '\n';
		--n;
	}

	/* export device characters */
	r = ixpci_dev;
	while (r) {
		l = "dev: ";
		for (; n > 0 && *l != '\0'; --n) {	/* export card's module name */
			*p++ = *l++;
		}
		a = (r->id >> 48) & 0xffff;
		b = (r->id >> 32) & 0xffff;
		c = (r->id >> 16) & 0xffff;
		d = r->id & 0xffff;
		sprintf(my_buf,
				"ixpci%d %d 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%04x%04x%04x%04x %s\n",
				r->no, r->irq, r->base[0], r->base[1], r->base[2],
				r->base[3], r->base[4], r->base[5], a, b, c, d,
				(char *) _pci_cardname(r->id));
		q = my_buf;
		for (; n > 0 && *q != '\0'; --n) {	/* export characters */
			*p++ = *q++;
		}
		r = r->next;
	}

	return (p - buf - offset);	/* bye bye */
}

static int ixpci_proc_show(struct seq_file *m, void *v)
{
	unsigned int i = 0;
	unsigned int a, b, c, d;
	ixpci_devinfo_t *r;

	seq_printf(m, "maj: %d\n", ixpci_major);
	seq_printf(m, "%s", "mod:");

	while (ixpci_card[i].id) {      /* scan card list */
		if (ixpci_card[i].present) {    /* find present card */
			seq_printf(m, " ");
			seq_printf(m, "%s", ixpci_card[i].module);
		}
		++i;
	}

	seq_printf(m, "\n");

         /* export device characters */
	r = ixpci_dev;
	while (r) {
		
		seq_printf(m, "%s", "dev: ");

		a = (r->id >> 48) & 0xffff;
		b = (r->id >> 32) & 0xffff;
		c = (r->id >> 16) & 0xffff;
		d = r->id & 0xffff;

			seq_printf(m, "ixpci%d %d 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%04x%04x%04x%04x %s\n", r->no, r->irq, r->base[0], r->base[1], r->base[2], r->base[3], r->base[4], r->base[5], a, b, c, d, (char *) _pci_cardname(r->id));

		r = r->next;
	}

	return  0;

}
/* use in kernel version 2.4 to 3.10
static int ixpci_proc_output(char *buf)
{
        char *p;
        unsigned int i = 0;
	unsigned int a, b, c, d;
        ixpci_devinfo_t *r;
        //char my_buf[80];
        p = buf;
        // export major number 
        p += sprintf(p, "maj: %d\n", ixpci_major);
        p += sprintf(p, "%s", "mod:");

	 while (ixpci_card[i].id) {      // scan card list
                if (ixpci_card[i].present) {    // find present card
                        p += sprintf(p, " ");
                        p += sprintf(p, "%s", ixpci_card[i].module);
                }
                ++i;
        }
        p += sprintf(p, "\n");

         // export device characters 
        r = ixpci_dev;
        while (r) {
                p += sprintf(p, "%s", "dev: ");

		a = (r->id >> 48) & 0xffff;
                b = (r->id >> 32) & 0xffff;
                c = (r->id >> 16) & 0xffff;
                d = r->id & 0xffff;
		if ( r->id == PISO_CAN200 || r->id == PISO_CAN400 ||
		     r->id == PISO_CAN200U || r->id == PISO_CAN400U ||	
		     r->id == PISO_CAN200E || r->id == PISO_CAN400E ||
		     r->id == PCM_CAN200 || r->id == PCM_CAN400
		   )
		{
                	p += sprintf(p, "ixpci-can%d %d 0x%lx 0x%04x%04x%04x%04x %s\n", r->no, r->irq, r->base[r->canportid+1], a, b, c, d, (char *) _pci_cardname(r->id));

		}
		else
		{
                	p += sprintf(p, "ixpci%d %d 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%04x%04x%04x%04x %s\n", r->no, r->irq, r->base[0], r->base[1], r->base[2], r->base[3], r->base[4], r->base[5], a, b, c, d, (char *) _pci_cardname(r->id));
		}

                r = r->next;
        }
        return  p - buf;
}
*/

static int ixpci_proc_open(struct inode *inode, struct file *file)
{
        return single_open(file, ixpci_proc_show, NULL);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,6,0)
static const struct proc_ops proc_fops = {
	.proc_open	= ixpci_proc_open,
	.proc_read	= seq_read,
	.proc_release	= single_release,
};
#else
static const struct file_operations proc_fops = {
	.open	= ixpci_proc_open,
	.read	= seq_read,
	.release = single_release,
};
#endif

/*  use in kernel version 2.4 to 3.10
static int ixpci_read_proc(char *page, char **start, off_t off, int count, int *eof, void *data)
{
        int len = ixpci_proc_output (page);
        if (len <= off+count) *eof = 1;
        *start = page + off;
        len -= off;
        if (len>count)
                len = count;
        if (len<0)
                len = 0;

        return len;
}
*/

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
static struct proc_dir_entry *ixpci_proc_dir;
#else
struct proc_dir_entry ixpci_procfile = {
	low_ino:0,				/* inode number, 0, auto-filled */
	namelen:DEVICE_NAME_LEN,		/* length of proc file name */
	name:DEVICE_NAME,			/* proc file name */
	mode:S_IFREG | S_IRUGO,
	nlink:1,				/* number of links (where the file is
								   referenced) */
	uid:0,					/* user id, 0, root */
	gid:0,					/* group id, 0, root */
	size:80,				/* the size of the file reported by
								   command "ls" */
	get_info:ixpci_get_info,		/* the read function for this file the
								   function called when somebody tries to
								   read something from it */
};
#endif						/* LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0) */

void ixpci_proc_exit(void)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
	remove_proc_entry(DEVICE_NAME, ixpci_proc_dir);
	remove_proc_entry(FAMILY, NULL);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
	proc_unregister(&proc_root, ixpci_procfile.low_ino);
#endif
}

int ixpci_proc_init(void)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
	//ixpci_proc_dir = proc_mkdir(ORGANIZATION, 0);
	ixpci_proc_dir = proc_mkdir(FAMILY, 0);
	proc_create(DEVICE_NAME, 0, ixpci_proc_dir, &proc_fops);
	/* use in kernel version 2.4 to 3.10 */
	//create_proc_read_entry(DEVICE_NAME, 0, ixpci_proc_dir, ixpci_read_proc, NULL);

	//create_proc_info_entry(DEVICE_NAME, 0, ixpci_proc_dir, ixpci_get_info);
	return 0;
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
	return proc_register(&proc_root, &ixpci_procfile);
#else
	return proc_register_dynamic(&proc_root, &ixpci_procfile);
#endif
}
