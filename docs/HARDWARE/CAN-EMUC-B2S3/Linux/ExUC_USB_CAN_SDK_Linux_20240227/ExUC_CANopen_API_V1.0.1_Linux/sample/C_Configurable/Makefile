Q := @

ifeq ($(OS),Windows_NT)
	MAKEFILE := Win32.mk
else
	ifeq ($(shell uname),Linux)
		MAKEFILE := Linux.mk
	endif
endif

all:
	$(Q)make -f $(MAKEFILE) all

clean:
	$(Q)make -f $(MAKEFILE) clean