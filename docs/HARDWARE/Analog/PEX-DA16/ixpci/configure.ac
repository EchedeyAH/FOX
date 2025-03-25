dnl Process this file with autoconf to produce a configure script.
AC_INIT(ixpci, 0.8.19)
AC_CONFIG_SRCDIR(driver/_pci.c)
AM_INIT_AUTOMAKE
# AM_CONFIG_HEADER(config.h:config.h.in)

compare=64
OSbit=`getconf LONG_BIT`
AM_CONDITIONAL(BITS,test $OSbit = $compare)

dnl Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET
dnl AC_ARG_PROGRAM

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
AC_CHECK_FUNCS(strtol)

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

AC_OUTPUT(Makefile.conf lib/Makefile lib/src/Makefile examples/Makefile examples/pcitmc12/Makefile examples/pcip16x16/Makefile examples/pcip8r8/Makefile examples/pci1002_pex1002/Makefile examples/pci1202/Makefile examples/pci1800/Makefile examples/pcim512/Makefile examples/pcifc16/Makefile examples/pcilanner/Makefile examples/pci826/Makefile examples/pci1602/Makefile examples/pcie8622/Makefile examples/pcid96_d128/Makefile examples/pci2602u/Makefile)
