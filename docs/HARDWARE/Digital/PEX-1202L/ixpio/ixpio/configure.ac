dnl Process this file with autoconf to produce a configure script.
AC_INIT(ixpio, 0.23.7)
AC_CONFIG_SRCDIR(driver/_pio.c)
AM_INIT_AUTOMAKE
#AM_CONFIG_HEADER(config.h:config.h.in)

compare=64
OSbit=`getconf LONG_BIT`
AM_CONDITIONAL(BITS,test $OSbit = $compare)

dnl Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET
# AC_ARG_PROGRAM

dnl Checks for libraries.

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(fcntl.h sys/ioctl.h unistd.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_DEFINE(_GNU_SOURCE)

dnl Checks for library functions.
AC_PROG_GCC_TRADITIONAL

dnl Checks for kernel source...
AC_MSG_CHECKING(for kernel source)
AC_ARG_WITH(kernel-source,
	[  --with-kernel=path      give the path for kernel sources (auto)],
	kerneldir="$withval",
	kerneldir="/lib/modules/`uname -r`/build"
	if test ! -r "$kerneldir/include/linux/version.h" && test ! -r "$kerneldir/include/generated/uapi/linux/version.h"; then
		AC_MSG_RESULT("no")
		AC_MSG_ERROR([You have to install the kernel source first.])
	fi
)
AC_SUBST(kerneldir)
AC_MSG_RESULT($kerneldir)

dnl Directory for modules
AC_MSG_CHECKING(for module directory)
AC_ARG_WITH(kmoddir,
	[  --with-kmoddir=path     give the path for installing kernel modules]
	[                  (auto)],
	kmoddir="$withval",
# Special test for RH 5.1
	if test -d /lib/modules/preferred; then
		kmoddir="/lib/modules/preferred/misc"
	else
		kmoddir="/lib/modules/`uname -r`/misc"
	fi
)
AC_SUBST(kmoddir)
AC_MSG_RESULT($kmoddir)

dnl check for directory of compiler...
AC_MSG_CHECKING(for compiler directory)

kver="2`uname -r | sed -ne 's/^2\.[4]\..*/4/p'`x"	
if test $kver = "24x"; then
ccdir=`$CC -v > ccver 2>> ccver; sed -n '/specs from/p' ccver | sed '/^.*from /s' | sed '/\/specs$/s'`
AC_MSG_RESULT($ccdir)
fi

dnl AC_OUTPUT(Makefile.conf)
AC_SUBST_FILE(Makefile_conf)
Makefile_conf="Makefile.conf"

dnl if kernel version equal 2.4.x establish driver/Makefile_linux24x
if test $kver = "24x"; then
AC_OUTPUT(driver/Makefile_linux24x)
fi

AC_OUTPUT(Makefile.conf examples/Makefile lib/Makefile lib/src/Makefile examples/pisop8r8/Makefile examples/pisop16r16_pexp8r8_p16r16/Makefile examples/piso813/Makefile examples/piso725/Makefile examples/pisop32c32/Makefile examples/pisop32a32/Makefile examples/piso730a/Makefile examples/piso730/Makefile examples/piod96/Makefile examples/pioda16/Makefile examples/piod24/Makefile examples/piod48_pexd48/Makefile examples/piod56/Makefile examples/piod64/Makefile examples/piod144/Makefile examples/piod168/Makefile examples/pisop64/Makefile examples/pisoa64/Makefile examples/pisoc64/Makefile examples/pisoda2/Makefile examples/pisops300/Makefile examples/pisoenc300/Makefile examples/pisoenc600/Makefile examples/pio821/Makefile)
