/* PIO series device driver.

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

   Use vi or gvim to edit this file and set tabstop=4.

   v 0.8.0  11 Nov 2014 by Winson Chen
     Give support to linux kernel 3.16.0.
     Fix warning by driver name defined.

   v 0.7.3  24 Jul 2007 by Golden Wang

     Give support PISO-730 v3.1.
     Give support PISO-P8R8 v3.1.

   v 0.7.2  23 Jul 2007 by Golden Wang

     Give support PISO-725 v3.1.

   v 0.7.1  24 May 2007 by Golden Wang
     Fixed a bug that doesn't check device id "0x0001"(IXPIO_DEVICE_ID2).
     The bug will result in some device that device id is "0x0001" install
     failure.

   v 0.7.0  28 Jan 2007 by Reed Lai
     Give support to linux kernel 2.6.x
	
   v 0.6.2  3 Jun 2003 by Reed Lai
     Fixed a bug about _find_minor() that caused the Segmentation fault
     when opening a fake device file.

   v 0.6.1 14 Jan 2003 by Reed Lai
     Refined and corrected some codes and comments.

   v 0.6.0 18 Nov 2002 by Reed Lai
     Comments out the exporting of ixpio_major.

   v 0.5.0  7 Nav 2002 by Reed Lai
     Releases IXPIO_SUB_AUX_ID_RANGE after reading sub aux id when searching
     on kernel 2.2.

     Complies to the kernel module license check.
     Uses slab.h in place of malloc.h.

   v 0.4.0 25 Jul 2002 by Reed Lai
     Gives support to Linux kernel 2.4.

   v 0.3.0 25 Jun 2002 by Reed Lai
     Gives support to PISO-730A.

   v 0.2.0  4 Jun 2002 by Reed Lai
     Gives support to PISO-725 and PISO-PS300.

   v 0.1.0 29 Apr 2002 by Reed Lai
     Renames PIO to IXPIO_.

   v 0.0.3 2000.10.04 by Reed Lai
     Marges pio_list.c with pio.c.

   v 0.0.2 2000.09.11 by Reed Lai

   v 0.0.1 2000.04.05 by Reed Lai
     base construction

   v 0.00 2000.03.15 by Reed Lai
     create, blah blah... */
/* *INDENT-OFF* */

#define IXPIO_RESERVED 0		/* 1 do compilation with reserved codes */

/* Mandatory */
#include <linux/kernel.h>		/* ta, kernel work */
#include <linux/module.h>		/* is a module */
#include "ixpio.h"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
#include <linux/cdev.h>
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0) && LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	#include <linux/devfs_fs_kernel.h>
	#include <linux/init.h>
#endif

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
	/* for compatibility with future version of Linux */

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,2,0)
#include <asm/uaccess.h>		/* function put_user() */
#endif

/* support /proc file */
#include <linux/proc_fs.h>

/* using I/O ports */
#include <asm/io.h>
#include <linux/ioport.h>

/* need kmalloc */
#include <linux/slab.h>

/* Local */

#define MODULE_NAME "ixpio"

#ifdef MODULE_LICENSE
MODULE_AUTHOR("Reed Lai <reed@icpdas.com>");
MODULE_DESCRIPTION("ICPDAS PIO-series driver, common interface");
MODULE_LICENSE(ICPDAS_LICENSE);
#endif							/* MODULE_LICENSE */

/* symbols exportation */
// EXPORT_NO_SYMBOLS; /* do not export symbols */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
EXPORT_SYMBOL_GPL(ixpio_dev);
EXPORT_SYMBOL_GPL(ixpio_copy_devinfo);
#else
EXPORT_SYMBOL_NOVERS(ixpio_dev);
EXPORT_SYMBOL_NOVERS(ixpio_copy_devinfo);
#endif

/* functions are declared here first */
static int ixpio_open(struct inode *, struct file *);
int ixpio_release(struct inode *, struct file *);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
long ixpio_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param);
#else
int ixpio_ioctl(struct inode *, struct file *, unsigned int , unsigned long );
#endif

/* variables are declared here first */
/* (export) dynamic major number */
int ixpio_major = DEVICE_MAJOR;
int ixpio_minor = DEVICE_MINOR;
/* pointer to the PIO cards' list */
ixpio_devinfo_t *ixpio_dev;

/* PIO cards' definition */
struct ixpio_carddef ixpio_card[] = {
	/* composed_id present module name */
	{PIO_D168A, 0, "ixpiod168a", "PIO-D168A"},
	{PIO_D168, 0, "ixpiod168", "PIO-D168"},
	{PIO_D168_T3A, 0, "ixpiod168", "PIO-D168"},
	{PIO_D168_T3B, 0, "ixpiod168", "PIO-D168"},
	{PIO_D144, 0, "ixpiod144", "PIO-D144"},
	{PIO_D144_T3A, 0, "ixpiod144", "PIO-D144"},
	{PIO_D144_T3B, 0, "ixpiod144", "PIO-D144"},
	{PIO_D96, 0, "ixpiod96", "PIO-D96"},
	{PIO_D96_T3A, 0, "ixpiod96", "PIO-D96"},
	{PIO_D96_T3B, 0, "ixpiod96", "PIO-D96"},
	{PIO_D64A, 0, "ixpiod64a", "PIO-D64"},
	{PIO_D64, 0, "ixpiod64", "PIO-D64"},
	{PIO_D64_T3A, 0, "ixpiod64", "PIO-D64"},
	{PIO_D64_T3B, 0, "ixpiod64", "PIO-D64"},
	{PIO_D56, 0, "ixpiod56", "PIO-D56_D24"},
	{PIO_D56_T3A, 0, "ixpiod56", "PIO-D56_D24"},
	{PIO_D56_T3B, 0, "ixpiod56", "PIO-D56_D24"},
	{PIO_D48, 0, "ixpiod48_pexd48", "PIO-D48_PEX-D48"},
	{PIO_D48_T3A, 0, "ixpiod48_pexd48", "PIO-D48_PEX-D48"},
	{PIO_D48_T3B, 0, "ixpiod48_pexd48", "PIO-D48_PEX-D48"},
	{PIO_D24, 0, "ixpiod24", "PIO-D24"},
	{PIO_D24_T3A, 0, "ixpiod24", "PIO-D24"},
	{PIO_D24_T3B, 0, "ixpiod24", "PIO-D24"},
	{PIO_823, 0, "ixpio823", "PIO-823"},
	{PIO_821, 0, "ixpio821", "PIO-821"},
	{PIO_821_T3A, 0, "ixpio821", "PIO-821"},
	{PIO_DA16, 0, "ixpioda16", "PIO-DA16_DA8_DA4"},
	{PIO_DA16_T3A, 0, "ixpioda16", "PIO-DA16_DA8_DA4"},
	{PIO_DA16_T3B, 0, "ixpioda16", "PIO-DA16_DA8_DA4"},
	{PIO_DA8, 0, "ixpioda8", "PIO-DA8"},
	{PIO_DA8_T3A, 0, "ixpioda8", "PIO-DA8"},
	{PIO_DA8_T3B, 0, "ixpioda8", "PIO-DA8"},
	{PIO_DA4, 0, "ixpioda4", "PIO-DA4"},
	{PIO_DA4_T3A, 0, "ixpioda4", "PIO-DA4"},
	{PIO_DA4_T3B, 0, "ixpioda4", "PIO-DA4"},
	{PISO_C64, 0, "ixpisoc64", "PISO-C64"},
	{PISO_C64_T3A, 0, "ixpisoc64", "PISO-C64"},
	{PISO_C64_T3B, 0, "ixpisoc64", "PISO-C64"},
	{PISO_P64, 0, "ixpisop64", "PISO-P64"},
	{PISO_P64_T3A, 0, "ixpisop64", "PISO-P64"},
	{PISO_P64_T3B, 0, "ixpisop64", "PISO-P64"},
	{PISO_A64, 0, "ixpisoa64", "PISO-A64"},
	{PISO_A64_T3A, 0, "ixpisoa64", "PISO-A64"},
	{PISO_A64_T3B, 0, "ixpisoa64", "PISO-A64"},
	{PISO_P32C32, 0, "ixpisop32c32", "PISO-P32C32"},
	{PISO_P32C32_T3A, 0, "ixpisop32c32", "PISO-P32C32"},
	{PISO_P32C32_T3B, 0, "ixpisop32c32", "PISO-P32C32"},
	{PISO_P32A32, 0, "ixpisop32a32", "PISO-P32A32"},
	{PISO_P32A32_T3A, 0, "ixpisop32a32", "PISO-P32A32"},
	{PISO_P32A32_T3B, 0, "ixpisop32a32", "PISO-P32A32"},
	{PISO_P8R8, 0, "ixpisop8r8", "PISO-P8R8_P8SSR8AC_P8SSR8DC"},
	{PISO_P8R8_T3A, 0, "ixpisop8r8", "PISO-P8R8_P8SSR8AC_P8SSR8DC"},
	{PISO_P8R8_T3B, 0, "ixpisop8r8", "PISO-P8R8_P8SSR8AC_P8SSR8DC"},
	{PISO_P8R8_T3C, 0, "ixpisop8r8", "PISO-P8R8_P8SSR8AC_P8SSR8DC"},
	{PISO_P8SSR8AC, 0, "ixpisop8ssr8ac", "PISO-P8SSR8AC"},
	{PISO_P8SSR8AC_T3A, 0, "ixpisop8ssr8ac", "PISO-P8SSR8AC"},
	{PISO_P8SSR8AC_T3B, 0, "ixpisop8ssr8ac", "PISO-P8SSR8AC"},
	{PISO_P8SSR8DC, 0, "ixpisop8ssr8dc", "PISO-P8SSR8DC"},
	{PISO_P8SSR8DC_T3A, 0, "ixpisop8ssr8dc", "PISO-P8SSR8DC"},
	{PISO_P8SSR8DC_T3B, 0, "ixpisop8ssr8dc", "PISO-P8SSR8DC"},
	{PISO_730, 0, "ixpiso730", "PISO-730"},
	{PISO_730_T3A, 0, "ixpiso730", "PISO-730"},
	{PISO_730_T3B, 0, "ixpiso730", "PISO-730"},
	{PISO_730_T3C, 0, "ixpiso730", "PISO-730"},
	{PISO_730A, 0, "ixpiso730a", "PISO-730A"},
	{PISO_730A_T3A, 0, "ixpiso730a", "PISO-730A"},
	{PISO_730A_T3B, 0, "ixpiso730a", "PISO-730A"},
	{PISO_813, 0, "ixpiso813", "PISO-813"},
	{PISO_813_T3A, 0, "ixpiso813", "PISO-813"},
	{PISO_813_T3B, 0, "ixpiso813", "PISO-813"},
	{PISO_DA2, 0, "ixpisoda2", "PISO-DA2"},
	{PISO_DA2_T3A, 0, "ixpisoda2", "PISO-DA2"},
	{PISO_DA2_T3B, 0, "ixpisoda2", "PISO-DA2"},
	{PISO_725, 0, "ixpiso725", "PISO-725"},
	{PISO_725_T3A, 0, "ixpiso725", "PISO-725"},
	{PISO_725_T3B, 0, "ixpiso725", "PISO-725"},
	{PISO_725_T3C, 0, "ixpiso725", "PISO-725"},
	{PISO_PS300, 0, "ixpisops300", "PISO-PS300"},
	{PISO_PS300_T3A, 0, "ixpisops300", "PISO-PS300"},
	{PISO_PS300_T3B, 0, "ixpisops300", "PISO-PS300"},
	{PISO_ENC600, 0, "ixpisoenc600", "PISO-ENC300_600"},
	{PISO_P16R16U, 0, "ixpisop16r16u", "PISO-P16R16_PEX-P8R8_P16R16"},
	{0, 0, "", "UNKNOW"},
};

/* device file operaitons information */

static struct file_operations fops = {
	/* kernel 2.6 prevent the module from unloading while there is a open file(kernel 2.4 use the funciton MOD_INC_USE_COUNT and MOD_DEC_USE_COUNT to protect module from unloading when someone is opening file), so driver writer must set the file_operation's field owner a value 'THIS_MODULE' */
# if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
        owner:THIS_MODULE,
#endif
        open:ixpio_open,
	release:ixpio_release,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
	unlocked_ioctl:ixpio_ioctl
#else
        ioctl:ixpio_ioctl
#endif
};

void *_cardname(unsigned id, int add_present)
{
	/* (export)
	 *
	 * get card name by sub-ids
	 *
	 * Arguments:
	 *   id       composed sub-ids, see ixpio.h
	 *
	 * Returned:  pointer to the name string that coresponds the given sub
	 *            id, if there is no card name found, returns 0 */

	int i = 0;

	while (ixpio_card[i].id) {
		if (ixpio_card[i].id == id) {
			if (add_present)
				++(ixpio_card[i].present);
			return ixpio_card[i].name;
		}
		++i;
	}
	return 0;
}

__inline__ void *_pio_cardname(int id)
{
	return _cardname(id, 0);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,8,0)
static int ixpio_board_init (ixpio_devinfo_t *dev, int index)
#else
static int __devinit ixpio_board_init (ixpio_devinfo_t *dev, int index)
#endif
{
  int err, devno;
  devno = MKDEV(ixpio_major, ixpio_minor + index);
  dev->cdev = kmalloc(sizeof(struct cdev), GFP_KERNEL);
  memset(dev->cdev, 0, sizeof(struct cdev));
  cdev_init(dev->cdev, &fops);

  dev->cdev->owner = THIS_MODULE;
  dev->cdev->ops = &fops;

  err = cdev_add(dev->cdev, devno, 1);
 
  if (err)
  {
    printk("Error %d adding ixpio\n", err);
    return FAILURE;
  }

  return SUCCESS;
}
#endif

void _del_dev(void)
{
	/* release memory of card list
	 *
	 * Arguments: none
	 *
	 * Returned:  void */

	ixpio_devinfo_t *dev, *prev;

	dev = ixpio_dev;
	while (dev) {
		prev = dev;
		dev = dev->next;
		kfree(prev);
	}
}

ixpio_devinfo_t *_add_dev(unsigned int no, unsigned int csid, char *name,
						  struct pci_dev *sdev)
{
	/* Add the found card to list
	 *
	 * Arguments:
	 *   no       device number in list
	 *   csid     composed sub-ids
	 *   name     card name
	 *   sdev     pointer to the device information from system
	 *
	 * Returned:  The added device */

	ixpio_devinfo_t *dev, *prev, *prev_f;

	if (ixpio_dev) {
		/* ixpio device list is already followed */
		if (ixpio_dev->csid == csid)
			prev_f = ixpio_dev;
		else
			prev_f = 0;
		prev = ixpio_dev;
		dev = prev->next;
		while (dev) {
			/* seek the tail of list */
			if (dev->csid == csid) {
				/* last device in family */
				prev_f = dev;
			}
			prev = dev;
			dev = dev->next;
		}
		dev = prev->next = kmalloc(sizeof(ixpio_devinfo_t), GFP_KERNEL);
		memset(dev, 0, sizeof(ixpio_devinfo_t));
		dev->prev = prev;
		/* member the previous address */
		if (prev_f) {
			dev->prev_f = prev_f;
			/* member the previous family device */
			prev_f->next_f = dev;
			/* member the next family device */
		}
	} else {
		/* ixpio device list is empty, initiate */
		ixpio_dev = kmalloc(sizeof(ixpio_devinfo_t), GFP_KERNEL);
		memset(ixpio_dev, 0, sizeof(ixpio_devinfo_t));
		dev = ixpio_dev;
	}
	dev->no = no;
	dev->csid = csid;
	dev->irq = sdev->irq;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
	dev->base = pci_resource_start(sdev, 0);
#else
	dev->base = sdev->base_address[0] & PCI_BASE_ADDRESS_IO_MASK;
#endif
	strncpy(dev->name, name, CNL);
	return dev;
}

static int _find_dev(void)
{
	/* find all ixpio devices (cards) in this system
	 *
	 * Arguments: none
	 *
	 * Returned:  number of found ixpio devices (cards) */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)

	//int err;
	unsigned int dev_no = 0;
	unsigned int csid;
	char *name;
	struct pci_dev *sdev;
	ixpio_devinfo_t *dev;
	unsigned long base;
	unsigned char said;			/* sub-aux-id */

	dev = NULL;

	#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	sdev = pci_find_subsys(IXPIO_VENDOR_ID, IXPIO_DEVICE_ID, PCI_ANY_ID,PCI_ANY_ID, NULL);
	#else
	sdev = pci_get_subsys(IXPIO_VENDOR_ID, IXPIO_DEVICE_ID, PCI_ANY_ID,PCI_ANY_ID, NULL);
	#endif

	while (sdev)
	{
          #if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
	  //err = pci_enable_device(sdev);

	  if (pci_enable_device(sdev))
	  {
	    printk("Error : enable pci fail\n");
	    return FAILURE;
	  }
	  #endif

   	  /* get sub-aux-id, yeh, a little annonying */
	  base = pci_resource_start(sdev, 0);
	  if (pci_request_regions(sdev, "ixpio_find_dev"))
          {
	    KMSG("IxPIO Aux-ID failed!\n");
	    return FAILURE;
	  }
	
	  said = IXPIO_SUB_AUX_ID_MASK & inb((unsigned) (base + IXPIO_SUB_AUX_ID_OFFSET));
  	  if((sdev->subsystem_device & 0xff ) == 0x0c) said = 0;
	  pci_release_regions(sdev);

	  /* makeup composed sub-ids */
	  csid = IXPIO_CSID(sdev->subsystem_vendor, sdev->subsystem_device, said);

          /* if got cardname, set presentation */
	  name = _cardname(csid, ADD_PRESENT);
	  if (name)
	  {
	    dev = _add_dev(++dev_no, csid, name, sdev);
	    if (dev_no == 1)
	    {
	      KMSG("NO  CSID____  IRQ   BASE__  NAME...\n");
              /*   "01  0x800010  10    0xa400  IXPIO_D144\n" */
	    }
	    
	    KMSG("%2d  0x%6x  %3d   0x%lx  %s\n", dev->no, dev->csid, dev->irq, dev->base, dev->name);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
	    ixpio_board_init(dev, dev_no);
#endif
	  }

	  /* scan for next one */
	  #if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	  sdev = pci_find_subsys(IXPIO_VENDOR_ID, IXPIO_DEVICE_ID, PCI_ANY_ID,PCI_ANY_ID, sdev);
          #else
          sdev = pci_get_subsys(IXPIO_VENDOR_ID, IXPIO_DEVICE_ID, PCI_ANY_ID,PCI_ANY_ID, sdev);
          #endif
	}

	#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
        sdev = pci_find_subsys(IXPIO_VENDOR_ID, IXPIO_DEVICE_ID2, PCI_ANY_ID,PCI_ANY_ID, NULL);
        #else
        sdev = pci_get_subsys(IXPIO_VENDOR_ID, IXPIO_DEVICE_ID2, PCI_ANY_ID,PCI_ANY_ID, NULL);
        #endif

	while (sdev)
        {
          #if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
          //err = pci_enable_device(sdev);

          if (pci_enable_device(sdev))
          {
            printk("Error : enable pci fail\n");
            return FAILURE;
          }
          #endif

          /* get sub-aux-id, yeh, a little annonying */
          base = pci_resource_start(sdev, 0);
          if (pci_request_regions(sdev, "ixpio_find_dev"))
          {
            KMSG("IxPIO Aux-ID failed!\n");
            return FAILURE;
          }

          said = IXPIO_SUB_AUX_ID_MASK & inb((unsigned) (base + IXPIO_SUB_AUX_ID_OFFSET));
          if((sdev->subsystem_device & 0xff ) == 0x0c) said = 0;
          pci_release_regions(sdev);

          /* makeup composed sub-ids */
          csid = IXPIO_CSID(sdev->subsystem_vendor, sdev->subsystem_device, said);

	  /* mask Sub-AUX-ID */

	  if ( (csid & 0xffffff00) == PISO_725_T3C )
          {
	    csid = csid & 0xffffff00;
          }
          else if ( (csid & 0xffffff00) == PISO_730_T3C )
          {
            csid = csid & 0xffffff00;
          }
          else if ( (csid & 0xffffff00) == PISO_P8R8_T3C )
          {
            csid = csid & 0xffffff00;
          }

          /* if got cardname, set presentation */

          name = _cardname(csid, ADD_PRESENT);

	  if (name)
          {
            dev = _add_dev(++dev_no, csid, name, sdev);
            if (dev_no == 1)
            {
              KMSG("NO  CSID____  IRQ   BASE__  NAME...\n");
              /*   "01  0x800010  10    0xa400  IXPIO_D144\n" */
            }

            KMSG("%2d  0x%6x  %3d   0x%lx  %s\n", dev->no, dev->csid, dev->irq, dev->base, dev->name);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
            ixpio_board_init(dev, dev_no);
#endif
          }

          /* scan for next one */
          #if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
          sdev = pci_find_subsys(IXPIO_VENDOR_ID, IXPIO_DEVICE_ID2, PCI_ANY_ID,PCI_ANY_ID, sdev);
          #else
          sdev = pci_get_subsys(IXPIO_VENDOR_ID, IXPIO_DEVICE_ID2, PCI_ANY_ID,PCI_ANY_ID, sdev);
          #endif
        }

#else /* LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0) */

	struct pci_dev *sdev;
	ixpio_devinfo_t *dev;
	unsigned char svid;			/* sub-vendor-id */
	unsigned char sdid;			/* sub-device-id */
	unsigned char said;			/* sub-aux-id */
	unsigned long base;
	unsigned int csid;
	char *name;
	unsigned int dev_no = 0;

	sdev = pci_devices;
	/* system's pci devices' list */

	while (sdev)
	{
	  if ((sdev->vendor == IXPIO_VENDOR_ID)	&& ((sdev->device == IXPIO_DEVICE_ID) || (sdev->device == IXPIO_DEVICE_ID2)))
	  {
            /* sub-vendor-id,  sub-device-id */
	    if (pci_read_config_byte(sdev, PCI_SUBSYSTEM_VENDOR_ID, &svid) || pci_read_config_byte(sdev, PCI_SUBSYSTEM_ID, &sdid))
	    {
	      KMSG("Subsystem IDs failed!\n");
	      return FAILURE;
	    }

	    /* sub-aux-id, yeh, a little annonying */
	    base = sdev->base_address[0] & PCI_BASE_ADDRESS_IO_MASK;

	    if (check_region(base, IXPIO_SUB_AUX_ID_RANGE))
	    {
	      KMSG("IxPIO Aux-ID failed!\n");
	      return FAILURE;
	    }
	    
	    request_region(base, IXPIO_SUB_AUX_ID_RANGE, "ixpio_find_dev");
	    said = IXPIO_SUB_AUX_ID_MASK & inb((unsigned) (base+ IXPIO_SUB_AUX_ID_OFFSET));
            release_region(base, IXPIO_SUB_AUX_ID_RANGE);

#if 0
			base_iomap = ioremap(base, IXPIO_SUB_AUX_ID_RANGE);
			sub_aux =
				readb(base_iomap + IXPIO_SUB_AUX_ID_OFFSET) &
				IXPIO_SUB_AUX_ID_MASK;
			KMSG("0x%08lx mapped to % p, sub - aux = 0x % x \ n ", dev->base_address[0], base_iomap, sub_aux);
			iounmap(base_iomap);
#endif
	    /* makeup composed sub-ids */
	    csid = IXPIO_CSID(svid, sdid, said);
	    /* if got cardname, set presentation */
	    name = _cardname(csid, ADD_PRESENT);
	    if (name)
	    {
	      dev = _add_dev(++dev_no, csid, name, sdev);
	      if (dev_no == 1)
	      {
		KMSG("NO  CSID____  IRQ   BASE__  NAME...\n");
		/*   "01  0x800010  10    0xa400  IXPIO_D144\n" */
	      }
	      
	      KMSG("%2d  0x%6x  %3d   0x%lx  %s\n", dev->no, dev->csid, dev->irq, dev->base, dev->name);
 	    }
	  }
	  sdev = sdev->next;
	}

#endif							/* LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0) ELSE */

	return dev_no;
}

void ixpio_copy_devinfo(ixpio_devinfo_t * dst, ixpio_devinfo_t * src)
{
	/* (export) */
	dst->no = src->no;
	dst->csid = src->csid;
	dst->irq = src->irq;
	dst->base = src->base;
	dst->hid = src->hid;
	strncpy(dst->name, src->name, CNL);
}

ixpio_devinfo_t *_find_minor(int minor)
{
	ixpio_devinfo_t *dp;
	for (dp = ixpio_dev; dp && dp->no != minor; dp = dp->next);
	return dp;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
long ixpio_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param)
#else
int ixpio_ioctl(struct inode *inode, struct file *file,	unsigned int ioctl_num, unsigned long ioctl_param)
#endif
{
	/* This function is called whenever a process tries to do and IO
	 * control on PIO device file
	 *
	 * Arguments: read <linux/fs.h> for (*ioctl) of struct file_operations
	 *
	 * Returned:  error code */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
	return SUCCESS;
#else
	ixpio_devinfo_t *dp;
	dp = _find_minor(MINOR(inode->i_rdev));

	if (!dp || !dp->fops || !dp->fops->ioctl) return -EINVAL;

	return (dp->fops->ioctl) (inode, file, ioctl_num, ioctl_param);
#endif
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
int ixpio_release(struct inode *inode, struct file *file)
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
static int ixpio_release(struct inode *inode, struct file *file)
#else
static void ixpio_release(struct inode *inode, struct file *file)
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
	ixpio_devinfo_t *dp;
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

static int ixpio_open(struct inode *inode, struct file *file)
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
	ixpio_devinfo_t *dp;
	dp = _find_minor(MINOR(inode->i_rdev));
	if (!dp || !dp->fops || !dp->fops->open)
		return -EINVAL;

	KMSG("insmod ixpio");
	return (dp->fops->open) (inode, file);
#endif
}

void ixpio_cleanup(void)
{

	/* cleanup this module */


#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
	dev_t devno = MKDEV(ixpio_major, ixpio_minor);
#else
	int unr;
#endif
	/* remove proc file */
	ixpio_proc_exit();
	/* remove device file operations */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
	unregister_chrdev_region(devno, DEVICE_NR_DEVS);
#else	
	unr = devfs_unregister_chrdev(ixpio_major, DEVICE_NAME);
	if (unr < 0) KMSG("%s devfs_unregister_chrdev() error: %d\n", MODULE_NAME, unr);
#endif
	_del_dev();
	/* release allocated memory */
	KMSG("%s has been removed\n", MODULE_NAME);
}

static int __init ixpio_init(void)
{
	/* initialize this module
	 *
	 * Arguments:
	 *
	 * Returned:
	 *   integer 0 means ok, otherwise failed (module can't be loaded) */
	unsigned found;
	char *comp;
	int err;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
	dev_t devt = 0;

        err = alloc_chrdev_region(&devt, DEVICE_MINOR, DEVICE_NR_DEVS, DEVICE_NAME);
        if (err < 0)
        {
          printk(KERN_WARNING "ixpio: can't get major %d\n", ixpio_major);
        }
        else
        {
          ixpio_major = MAJOR(devt);
        }
#endif
	KMSG("%s\n", MODULE_NAME);
	found = _find_dev();
	/* search ixpio devices (cards) in this system */
	if (found == FAILURE) {
		KMSG("_find_dev() failed!\n");
		return FAILURE;
	}
	if (found == 0) {
		KMSG("No device (card) found.\n");
		return FAILURE;
	}
	if (found == 1) {
		comp = "";
	} else {
		comp = "s";
	}
	KMSG("Total %d device%s (card%s) found.\n", found, comp, comp);

	/* register device file operations  */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	ixpio_major = devfs_register_chrdev(DEVICE_MAJOR, DEVICE_NAME, &fops);
#endif

	if (ixpio_major <= 0) {
		KMSG("major %d failed!\n", ixpio_major);
		_del_dev();
		return ixpio_major;
	}
	KMSG("major %d.\n", ixpio_major);
	/* setup proc file */
	if ((err = ixpio_proc_init())) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
		KMSG("/proc/%s/%s %d failed!\n", ORGANIZATION, DEVICE_NAME, err);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
		KMSG("%s/%s/%s %d failed!\n", proc_root.name,
			 ORGANIZATION, DEVICE_NAME, err);
#else
		KMSG("%s/%s %d failed!\n", proc_root.name, DEVICE_NAME, err);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
		unregister_chrdev_region(devt, DEVICE_NR_DEVS);
#else
		devfs_unregister_chrdev(ixpio_major, DEVICE_NAME);
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

#if LINUX_VERSION_CODE >=KERNEL_VERSION(2,4,0)
module_init(ixpio_init);
module_exit(ixpio_cleanup);
#endif
/* *INDENT-ON* */
