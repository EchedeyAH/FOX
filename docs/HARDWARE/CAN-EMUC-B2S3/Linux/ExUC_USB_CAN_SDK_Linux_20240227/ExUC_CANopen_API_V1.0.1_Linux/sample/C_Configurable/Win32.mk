Q         := @
CC        := gcc
SRCS      := $(wildcard *.c)
OBJS      := $(SRCS:.c=.o)
TARGET    := canopen_sample.exe
# CFLAGS    := -D__USE_MINGW_ANSI_STDIO=1 -D_GNU_SOURCE -Wall -I./include
CFLAGS    := -Wall -I./include

.PHONY: all clean

all: $(TARGET)

%.o: %.c Makefile
	$(Q)echo "  Compiling '$<' ..."
	$(Q)$(CC) $(CFLAGS) -o $@ -c $<

$(TARGET): $(OBJS)
	$(Q)echo -e "  Building '$@' ..."
	$(Q)$(CC) -o $@ $(OBJS) $(LDFLAGS)

clean: 
	$(Q)echo "  Cleaning '$(TARGET)' ..."
	$(Q)rm -f .depend *~ $(OBJS) $(TARGET) *.bak *.res