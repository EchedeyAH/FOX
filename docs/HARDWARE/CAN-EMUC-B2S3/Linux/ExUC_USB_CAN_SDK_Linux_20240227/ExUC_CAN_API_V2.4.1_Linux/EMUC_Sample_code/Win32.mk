Q         := @
CC        := gcc
SRCS      := $(wildcard *.c)
OBJS      := $(SRCS:.c=.o)
TARGET    := emuc_b202_sample.exe
CFLAGS    := -D__USE_MINGW_ANSI_STDIO=1 -D_GNU_SOURCE -Wall -I./include
LDFLAGS   := $(LIBS)

.PHONY: all clean

all: .depend $(TARGET)

%.o: %.c Makefile
	$(Q)echo "  Compiling '$<' ..."
	$(Q)$(CC) $(CFLAGS) -o $@ -c $<

$(TARGET): $(OBJS)
	$(Q)echo -e "  Building '$@' VER=$(AP_VER)..."
	$(Q)$(CC) -o $@ $(OBJS) $(LDFLAGS)

clean: 
	$(Q)echo "  Cleaning '$(TARGET)' ..."
	$(Q)rm -f .depend *~ $(OBJS) $(TARGET) *.bak *.res

.depend:
	$(Q)echo "  Generating '$@' ..."
	$(Q)$(CC) $(CFLAGS) -M *.c > $@

ifeq (.depend, $(wildcard .depend))
	include .depend
endif