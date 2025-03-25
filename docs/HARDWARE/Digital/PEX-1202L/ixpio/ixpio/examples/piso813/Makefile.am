bin_PROGRAMS = ai aia aiso card_id card_id_a
ai_SOURCES = ai.c
aia_SOURCES = ai_a.c
aiso_SOURCES = ai_so.c
card_id_SOURCES = card_id.c
card_id_a_SOURCES = card_id_a.c
AM_CPPFLAGS = -I. -I../../include
if BITS
aia_LDADD = ../../lib/libpio_64.a
aiso_LDADD = ../../lib/libpio_64.so.1.0
card_id_LDADD = ../../lib/libpio_64.a
card_id_a_LDADD = ../../lib/libpio_64.a
else
aia_LDADD = ../../lib/libpio.a
aiso_LDADD = ../../lib/libpio.so.1.0
card_id_LDADD = ../../lib/libpio.a
card_id_a_LDADD = ../../lib/libpio.a
endif

# Read the root Makefile.am
# indent:
#	indent -kr -ts4 -nsc -pmt *.c

CLEANFILES = *~ \#*
