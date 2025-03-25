bin_PROGRAMS = ai_pacer ai_trigger ai_soft int time_span dio2 dio rst dioa ai_softa ai_pacera ai_triggera inta rsta
ai_pacer_SOURCES = ai_pacer.c
ai_trigger_SOURCES = ai_trigger.c
ai_soft_SOURCES = ai_soft.c
time_span_SOURCES = time_span.c
dio_SOURCES = dio.c
dio2_SOURCES = dio2.c
rst_SOURCES = rst.c
int_SOURCES = int.c
dioa_SOURCES = dio_a.c
ai_softa_SOURCES = ai_soft_a.c
ai_pacera_SOURCES = ai_pacer_a.c
ai_triggera_SOURCES = ai_trigger_a.c
inta_SOURCES = int_a.c
rsta_SOURCES = rst_a.c
AM_CPPFLAGS = -I. -I../../include 
if BITS
dioa_LDADD = ../../lib/libpci_64.a
ai_softa_LDADD = ../../lib/libpci_64.a
ai_pacera_LDADD = ../../lib/libpci_64.a
ai_triggera_LDADD = ../../lib/libpci_64.a
inta_LDADD = ../../lib/libpci_64.a
rsta_LDADD = ../../lib/libpci_64.a
else
dioa_LDADD = ../../lib/libpci.a
ai_softa_LDADD = ../../lib/libpci.a
ai_pacera_LDADD = ../../lib/libpci.a
ai_triggera_LDADD = ../../lib/libpci.a
inta_LDADD = ../../lib/libpci.a
rsta_LDADD = ../../lib/libpci.a
endif


# Read the root Makefile.am
# indent:
#	indent -kr -ts4 -nsc -pmt *.c

CLEANFILES = *~ \#*
