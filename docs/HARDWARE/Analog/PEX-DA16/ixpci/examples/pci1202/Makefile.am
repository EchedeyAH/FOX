bin_PROGRAMS = time_span dio reset ai_soft magicscan
time_span_SOURCES = time_span.c
dio_SOURCES = dio.c
reset_SOURCES = reset.c
ai_soft_SOURCES = ai_soft.c
magicscan_SOURCES = magicscan_a.c
AM_CFLAGS = -pthread
if BITS
magicscan_LDADD = ../../lib/libpci_64.a -lm
else
magicscan_LDADD = ../../lib/libpci.a -lm
endif

AM_CPPFLAGS = -I. -I../../include

# Read the root Makefile.am
# indent:
#	indent -kr -ts4 -nsc *.c

CLEANFILES = *~ \#*
