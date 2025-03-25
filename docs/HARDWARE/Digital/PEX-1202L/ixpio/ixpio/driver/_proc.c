/* Procfs interface for the PIO series device driver.

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
  v 0.2.0 5 DEC 2013 by Winson Chen
     Give support to linux kernel 3.12.0

  v 0.1.0 28 Jan 2007 by Reed Lai
     Give support to linux kernel 2.6.x

  v 0.0.0 26 Jul 2002 by Reed Lai
    Support Kernel 2.4.
    Steal form IxPCI project.
    Create. */

#include <linux/proc_fs.h>
#include <linux/init.h>
#include <linux/version.h>
#include <linux/seq_file.h>

#include "ixpio.h"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
int ixpio_get_info(char *buf, char **start, off_t offset, int buf_size)
#else
int ixpio_get_info(char *buf, char **start, off_t offset, int buf_size,
				   int unused)
#endif
{
	/* read file /proc/ixpio
	 *
	 * Arguments: as /proc filesystem, read <linux/proc_fs.h>
	 *
	 * Returned:  number of written bytes */

	char *p, *q, *l;
	unsigned int i, n;
	ixpio_devinfo_t *r;
	char my_buf[80];

	if (offset > 0)
		return 0;
	/* here, we assume the buf is always large
	   enough to hold all of our info data at
	   one fell swoop */

	p = buf;
	n = buf_size;

	/* export major number */
	sprintf(my_buf, "maj: %d\n", ixpio_major);
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
	while (ixpio_card[i].id) {	/* scan card list */
		if (ixpio_card[i].present) {	/* find present card */
			if (n > 0) {
				*p++ = ' ';
				--n;
			}
			q = ixpio_card[i].module;
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
	r = ixpio_dev;
	while (r) {
		l = "dev: ";
		for (; n > 0 && *l != '\0'; --n) {	/* export card's module name */
			*p++ = *l++;
		}
		sprintf(my_buf,
				"ixpio%d 0x%lx 0x%x %s\n",
				r->no, r->base, r->csid, (char *) _pio_cardname(r->csid));
		q = my_buf;
		for (; n > 0 && *q != '\0'; --n) {	/* export characters */
			*p++ = *q++;
		}
		r = r->next;
	}

	return (p - buf - offset);	/* bye bye */
}

static int ixpio_proc_show(struct seq_file *m, void *v)
{
        unsigned int i = 0;
        ixpio_devinfo_t *r;

	/*
                 seq_printf(m,
                     "rtc_time\t: %02d:%02d:%02d.%02d\n"
                     "rtc_date\t: %04d-%02d-%02d\n",
                     tm.tm_hour, tm.tm_min, tm.tm_sec, hundredth,
                     tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
	*/

        /* export major number */

	seq_printf(m, "maj: %d\n", ixpio_major);
	seq_printf(m, "%s", "mod:");

	while (ixpio_card[i].id) {      /* scan card list */
		if (ixpio_card[i].present) {    /* find present card */
			seq_printf(m, " ");
			seq_printf(m, "%s", ixpio_card[i].module);
		}
		++i;
	}
	seq_printf(m, "\n");

        /* export device characters */
	r = ixpio_dev;
	while (r) {
	seq_printf(m, "%s", "dev: ");
	seq_printf(m, "ixpio%d 0x%lx 0x%x %s\n",
			r->no, r->base, r->csid, (char *) _pio_cardname(r->csid));
		r = r->next;
	}
	
	return 0;
}
/* use in kernel version 2.4 to 3.10
static int ixpio_proc_output(char *buf)
{
	char *p;
        unsigned int i = 0;
        ixpio_devinfo_t *r;
        //char my_buf[80];

	p = buf;
*/
	/*
		 p += sprintf(p,
                     "rtc_time\t: %02d:%02d:%02d.%02d\n"
                     "rtc_date\t: %04d-%02d-%02d\n",
                     tm.tm_hour, tm.tm_min, tm.tm_sec, hundredth,
                     tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
	*/
	
        /* export major number */
/*
	p += sprintf(p, "maj: %d\n", ixpio_major);
        p += sprintf(p, "%s", "mod:");

	while (ixpio_card[i].id) {      // scan card list 
                if (ixpio_card[i].present) {    // find present card 
			p += sprintf(p, " ");
                        p += sprintf(p, "%s", ixpio_card[i].module);
                }
                ++i;
        }
	p += sprintf(p, "\n");

	 // export device characters 
        r = ixpio_dev;
        while (r) {
        	p += sprintf(p, "%s", "dev: ");
                p += sprintf(p, "ixpio%d 0x%lx 0x%x %s\n",
                             r->no, r->base, r->csid, (char *) _pio_cardname(r->csid));
                r = r->next;
        }    
        
	return  p - buf;
}
*/

/*  use in kernel version 2.4 to 3.10
static int ixpio_read_proc(char *page, char **start, off_t off, int count, int *eof, void *data)
{
        int len = ixpio_proc_output (page);
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

static int ixpio_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, ixpio_proc_show, NULL);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,6,0)
static const struct proc_ops proc_fops = {
        .proc_open     =     ixpio_proc_open,
        .proc_read     =     seq_read,
        .proc_release  =     single_release,
};
#else
static const struct file_operations proc_fops = {
	.open	  =	ixpio_proc_open,
	.read	  =	seq_read,
	.release  =	single_release,
};
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
static struct proc_dir_entry *ixpio_proc_dir;
#else
struct proc_dir_entry ixpio_procfile = {
	low_ino:0,					/* inode number, 0, auto-filled */
	namelen:DEVICE_NAME_LEN,	/* length of proc file name */
	name:DEVICE_NAME,			/* proc file name */
	mode:S_IFREG | S_IRUGO,
	nlink:1,					/* number of links (where the file is
								   referenced) */
	uid:0,						/* user id, 0, root */
	gid:0,						/* group id, 0, root */
	size:80,					/* the size of the file reported by
								   command "ls" */
	get_info:ixpio_get_info,	/* the read function for this file the
								   function called when somebody tries to
								   read something from it */
};
#endif							/* LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0) */

void ixpio_proc_exit(void)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
	remove_proc_entry(DEVICE_NAME, ixpio_proc_dir);
	remove_proc_entry(FAMILY, NULL);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
	proc_unregister(&proc_root, ixpio_procfile.low_ino);
#endif
}

int ixpio_proc_init(void)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
	//ixpio_proc_dir = proc_mkdir(ORGANIZATION, 0);
	ixpio_proc_dir = proc_mkdir(FAMILY, 0);

	proc_create(DEVICE_NAME, 0, ixpio_proc_dir, &proc_fops);
	/* use in kernel version 2.4 to 3.10 */
	//create_proc_read_entry(DEVICE_NAME, 0, ixpio_proc_dir, ixpio_read_proc, NULL);
//	create_proc_info_entry(DEVICE_NAME, 0, ixpio_proc_dir, ixpio_get_info);
	return 0;
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
	return proc_register(&proc_root, &ixpio_procfile);
#else
	return proc_register_dynamic(&proc_root, &ixpio_procfile);
#endif
}
