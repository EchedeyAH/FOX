Q               := @
CC              := gcc -std=gnu99
SRCS            := $(wildcard *.c)
OBJS_32         := $(SRCS:.c=.o32)
OBJS_64         := $(SRCS:.c=.o64)
TARGET_32       := emuc_32
TARGET_64       := emuc_64
TARGET_32_D		:= emuc_32_D
TARGET_64_D		:= emuc_64_D
LIBS_32         := lib_emuc2_32.a -m32 -lpthread
LIBS_64         := lib_emuc2_64.a -m64 -lpthread
CFLAGS_32       := -m32 -Wall -I ./include -fPIC -D_DEFAULT_SOURCE
CFLAGS_64       := -m64 -Wall -I ./include -fPIC -D_DEFAULT_SOURCE
LDFLAGS_32      := $(LIBS_32)
LDFLAGS_64      := $(LIBS_64)
STATIC			:= 1

ifeq ($(STATIC), 1)
	CFLAGS_32 += -D_STATIC
	CFLAGS_64 += -D_STATIC
else
	TARGET_32 = $(TARGET_32_D)
	TARGET_64 = $(TARGET_64_D)
	LDFLAGS_32	  = -ldl -lpthread
	LDFLAGS_64	  = -ldl -lpthread
endif


############################################
LBITS           := $(shell getconf LONG_BIT)

.PHONY: all both clean

ifeq ($(LBITS),64)
    all: .depend $(TARGET_64)
else
    all: .depend $(TARGET_32)
endif

both: .depend $(TARGET_32) $(TARGET_64)
############################################


%.o32: %.c Makefile
	$(Q)echo "  Compiling '$<' ..."
	$(Q)$(CC) $(CFLAGS_32) -o $@ -c $<

%.o64: %.c Makefile
	$(Q)echo "  Compiling '$<' ..."
	$(Q)$(CC) $(CFLAGS_64) -o $@ -c $<

$(TARGET_32): $(OBJS_32)
	$(Q)echo "  $(COLOR_G)Building '$@' VER=$(AP_VER)... $(COLOR_W)"
ifeq ($(STATIC), 1)
	$(Q)$(CC) -o $@ $(OBJS_32) $(LDFLAGS_32)
else
	$(Q)$(CC) -o $(TARGET_32) $(OBJS_32) $(LDFLAGS_32)
endif

$(TARGET_64): $(OBJS_64)
	$(Q)echo "  $(COLOR_G)Building '$@' VER=$(AP_VER)... $(COLOR_W)"
ifeq ($(STATIC), 1)
	$(Q)$(CC) -o $@ $(OBJS_64) $(LDFLAGS_64)
else
	$(Q)$(CC) -o $(TARGET_64) $(OBJS_64) $(LDFLAGS_64)
endif

clean:
	$(Q)rm -f .depend *~ *.bak *.res *.o32 *.o64
	$(Q)echo "  Cleaning '$(TARGET_32)' ..."
	$(Q)rm -f $(TARGET_32)
	$(Q)echo "  Cleaning '$(TARGET_64)' ..."
	$(Q)rm -f $(TARGET_64)

.depend:
	$(Q)echo "  Generating '$@' ..."
	$(Q)$(CC) $(CFLAGS_32) -M *.c > $@
	$(Q)$(CC) $(CFLAGS_64) -M *.c > $@

ifeq (.depend, $(wildcard .depend))
	include .depend
endif
