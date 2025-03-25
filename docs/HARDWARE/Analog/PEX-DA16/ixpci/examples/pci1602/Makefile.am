#bin_PROGRAMS = time_span dio reset 

bin_PROGRAMS = dio dioa daca adspolling 
dio_SOURCES = dio.c
dioa_SOURCES = dio_a.c
daca_SOURCES = dac_a.c
adspolling_SOURCES = adspolling_a.c
AM_CPPFLAGS = -I. -I../../include
AM_CFLAGS = -pthread
if BITS
dioa_LDADD = ../../lib/libpci_64.a -lm
daca_LDADD = ../../lib/libpci_64.a -lm
adspolling_LDADD = ../../lib/libpci_64.a -lm
else
dioa_LDADD = ../../lib/libpci.a -lm
daca_LDADD = ../../lib/libpci.a -lm
adspolling_LDADD = ../../lib/libpci.a -lm
endif


# Read the root Makefile.am
# indent:
#	indent -kr -ts4 -nsc -pmt *.c

CLEANFILES = *~ \#*
