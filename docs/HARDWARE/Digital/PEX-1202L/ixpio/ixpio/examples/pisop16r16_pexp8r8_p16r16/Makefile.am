bin_PROGRAMS = port port2 di_polling porta card_id card_ida
port_SOURCES = port.c
port2_SOURCES = port2.c
porta_SOURCES = port_a.c
di_polling_SOURCES = di_polling.c
card_id_SOURCES = card_id.c
card_ida_SOURCES = card_id_a.c
AM_CPPFLAGS = -I. -I../../include 
if BITS
porta_LDADD = ../../lib/libpio_64.a
card_ida_LDADD = ../../lib/libpio_64.a
else
porta_LDADD = ../../lib/libpio.a
card_ida_LDADD = ../../lib/libpio.a
endif


# Read the root Makefile.am
# indent:
#	indent -kr -ts4 -nsc -pmt *.c

CLEANFILES = *~ \#*
