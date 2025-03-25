bin_PROGRAMS = dio dioa
dio_SOURCES = dio.c
dioa_SOURCES = dio_a.c
AM_CPPFLAGS = -I. -I../../include
if BITS
dioa_LDADD = ../../lib/libpio_64.a
else
dioa_LDADD = ../../lib/libpio.a
endif
# Read the root Makefile.am
# indent:
#	indent -kr -ts4 -nsc -pmt *.c

CLEANFILES = *~ \#*
