bin_PROGRAMS = ao ao2 ao_pat ao_pat_stop ao_pat_id dio dio2 dioa counter countera eeprow_read eeprow_reada output_voltagea output_currenta ao_performance card_id
ao_SOURCES = ao.c
ao2_SOURCES = ao2.c
ao_pat_SOURCES = ao_pat.c
ao_pat_stop_SOURCES = ao_pat_stop.c
ao_pat_id_SOURCES = ao_pat_id.c
dio_SOURCES = dio.c
dio2_SOURCES = dio2.c
counter_SOURCES = counter.c
dioa_SOURCES = dio_a.c
countera_SOURCES = counter_a.c
eeprow_read_SOURCES = eeprow_read.c
eeprow_reada_SOURCES = eeprow_read_a.c
output_voltagea_SOURCES = output_voltage_a.c
output_currenta_SOURCES = output_current_a.c
ao_performance_SOURCES = ao_performance.c
card_id_SOURCES = card_id.c
AM_CPPFLAGS = -I. -I../../include #-no-pie
if BITS
dioa_LDADD = ../../lib/libpio_64.a
countera_LDADD = ../../lib/libpio_64.a
eeprow_reada_LDADD = ../../lib/libpio_64.a
output_voltagea_LDADD = ../../lib/libpio_64.a
output_currenta_LDADD = ../../lib/libpio_64.a
else
dioa_LDADD = ../../lib/libpio.a
countera_LDADD = ../../lib/libpio.a
eeprow_reada_LDADD = ../../lib/libpio.a
output_voltagea_LDADD = ../../lib/libpio.a
output_currenta_LDADD = ../../lib/libpio.a
endif

# Read the root Makefile.am
# indent:
#	indent -kr -ts4 -nsc -pmt *.c

CLEANFILES = *~ \#*
