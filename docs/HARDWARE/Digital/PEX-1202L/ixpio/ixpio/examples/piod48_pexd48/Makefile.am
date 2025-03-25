bin_PROGRAMS = counter int port fc dio porta inta countera card_id card_ida
counter_SOURCES = counter.c
countera_SOURCES = counter_a.c
int_SOURCES = int.c
port_SOURCES = port.c
fc_SOURCES = fc.c
dio_SOURCES = dio.c
card_id_SOURCES = card_id.c
card_ida_SOURCES = card_id_a.c
porta_SOURCES = port_a.c
inta_SOURCES = int_a.c
AM_CPPFLAGS = -I. -I../../include
if BITS
countera_LDADD = ../../lib/libpio_64.a
porta_LDADD = ../../lib/libpio_64.a
inta_LDADD = ../../lib/libpio_64.a
card_ida_LDADD = ../../lib/libpio_64.a
else
countera_LDADD = ../../lib/libpio.a
porta_LDADD = ../../lib/libpio.a
inta_LDADD = ../../lib/libpio.a
card_ida_LDADD = ../../lib/libpio.a
endif

# Read the root Makefile.am
# indent:
#	indent -kr -ts4 -nsc -pmt *.c

CLEANFILES = *~ \#*
