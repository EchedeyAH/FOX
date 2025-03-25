@Makefile_conf@

AR = ar
ARFLAGS = rv

# CDEBUG = -g
CDEBUG =
CFLAGS = $(CDEBUG) -I. -I../../include
CFLAGS_1 = $(CDEBUG) -I. -I../../include -O2 -fPIC
LDFLAGS = -shared
INSTALL = install -c
INSTALLDATA = install -c -m 644

trgdir = ..

if BITS
trgs1 = $(trgdir)/libmstep3_64.a
trgs2 = $(trgdir)/libenc600_64.a
trgs3 = $(trgdir)/libpio_64.a
trgs4 = $(trgdir)/libpio_64.so.1.0
dlib = libpio_64.so
else
trgs1 = $(trgdir)/libmstep3.a
trgs2 = $(trgdir)/libenc600.a
trgs3 = $(trgdir)/libpio.a
trgs4 = $(trgdir)/libpio.so.1.0
dlib = libpio.so
endif

all: $(trgs1) $(trgs2) $(trgs3) $(trgs4)

$(trgs1): mstep3.c msw.c
	$(CC) -c $(CFLAGS) -Dtarget_$(patsubst %.a,%,$(subst $(trgdir)/lib,,$@)) $^
	$(AR) $(ARFLAGS) $@ $(patsubst %.c,%.o,$^)
	$(RM) *.o

$(trgs2): enc600.c msw.c
	$(CC) -c $(CFLAGS) -Dtarget_$(patsubst %.a,%,$(subst $(trgdir)/lib,,$@)) $^
	$(AR) $(ARFLAGS) $@ $(patsubst %.c,%.o,$^)
	$(RM) *.o

$(trgs3): piodio.c pisop32a32_c32.c pisop64.c pisoa64_c64.c piso725.c pisop8r8.c piod56_24.c piod144_168.c piod96.c pioda16_8.c piso813.c piod48.c pisop16r16.c piso730.c
	$(CC) -c $(CFLAGS) -Dtarget_$(patsubst %.a,%,$(subst $(trgdir)/lib,,$@)) $^
	$(AR) $(ARFLAGS) $@ $(patsubst %.c,%.o,$^)
	$(RM) *.o

$(trgs4): piodio.c pisop32a32_c32.c pisop64.c pisoa64_c64.c piso725.c pisop8r8.c piod56_24.c piod144_168.c piod96.c pioda16_8.c piso813.c piod48.c pisop16r16.c piso730.c
	$(CC) -c $(CFLAGS_1) $^
	$(CC) $(LDFLAGS) -o $(dlib).1.0 *.o
	mv $(dlib).1.0 ../
	ln -sf $(dlib).1.0 $(trgdir)/$(dlib).1
	ln -sf $(dlib).1 $(trgdir)/$(dlib)
	$(RM) *.o

# Read the root Makefile.am
# indent:
#	indent -kr -ts4 -nsc -pmt *.c

clean:
	rm -f *.o *~ *.tmp *.bak \#* .\#* $(trgs)

#CLEANFILES = *~ \#*
