@Makefile_conf@

AR = ar
ARFLAGS = rv

CFLAGS = $(CDEBUG) -I. -I../../include 

INSTALL = install -c
INSTALLDATA = install -c -m 644

trgdir = ..

if BITS
trgs1 = $(trgdir)/libpci_64.a
trgs2 = $(trgdir)/libpcidaq_64.a
else
trgs1 = $(trgdir)/libpci.a
trgs2 = $(trgdir)/libpcidaq.a
endif

all:$(trgs1) $(trgs2)

$(trgs1): pcidio.c pci180x.c pcilanner.c pci1002.c
	$(CC) -c $(CFLAGS) -Dtarget_$(patsubst %.a,%,$(subst $(trgdir)/lib,,$@)) $^
	$(AR) $(ARFLAGS) $@ $(patsubst %.c,%.o,$^)
	$(RM) *.o

$(trgs2): pcidaq.c 
	$(CC) -c $(CFLAGS) -Dtarget_$(patsubst %.a,%,$(subst $(trgdir)/lib,,$@)) $^
	$(AR) $(ARFLAGS) $@ $(patsubst %.c,%.o,$^)
	$(RM) *.o

clean:
	rm -f *.o *~ *.tmp *.bak \#* .\#* $(trgs)

