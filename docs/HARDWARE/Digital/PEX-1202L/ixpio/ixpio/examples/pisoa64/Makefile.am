bin_PROGRAMS = port porta
port_SOURCES = port.c
porta_SOURCES = port_a.c
AM_CPPFLAGS = -I. -I../../include
if BITS
porta_LDADD = ../../lib/libpio_64.a
else
porta_LDADD = ../../lib/libpio.a
endif
# Read the root Makefile.am
# indent:
#	indent -kr -ts4 -nsc -pmt *.c

CLEANFILES = *~ \#*
