bin_PROGRAMS = int dio2 dio rst led inta rcnta
dio_SOURCES = dio.c
dio2_SOURCES = dio2.c
rst_SOURCES = rst.c
int_SOURCES = int.c
inta_SOURCES = int_a.c
rcnta_SOURCES = rcnt_a.c
led_SOURCES = led.c

AM_CPPFLAGS = -I. -I../../include 

if BITS
inta_LDADD = ../../lib/libpcidaq_64.a
rcnta_LDADD = ../../lib/libpcidaq_64.a
else
inta_LDADD = ../../lib/libpcidaq.a
rcnta_LDADD = ../../lib/libpcidaq.a
endif

# Read the root Makefile.am
# indent:
#	indent -kr -ts4 -nsc -pmt *.c

CLEANFILES = *~ \#*
