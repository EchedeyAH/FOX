bin_PROGRAMS = demo  

demo_SOURCES = demo.c 
demo_LDFLAGS = -lm
AM_CPPFLAGS = -I. -I../../include
if BITS
demo_LDADD = ../../lib/libenc600_64.a
else
demo_LDADD = ../../lib/libenc600.a
endif

# Read the root Makefile.am
# indent:
#	indent -kr -ts4 -nsc -pmt *.c

CLEANFILES = *~ \#*
