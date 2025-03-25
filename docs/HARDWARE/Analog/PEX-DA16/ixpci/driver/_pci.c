/* PCI series device driver.

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
  v 0.14.2 14 Sep 2023 by Winson Chen
    Give support to PCI-2602U
 
  v 0.14.1 23 Mar 2017 by Golden Wang
    Give support to PCI-FC16U
  
  v 0.14.0 23 Sep 2015 by Wincon Chen
    Give support to PCIe-8622
  
  v 0.13.2 25 Nov 2014 by Winson Chen
    Give support to linux kernel 3.16.0.

  v 0.13.1 26 Feb 2014 by Winson Chen
    To remove code for PISO-CAN/200/400 serial cards.

  v 0.13.0 1 Nov 2013 by Winson Chen
    Give support to PCI-826.
    Give support to PCI-822.

  v 0.12.0 1 Apr 2011 by Golden Wang
    Give support to linux kernel 2.6.37.

  v 0.11.0 12 May 2010 by Golden Wang
    Give support to Lanner OEM I/O Card.

  v 0.10.0 16 Aug 2009 by Golden Wang
    Give support to PISO-CAN200/400 Series.

  v 0.9.1 16 May 2007 by Golden Wang
    Include "linux/cdev.h" after including "ixpci.h".

  v 0.9.0  8 Feb 2007 by Golden Wang
    Give support to linux kernel 2.6.x.

  v 0.8.1  8 Jul 2003 by Reed Lai
    Fixed a bug about _find_minor().

  v 0.8.0 11 Mar 2003 by Reed Lai
    Gives support to PCI-TMC12.
	Gives sub-verdor sub-device IDs.

  v 0.7.0  7 Jan 2003 by Reed Lai
    Adds the io range information for devinfo.

  v 0.6.0 11 Nov 2002 by Reed Lai
    Uses slab.h in place of malloc.h.
    Complies to the kernel module license check.

  v 0.5.3 25 Jul 2002 by Reed Lai
    _pio_cardname() ==> _pci_cardname()

  v 0.5.2 25 Jul 2002 by Reed Lai
    Just refines some codes and messages.

  v 0.5.1 16 May 2002 by Reed Lai
    Corrects the PCI-P16R16(series) service module name with "ixpcip16x16"

  v 0.5.0  28 Dec 2001 by Reed Lai
    Gives support to Kernel 2.4.

  v 0.4.0  1 Nov 2001 by Reed Lai
    Uses module_init() and module_exit() in place of init_module() and
	cleanup_module() for Kernel 2.4

  v 0.3.0 31 Oct 2001 by Reeed Lai
    Renames module_register_chrdev module_unregister_chrdev to
    devfs_register_chrdev and devfs_unregister_chrdev for Kernel 2.4.

  v 0.2.0 29 Oct 2001 by Reeed Lai
    Updates modules _find_dev() and _add_dev() for Kernel 2.4 compatibility.

  v 0.1.0 25 Oct 2001 by Reed Lai
    Re-filenames to _pci.c (from pdaq.c.)
    Changes all of "pdaq" to "ixpci."

  v 0.0.0 10 Apr 2001 by Reed Lai
    Create. */

/* *INDENT-OFF* */
#define IXPCI_RESERVED 0		/* 1 do compilation with reserved codes */

/* Mandatory */
#include <linux/kernel.h>		/* ta, kernel work */
#include <linux/module.h>		/* is a module */
#include "ixpci.h"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
#include <linux/cdev.h>
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0) && LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
        #include <linux/devfs_fs_kernel.h>
        #include <linux/init.h>
#endif

/* Deal with CONFIG_MODVERSIONS that is defined in
   /usr/include/linux/config.h (config.h is included by module.h) */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
/* Deal with CONFIG_MODVERSIONS that is defined in
   /usr/include/linux/config.h (config.h is included by module.h) */
        #if CONFIG_MODVERSIONS==1
        #define MODVERSIONS
        #include <linux/modversions.h>
        #endif
/* Additional */
// #include <linux/fs.h> /* file system here */
        #include <linux/wrapper.h>
#endif

/* using I/O ports */
#include <asm/io.h>
#include <linux/ioport.h>

/* need kmalloc */
#include <linux/slab.h>
#include <linux/proc_fs.h>

/* Local matter */

#define MODULE_NAME "ixpci"

#ifdef MODULE_LICENSE
MODULE_AUTHOR("Reed Lai <reed@icpdas.com>");
MODULE_DESCRIPTION("ICPDAS PCI-series driver, common Interface");
MODULE_LICENSE(ICPDAS_LICENSE);
#endif

// EXPORT_NO_SYMBOLS; /* do not export symbols */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
EXPORT_SYMBOL_GPL(ixpci_dev);
EXPORT_SYMBOL_GPL(ixpci_major);
//EXPORT_SYMBOL_GPL(ixpci_total_boards);
EXPORT_SYMBOL_GPL(ixpci_copy_devinfo);
#else
EXPORT_SYMBOL_NOVERS(ixpci_dev);
EXPORT_SYMBOL_NOVERS(ixpci_major);
//EXPORT_SYMBOL_GPL(ixpci_total_boards);
EXPORT_SYMBOL_NOVERS(ixpci_copy_devinfo);
#endif


/* functions are declared here first */
static int ixpci_open(struct inode *, struct file *);
int ixpci_release(struct inode *, struct file *);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
long ixpci_ioctl(struct file *, unsigned int , unsigned long );
#else
int ixpci_ioctl(struct inode *, struct file *, unsigned int , unsigned long );
#endif

/* variables are declared here first */
//int ixpci_total_boards = 0;
/* (export) dynamic major number */
int ixpci_major = DEVICE_MAJOR;
int ixpci_minor = DEVICE_MINOR;
/* pointer to the PCI cards' list */
ixpci_devinfo_t *ixpci_dev;

/* device file operaitons information */
static struct file_operations fops = {
        /* kernel 2.6 prevent the module from unloading while there is a open file(kernel 2.4 use the funciton MOD_INC_USE_COUNT and MOD_DEC_USE_COUNT to protect module from unloading when someone is opening file), so driver writer must set the file_operation's field owner a value 'THIS_MODULE' */
# if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
        owner:THIS_MODULE,
#endif
        open:ixpci_open,
        release:ixpci_release,

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
        unlocked_ioctl:ixpci_ioctl,
#else
        ioctl:ixpci_ioctl,
#endif
};

struct ixpci_carddef ixpci_card[] = {
	/* composed_id present module name */
	{PCI_1800, 0, "ixpci1800", "PCI-1800_1802_1602"},
	{PCI_1602_A, 0, "ixpci1602", "PCI-1602 (new id)"},
	{PCI_1202, 0, "ixpci1202", "PCI-1202"},
	{PCI_1002, 0, "ixpci1002", "PCI-1002_PEX-1002"},
	{PCI_P16R16, 0, "ixpcip16x16", "PCI-P16C16_P16R16_P16POR16_PEX-P16POR16"},
	{PCI_P8R8, 0, "ixpcip8r8", "PCI-P8R8"},
	{PCI_M512, 0, "ixpcim512", "PCI-M512"},
	{PCI_FC16, 0, "ixpcifc16", "PCI-FC16"},
	{PCI_TMC12, 0, "ixpcitmc12", "PCI-TMC12"},
        {PCI_LANNER, 0, "ixpcilanner", "PCI-LANNERIO"},
	{PCI_826, 0, "ixpci826", "PCI-826"},
	{PCI_822, 0, "ixpci822", "PCI-822"},
	{PCIe_8622, 0, "ixpcie8622", "PCIe-8622"},
	{PCIe_8620, 0, "ixpcie8620", "PCIe-8620"},
	{PCI_D96, 0, "ixpcid96", "PCI-D96_D128"},
	{PCI_D128, 0, "ixpcid96", "PCI-D96_D128"},
	{PCI_2602U, 0, "ixpci2602u", "PCI-2602U"},
	{0, 0, "", "UNKNOW"},
};

void *_cardname(__u64 id, int add_present)
{
	/* Get card name by id
	 * 
	 * Arguments:
	 *     id     card id (see ixpci.h)
	 *     new    flag for a new card just be found
	 * Returned:
	 *     Pointer to the name string that coresponds the given id.
	 *     If there is no card name has been found, return 0. */

	int i = 0;

	while (ixpci_card[i].id) {
		if (ixpci_card[i].id == id) {
			if (add_present)
				++(ixpci_card[i].present);
			/* yeh, present, check in */
			return ixpci_card[i].name;
		}
		++i;
	}
	return 0;
}

__inline__ void *_pci_cardname(__u64 id)
{
	return _cardname(id, 0);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,8,0)
static int ixpci_board_init (ixpci_devinfo_t *dev, int index)
#else
static int __devinit ixpci_board_init (ixpci_devinfo_t *dev, int index)
#endif
{
	int err;
	dev_t devno;
	devno = MKDEV(ixpci_major, ixpci_minor + index);
	dev->cdev = kmalloc(sizeof(struct cdev), GFP_KERNEL);
	memset(dev->cdev, 0, sizeof(struct cdev));
	cdev_init(dev->cdev, &fops);

	dev->cdev->owner = THIS_MODULE;
	dev->cdev->ops = &fops;

	err = cdev_add(dev->cdev, devno, 1);

	if (err)
	{
		printk("Error %d adding ixpci\n", err);
		return FAILURE;
	}

	return SUCCESS;
}
#endif

void _del_dev(void)
{
	/* Release memory from card list
	 *
	 * Arguments: none
	 *
	 * Returned:  void */

	ixpci_devinfo_t *dev, *prev;

	dev = ixpci_dev;
	while (dev) {
		prev = dev;
		dev = dev->next;
		kfree(prev);
	}
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
ixpci_devinfo_t *_add_dev(unsigned int no, unsigned int cno, __u64 id, char *name,
						  struct pci_dev *sdev)
#else
ixpci_devinfo_t *_add_dev(unsigned int no, __u64 id, char *name,
                                                  struct pci_dev *sdev)
#endif
{
	/* Add the found card to list
	 *
	 * Arguments:
	 *     no      device number in list
	 *     id      card id (see ixpci.h)
	 *     name    card name
	 *     sdev    pointer to the device information from system
	 *
	 * Returned:   The added device */

	int i;
	ixpci_devinfo_t *dev, *prev, *prev_f;

	if (ixpci_dev) {			/* ixpci device list is already followed */
		if (ixpci_dev->id == id)
			prev_f = ixpci_dev;
		else
			prev_f = 0;

		prev = ixpci_dev;
		dev = prev->next;

		while (dev) {			/* seek the tail of list */
			if (dev->id == id) {	/* last device in family */
				prev_f = dev;
			}
			prev = dev;
			dev = dev->next;
		}
		dev = prev->next = kmalloc(sizeof(ixpci_devinfo_t), GFP_KERNEL);
		//ixpci_memset(dev, 0, sizeof(ixpci_devinfo_t));
		memset(dev, 0, sizeof(ixpci_devinfo_t));
		dev->prev = prev;		/* member the previous address */
		if (prev_f) {
			dev->prev_f = prev_f;	/* member the previous family device */
			prev_f->next_f = dev;	/* member the next family device */
		}
	} else {					/* ixpci device list is empty, initiate */
		ixpci_dev = kmalloc(sizeof(ixpci_devinfo_t), GFP_KERNEL);
		//ixpci_memset(ixpci_dev, 0, sizeof(ixpci_devinfo_t));
		memset(ixpci_dev, 0, sizeof(ixpci_devinfo_t));
		dev = ixpci_dev;
	}
	dev->cno = cno;
	dev->no = no;
	dev->id = id;
	dev->irq = sdev->irq;

	switch(IXPCI_SUBDEVICE(id))
	{
	  case 0x8620:
		dev->AI_ch = 8;
		dev->AO_ch = 0;
		break;
	  case 0x8622:
		dev->AI_ch = 16;
		dev->AO_ch = 2;
		break;
	}

	for (i = 0; i < PBAN; i++) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
		dev->base[i] = pci_resource_start(sdev, i);

		if(pci_resource_flags(sdev, i) & IORESOURCE_MEM)
		{
			printk("BAR %d --> 0x%lx\n", i, dev->base[i]);
		}
		else
		{
			printk("no memory map\n");
		}

#else		/* LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0) */
		dev->base[i] = sdev->base_address[i] & PCI_BASE_ADDRESS_IO_MASK;
#endif
		dev->range[i] = pci_resource_len(sdev, i);
	}

	strncpy(dev->name, name, CNL);
	return dev;
}

int __init _find_dev(void)
{
	/* Find all devices (cards) in this system.
	 *
	 * Arguments: none
	 *
	 * Returned: The number of found devices */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
	
	unsigned int i = 0;
	unsigned int dev_no = 0, card_no = 0;
	unsigned int j;
	__u64 id;
	unsigned int vid, did, svid, sdid;
	char *name;
	struct pci_dev *sdev;
	ixpci_devinfo_t *dev;

	dev = NULL;
	sdev = NULL;

	for (; (id = ixpci_card[i].id) != 0; ++i)
	{
		vid = IXPCI_VENDOR(id);
		did = IXPCI_DEVICE(id);
		svid = IXPCI_SUBVENDOR(id);
		sdid = IXPCI_SUBDEVICE(id);
	
		#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
		while ((sdev = pci_find_subsys(vid, did, PCI_ANY_ID, PCI_ANY_ID, sdev)))
		#else
		while ((sdev = pci_get_subsys(vid, did, PCI_ANY_ID, PCI_ANY_ID, sdev)))
		#endif
		{
			if (svid && svid != sdev->subsystem_vendor) continue;
			if (sdid && sdid != sdev->subsystem_device) continue;

			//printk("sdev->subsystem_vendor is 0x%x sdev->subsystem_device is 0x%x\n",sdev->subsystem_vendor,sdev->subsystem_device);
		
			#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
			if (pci_enable_device(sdev))
			{
				printk("Error : enable pci fail\n");
				return FAILURE;
			}
			#endif
		 
			++(ixpci_card[i].present);
			name = ixpci_card[i].name;
			++dev_no;

			if ( card_no )
			{
				card_no++;
			}
			else
			{
				card_no = dev_no;
			}

			dev = _add_dev(dev_no, card_no, id, name, sdev);
	
			if (dev_no == 1)
			{
				KMSG("NO  PCI_ID____________  IRQ  BASE______  NAME...\n");
				/* "  01  0x1234567812345678  10   0x0000a400  PCI-1800\n" */
			}

			KMSG("%2d  0x%04x%04x%04x%04x  %3d  0x%08lx  %s\n", dev->cno, vid, did, svid, sdid, dev->irq, dev->base[0], name);
 		  
			for (j = 1; (j < PBAN) && (dev->base[j] != 0); ++j)
			{
				KMSG("                             0x%08lx\n", dev->base[j]);
			}
		  
			#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
			ixpci_board_init(dev, dev_no);
			#endif
		}
	}

	//set how many ixpci board in PC

	//ixpci_total_boards = card_no;
#else  /* LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0) */

	struct pci_dev *dev;
	unsigned int dev_no = 0, j;
	__u64 id, vid, did;
	__u16 svid, sdid;
	unsigned int a, b, c, d;
	char *name;

	dev = pci_devices;  /* system's pci device list */

	while (dev)
	{
	  vid = dev->vendor;
	  did = dev->device;
	  pci_read_config_word(dev, PCI_SUBSYSTEM_VENDOR_ID, &svid);
	  pci_read_config_word(dev, PCI_SUBSYSTEM_ID, &sdid);
	  id = (vid << 48) | (did << 32) | (svid << 16) | sdid;

	  /* Identify this device */
	  name = _cardname(id, 1);
	  if (name)
	  {
	    _add_dev(++dev_no, id, name, dev);

	    if (dev_no == 1)
	    {
	      KMSG("NO  PCI_ID____________  IRQ  BASE______  NAME...\n");
	      /* "  01  0x123456789abcdef0  10   0x0000a400  PCI-1800\n" */
	    }
	    
	    a = (id >> 48) & 0xffff;
	    b = (id >> 32) & 0xffff;
	    c = (id >> 16) & 0xffff;
	    d = id & 0xffff;
   	    KMSG("%2d  0x%04x%04x%04x%04x  %3d  0x%08lx  %s\n", dev_no, a, b, c, d, dev->irq,
	    dev->base_address[0] & PCI_BASE_ADDRESS_IO_MASK, name);
	    for (j = 1; (j < PBAN) && (dev->base_address[j] != 0); j++)
	    {
	      KMSG("                             0x%08lx\n", dev->base_address[j] & PCI_BASE_ADDRESS_IO_MASK);
	    }
	  }
	  
	  dev = dev->next;
	}

#endif

	return dev_no;
}

void ixpci_copy_devinfo(ixpci_devinfo_t * dst, ixpci_devinfo_t * src)
{
	int i;

	dst->no = src->no;
	dst->id = src->id;
	for (i = 0; i < PBAN; i++) {
		dst->base[i] = src->base[i];
	}
	strncpy(dst->name, src->name, CNL);
}

ixpci_devinfo_t *_find_minor(int minor)
{
	ixpci_devinfo_t *dp;
	for (dp = ixpci_dev; dp && dp->no != minor; dp = dp->next);
	return dp;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
long ixpci_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param)
#else
int ixpci_ioctl(struct inode *inode, struct file *file, unsigned int ioctl_num, unsigned long ioctl_param)
#endif
{
	/* This function is called whenever a process tries to do and IO
	 * control on IXPCI device file
	 *
	 * Arguments: read <linux/fs.h> for (*ioctl) of struct file_operations
	 *
	 * Returned:  error code */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
        return SUCCESS;
#else
	ixpci_devinfo_t *dp;

	dp = _find_minor(MINOR(inode->i_rdev));

	if (!dp || !dp->fops || !dp->fops->ioctl) return -EINVAL;

	return (dp->fops->ioctl) (inode, file, ioctl_num, ioctl_param);
#endif
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
int ixpci_release(struct inode *inode, struct file *file)
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
static int ixpci_release(struct inode *inode, struct file *file)
#else
static void ixpci_release(struct inode *inode, struct file *file)
#endif
{
	/* This function is called whenever a process attempts to closes the
	 * device file. It doesn't have a return value in kernel version 2.0.x
	 * because it can't fail (you must always be able to close a device).
	 * In version 2.2.x it is allowed to fail.
	 *
	 * Arguments: read <linux/fs.h> for (*release) of struct file_operations
	 *
	 * Returned:  version dependence */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
        return SUCCESS;
#else
	ixpci_devinfo_t *dp;

	dp = _find_minor(MINOR(inode->i_rdev));

	if (!dp || !dp->fops || !dp->fops->release)
	{
	  #if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
	  return -EINVAL;
	  #endif
	}
	else
	{
	  #if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
	  return (dp->fops->release) (inode, file);
	  #else
	  (dp->fops->release) (inode, file);
	  #endif
	}
#endif
}

static int ixpci_open(struct inode *inode, struct file *file)
{
	/* This function is called whenever a process attempts to open the
	 * device file
	 *
	 * Arguments: read <linux/fs.h> for (*open) of struct file_operations
	 *
	 * Returned:  error code */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
        return SUCCESS;
#else
	ixpci_devinfo_t *dp;

	dp = _find_minor(MINOR(inode->i_rdev));

	if (!dp || !dp->fops || !dp->fops->open) return -EINVAL;

	return (dp->fops->open) (inode, file);
#endif
}

void ixpci_cleanup(void)
{
	/* cleanup this module */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
        dev_t devno = MKDEV(ixpci_major, ixpci_minor);
#else
        int unr;
#endif
	/* remove /proc/ixpci */
	ixpci_proc_exit();

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
        unregister_chrdev_region(devno, DEVICE_NR_DEVS);
#else
	/* remove device file operations */
	unr = devfs_unregister_chrdev(ixpci_major, DEVICE_NAME);
	if (unr < 0) KMSG("%s devfs(module)_unregister_chrdev() error: %d\n", MODULE_NAME, unr);
#endif
	_del_dev();					/* release allocated memory */

	KMSG("%s has been removed\n", MODULE_NAME);
}

static int __init ixpci_init(void)
{
	/* initialize this module
	 *
	 * Arguments:
	 *
	 * Returned:  error code, 0 ok */

	unsigned found;
	char *comp;
	int err;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
	dev_t devt = 0;

        err = alloc_chrdev_region(&devt, DEVICE_MINOR, DEVICE_NR_DEVS, DEVICE_NAME);
        if (err < 0)
        {
          printk(KERN_WARNING "ixpci: can't get major %d\n", ixpci_major);
        }
        else
        {
          ixpci_major = MAJOR(devt);
        }
#endif

	KMSG("%s\n", MODULE_NAME);

	found = _find_dev();
	if (found == FAILURE) {
		KMSG("_find_dev() failed!\n");
		return FAILURE;
	}
	if (found == 0) {
		KMSG("No device (card) found.\n");
		return FAILURE;
	}
	if (found == 1)
		comp = "";
	else
		comp = "s";
	KMSG("Total %d device%s (card%s) found.\n", found, comp, comp);

	/* register device file operations  */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
        ixpci_major = devfs_register_chrdev(DEVICE_MAJOR, DEVICE_NAME, &fops);
#endif
	if (ixpci_major < 0) {
		KMSG("major %d failed!\n", ixpci_major);
		_del_dev();				/* release allocated memory */
		return ixpci_major;
	}
	KMSG("major %d.\n", ixpci_major);

	/* setup proc entry */
	if ((err = ixpci_proc_init())) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
                KMSG("/proc/%s/%s %d failed!\n", FAMILY, DEVICE_NAME, err);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
		KMSG("%s/%s/%s %d failed!\n", proc_root.name, FAMILY,
			 DEVICE_NAME, err);
#else
		KMSG("%s/%s %d failed!\n", proc_root.name, DEVICE_NAME, err);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
                unregister_chrdev_region(devt, DEVICE_NR_DEVS);
#else
                devfs_unregister_chrdev(ixpci_major, DEVICE_NAME);
#endif
		_del_dev();
		return err;
	}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
        KMSG("/proc/%s/%s %d.\n", FAMILY, DEVICE_NAME, err);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
	KMSG("%s/%s/%s %d.\n", proc_root.name, FAMILY, DEVICE_NAME, err);
#else
	KMSG("%s/%s %d.\n", proc_root.name, DEVICE_NAME, err);
#endif

	return SUCCESS;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
module_init(ixpci_init);
module_exit(ixpci_cleanup);
#endif  /* LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0) */
/* *INDENT-ON * */
