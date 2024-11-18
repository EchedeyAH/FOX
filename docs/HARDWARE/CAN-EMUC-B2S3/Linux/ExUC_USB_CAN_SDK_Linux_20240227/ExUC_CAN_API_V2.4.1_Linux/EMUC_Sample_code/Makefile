Q := @

ifeq ($(OS),Windows_NT)
	MAKEFILE := Win32.mk
else
	ifeq ($(shell uname),Linux)
		ifeq ($(ARCH),arm)
			MAKEFILE        := LinuxArm32.mk
		else
			MAKEFILE        := Linux.mk
		endif
	endif

	ifeq ($(shell qconfig | head -n 1 | cut -b 1-3),QNX)
		MAKEFILE        := QNX.mk
    endif
endif

all:
	$(Q)make -f $(MAKEFILE) all

both:
	$(Q)make -f $(MAKEFILE) both

clean:
	$(Q)make -f $(MAKEFILE) clean