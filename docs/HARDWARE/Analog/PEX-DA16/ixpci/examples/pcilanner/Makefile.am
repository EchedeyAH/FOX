bin_PROGRAMS = dio do_readback ai_voltage ai_current count dio_a count_a ai_voltage_a ai_current_a  eeprom_read_a precision_test_a ad_polling_a ad_scan_a

dio_SOURCES = dio.c
do_readback_SOURCES = do_readback.c
ai_voltage_SOURCES = ai_voltage.c
ai_current_SOURCES = ai_current.c
count_SOURCES = count.c
dio_a_SOURCES = dio_a.c
count_a_SOURCES = count_a.c
ai_voltage_a_SOURCES = ai_voltage_a.c
ai_current_a_SOURCES = ai_current_a.c
eeprom_read_a_SOURCES = eeprom_read_a.c
precision_test_a_SOURCES = precision_test_a.c
ad_polling_a_SOURCES = ad_polling_a.c
ad_scan_a_SOURCES = ad_scan_a.c
AM_CPPFLAGS = -I. -I../../include 
AM_CFLAGS = -lpthread -lm
if BITS
dio_a_LDADD = ../../lib/libpci_64.a
count_a_LDADD = ../../lib/libpci_64.a
ai_voltage_a_LDADD = ../../lib/libpci_64.a
ai_current_a_LDADD = ../../lib/libpci_64.a
eeprom_read_a_LDADD = ../../lib/libpci_64.a
precision_test_a_LDADD = ../../lib/libpci_64.a
ad_polling_a_LDADD = ../../lib/libpci_64.a
ad_scan_a_LDADD = ../../lib/libpci_64.a
else
dio_a_LDADD = ../../lib/libpci.a
count_a_LDADD = ../../lib/libpci.a
ai_voltage_a_LDADD = ../../lib/libpci.a
ai_current_a_LDADD = ../../lib/libpci.a
eeprom_read_a_LDADD = ../../lib/libpci.a
precision_test_a_LDADD = ../../lib/libpci.a
ad_polling_a_LDADD = ../../lib/libpci.a
ad_scan_a_LDADD = ../../lib/libpci.a
endif


# Read the root Makefile.am
# indent:
#	indent -kr -ts4 -nsc -pmt *.c

CLEANFILES = *~ \#*
