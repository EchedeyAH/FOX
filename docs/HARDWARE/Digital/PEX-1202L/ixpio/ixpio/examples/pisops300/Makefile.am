bin_PROGRAMS = demo1 demo2 

demo1_SOURCES = demo1.c 
demo2_SOURCES = demo2.c
AM_CPPFLAGS = -I. -I../../include
if BITS
demo1_LDADD = ../../lib/libmstep3_64.a -lm
demo2_LDADD = ../../lib/libmstep3_64.a -lm
else
demo1_LDADD = ../../lib/libmstep3.a -lm
demo2_LDADD = ../../lib/libmstep3.a -lm
endif

# Read the root Makefile.am
# indent:
#	indent -kr -ts4 -nsc -pmt *.c

CLEANFILES = *~ \#*
