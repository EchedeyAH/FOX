#bin_PROGRAMS = time_span dio reset 

bin_PROGRAMS = time_span dio reset dioa daca adspolling adspacer magicscan
time_span_SOURCES = time_span.c
dio_SOURCES = dio.c
reset_SOURCES = reset.c
dioa_SOURCES = dio_a.c
daca_SOURCES = dac_a.c
adspolling_SOURCES = adspolling_a.c
adspacer_SOURCES = adspacer_a.c
magicscan_SOURCES = magicscan_a.c
AM_CPPFLAGS = -I. -I../../include
AM_CFLAGS = -pthread
if BITS
dioa_LDADD = ../../lib/libpci_64.a -lm
daca_LDADD = ../../lib/libpci_64.a -lm
adspolling_LDADD = ../../lib/libpci_64.a -lm
adspacer_LDADD = ../../lib/libpci_64.a -lm
magicscan_LDADD = ../../lib/libpci_64.a -lm
else
dioa_LDADD = ../../lib/libpci.a -lm
daca_LDADD = ../../lib/libpci.a -lm
adspolling_LDADD = ../../lib/libpci.a -lm
adspacer_LDADD = ../../lib/libpci.a -lm
magicscan_LDADD = ../../lib/libpci.a -lm
endif




# Read the root Makefile.am
# indent:
#	indent -kr -ts4 -nsc -pmt *.c

CLEANFILES = *~ \#*
