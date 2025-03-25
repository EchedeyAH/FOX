#################################################################################
# File level history, record changes for this file here.			#
#										#	
# v 0.0.1 15 May 2007 by Golden Wang						#
#  Refresh Makefile structure to give support to Linux kernel 2.6.x completely  #
#										#
# v 0.0.0 28 Jan 2007 by Golden Wang						#
#  Give support to Linux kernel 2.6.x						#
#										#
#################################################################################		
ifeq ($(KERNELRELEASE),)
KVER           := $(shell uname -r)
KDIR           := /lib/modules/$(KVER)/build
KMISC          := /lib/modules/$(KVER)/kernel/drivers/net/
KEXT           := $(shell echo $(KVER) | sed -ne 's/^2\.[567]\..*/k/p')o
KFLAG          := 2$(shell echo $(KVER) | sed -ne 's/^2\.[4]\..*/4/p')x

modules:
ifeq ($(KFLAG),24x)
	$(MAKE) -f Makefile_linux24x
else
	rm -f Module.symvers
	$(MAKE) -C $(KDIR) M=$(PWD) LDDINCDIR=$(PWD)/../include modules
endif

clean:
	rm -rf *.o *.ko *~ core* .dep* .*.d .*.cmd *.mod* *.a *.s .*.flags .tmp_versions Module.symvers

distclean:
	@if [ -e Makefile_linux24x ]; then      \
	rm -f Makefile_linux24x;                \
	rm -rf *.o *.ko *~ core* .dep* .*.d .*.cmd *.mod.c *.a *.s .*.flags .tmp_versions;\
	else    \
	rm -rf *.o *.ko *~ core* .dep* .*.d .*.cmd *.mod.c *.a *.s .*.flags .tmp_versions;\
	fi
else
EXTRA_CFLAGS += -I$(PWD)/../include
# call from kernel build system
ixpio-objs := _pio.o _proc.o
ixpiod144-objs := _piod144.o _signal.o
ixpiod168-objs := _piod168.o _signal.o
ixpiod168a-objs := _piod168a.o _signal.o
ixpiod48_pexd48-objs := _piod48_pexd48.o _signal.o
ixpiod56-objs := _piod56.o _signal.o
ixpiod64-objs := _piod64.o _signal.o
ixpiod64a-objs := _piod64a.o _signal.o
ixpio821-objs := _pio821.o _signal.o
ixpiod96-objs := _piod96.o _signal.o
ixpioda16-objs := _pioda16.o _signal.o
ixpisop64-objs := _pisop64.o
ixpisoa64-objs := _pisoa64.o
ixpisoc64-objs := _pisoc64.o
ixpiso725-objs := _piso725.o _signal.o
ixpiso730-objs := _piso730.o _signal.o
ixpiso730a-objs := _piso730a.o _signal.o
ixpiso813-objs := _piso813.o
ixpisoda2-objs := _pisoda2.o _signal.o
ixpisop8r8-objs := _pisop8r8.o
ixpisop16r16u-objs := _pisop16r16u.o
ixpisop32a32-objs := _pisop32a32.o
ixpisop32c32-objs := _pisop32c32.o
ixpisoenc600-objs := _pisoenc600.o
ixpisops300-objs := _pisops300.o

obj-m := ixpio.o ixpiod144.o ixpiod168.o ixpiod168a.o ixpiod48_pexd48.o ixpiod56.o ixpiod64.o ixpiod64a.o ixpio821.o ixpiod96.o ixpioda16.o ixpisop64.o ixpisoa64.o ixpisoc64.o ixpiso725.o ixpiso730.o ixpiso730a.o ixpiso813.o ixpisoda2.o ixpisop8r8.o ixpisop16r16u.o ixpisop32a32.o ixpisop32c32.o ixpisoenc600.o ixpisops300.o


endif

