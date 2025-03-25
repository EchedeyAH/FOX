ifeq ($(KERNELRELEASE),)
KVER           := $(shell uname -r)
KDIR           := /lib/modules/$(KVER)/build
KMISC          := /lib/modules/$(KVER)/kernel/drivers/net/
KEXT           := $(shell echo $(KVER) | sed -ne 's/^2\.[567]\..*/k/p')o
KFLAG          := 2$(shell echo $(KVER) | sed -ne 's/^2\.[4]\..*/4/p')x
#CFLAGS  += -I$(LDDINCDIR) -std=gnu99

modules:
ifeq ($(KFLAG),24x)
	$(MAKE) -f Makefile_linux24x
else
	rm -f Module.symvers
	$(MAKE) -C $(KDIR) M=$(PWD) modules
endif

clean:
	rm -rf *.o *.ko *~ core* .dep* .*.d .*.cmd *.mod* *.a *.s .*.flags .tmp_versions

distclean:
	@if [ -e Makefile_linux24x ]; then      \
       	rm -f Makefile_linux24x;                \
       	rm -rf *.o *.ko *~ core* .dep* .*.d .*.cmd *.mod* *.a *.s .*.flags .tmp_versions;\
        else    \
       	rm -rf *.o *.ko *~ core* .dep* .*.d .*.cmd *.mod* *.a *.s .*.flags .tmp_versions;\
        fi
else		
#resolve warning message
EXTRA_CFLAGS += -I$(PWD)/../include -std=gnu99

# call from kernel build system
ixpci-objs := _pci.o _proc.o
ixpcitmc12-objs := _pcitmc12.o
ixpcip16x16-objs := _pcip16x16.o
ixpcip8r8-objs := _pcip8r8.o
ixpci1800-objs := _pci1800.o
ixpci1202-objs := _pci1202.o
ixpci1002-objs := _pci1002.o
ixpci1602-objs := _pci1602.o
ixpcim512-objs := _pcim512.o
ixpcifc16-objs := _pcifc16.o
ixpcilanner-objs := _pcilanner.o
ixpci826-objs := _pci826.o
ixpci822-objs := _pci822.o
ixpcie8622-objs := _pcie8622.o
ixpcid96-objs := _pcid96_d128.o
ixpci2602u-objs := _pci2602u.o

obj-m := ixpci.o ixpcitmc12.o ixpcip16x16.o ixpcip8r8.o ixpci1800.o ixpci1202.o ixpci1002.o ixpci1602.o ixpcim512.o ixpcifc16.o ixpcilanner.o ixpci826.o ixpcie8622.o ixpci822.o ixpcid96.o ixpci2602u.o
endif #ifeq ($(KERNELRELEASE),)
