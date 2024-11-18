Q               := @
CC              := gcc
SRCS            := $(wildcard *.c)
OBJS_32         := $(SRCS:.c=.o32)
OBJS_64         := $(SRCS:.c=.o64)
TARGET_32       := canopen_32
TARGET_64       := canopen_64
TARGET_32_D     := canopen_32_D
TARGET_64_D     := canopen_64_D
LIBS_32         := lib_canopen_32.a
LIBS_64         := lib_canopen_64.a
LIBS_32_D       := ./lib_canopen_32.so
LIBS_64_D       := ./lib_canopen_64.so
CFLAGS       	:= -std=c99 -Wall -I./include -fPIC -D_BSD_SOURCE
CFLAGS_32       := -m32
CFLAGS_64       := -m64
LDFLAGS         := -lpthread -lm
STATIC		 	:= 0

ifeq ($(STATIC), 1)
	CFLAGS_32 += -D_STATIC
	CFLAGS_64 += -D_STATIC
else
	TARGET_32  = $(TARGET_32_D)
	TARGET_64  = $(TARGET_64_D)
	LDFLAGS    = -ldl
endif

############################################
LBITS           := $(shell getconf LONG_BIT)

.PHONY: all both clean

ifeq ($(LBITS),64)
    all: $(TARGET_64)
else
    all: $(TARGET_32)
endif

both: $(TARGET_32) $(TARGET_64)
############################################


%.o32: %.c Makefile
	$(Q)echo "  Compiling '$<' ..."
	$(Q)$(CC) $(CFLAGS_32) $(CFLAGS) -o $@ -c $<

%.o64: %.c Makefile
	$(Q)echo "  Compiling '$<' ..."
	$(Q)$(CC) $(CFLAGS_64) $(CFLAGS) -o $@ -c $<

$(TARGET_32): $(OBJS_32)
	$(Q)echo "  $(COLOR_G)Building '$@' ..."
ifeq ($(STATIC), 1)
	$(Q)$(CC) -o $@ $(OBJS_32) $(LIBS_32) $(LDFLAGS)
else
	$(Q)$(CC) $(CFLAGS_32) -o $(TARGET_32) $(OBJS_32) $(LDFLAGS)
endif

$(TARGET_64): $(OBJS_64)
	$(Q)echo "  $(COLOR_G)Building '$@' ..."
ifeq ($(STATIC), 1)
	$(Q)$(CC) -o $@ $(OBJS_64) $(LIBS_64) $(LDFLAGS)
else
	$(Q)$(CC) $(CFLAGS_64) -o $(TARGET_64) $(OBJS_64) $(LDFLAGS)
endif

clean:
	$(Q)rm -f .depend *~ *.bak *.res *.o32 *.o64
	$(Q)echo "  Cleaning '$(TARGET_32)' ..."
	$(Q)rm -f $(TARGET_32)
	$(Q)echo "  Cleaning '$(TARGET_64)' ..."
	$(Q)rm -f $(TARGET_64)