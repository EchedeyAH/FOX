bin_PROGRAMS = int port port2 porta inta
int_SOURCES = int.c
port_SOURCES = port.c
port2_SOURCES = port2.c
porta_SOURCES = port_a.c
inta_SOURCES = int_a.c
AM_CPPFLAGS = -I. -I../../include
if BITS
porta_LDADD = ../../lib/libpio_64.a
inta_LDADD = ../../lib/libpio_64.a
else
porta_LDADD = ../../lib/libpio.a
inta_LDADD = ../../lib/libpio.a
endif

# Read the root Makefile.am
# indent:
#	indent -kr -ts4 -nsc -pmt *.c

CLEANFILES = *~ \#*
