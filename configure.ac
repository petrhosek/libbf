dnl Process this file with autoconf to produce a configure script
AC_PREREQ([2.60])
AC_INIT([libbind], [0.1], [michael.kwan08@imperial.ac.uk])

AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.11 foreign subdir-objects color-tests -Wall -Werror])
AM_MAINTAINER_MODE
AM_SILENT_RULES([yes])
LT_INIT

AC_LANG([C])

dnl Toolchain checks
AC_PROG_CC_C99
if test x"$ac_cv_prog_cc_c99" = x"no"; then
	AC_MSG_ERROR([C compiler supporting C99 required])
fi
AC_PROG_LIBTOOL
AM_PROG_CC_C_O
AM_PROG_AS

dnl Operating system checks
AC_MSG_CHECKING([for supported operating system])
case "${host_os}" in
linux*)
	HOST_OS=linux
	AC_DEFINE([LINUX], 1, [Define for the Linux kernel.])
	;;
*)
	AC_MSG_RESULT([no])
	AC_MSG_ERROR([Operating system ${host_os} not supported])
	;;
esac
AC_SUBST([HOST_OS])
AM_CONDITIONAL([LINUX], [test x$HOST_OS = xlinux])

dnl System architecture checks
AC_MSG_CHECKING([for supported architecture])
case "${host_cpu}" in
i[[3456]]86|pentium)
	HOST_CPU=x86
	AC_DEFINE([X86], 1, [Define for the i386 architecture.])
;;
x86?64*|amd64)
	HOST_CPU=x86_64
	AC_DEFINE([X86_64], 1, [Define for the AMD x86-64 architecture.])
;;
*)
	AC_MSG_RESULT([no])
	AC_MSG_ERROR([Architecture ${host_cpu} not supported])
esac
AC_SUBST([HOST_CPU])
AM_CONDITIONAL([X86], [test x$HOST_CPU = xx86])
AM_CONDITIONAL([X86_64], [test x$HOST_CPU = xx86_64])

dnl Checks for programs
AC_PATH_PROG([TCLSH], [tclsh])
AC_PATH_PROG([BASH], [bash])

dnl Checks for libraries
AC_CHECK_LIB([bfd], [bfd_init], [], [AC_MSG_ERROR([Missing GNU binutils])])
AC_CHECK_LIB([opcodes], [init_disassemble_info], [], [AC_MSG_ERROR([Missing GNU binutils])])
AC_CHECK_LIB([kern], [find_next_bit], [KERN_LIBS='-lkern'; AC_SUBST([KERN_LIBS])], [AC_MSG_ERROR([Missing libkern])])

dnl Checks for typedefs
AC_TYPE_SIZE_T

dnl Check for sizes
AC_CHECK_SIZEOF([int])
AC_CHECK_SIZEOF([long])
AC_CHECK_SIZEOF([long long])

dnl Checks for header files
AC_HEADER_ASSERT
AC_HEADER_STDBOOL
AC_HEADER_STDC
AC_HEADER_TIME

AC_CHECK_HEADERS([bfd.h dis-asm.h], [], [AC_MSG_ERROR([Missing GNU binutils headers])])
AC_CHECK_HEADERS([demangle.h])

dnl Checks for library functions
AC_FUNC_VPRINTF
if test x$ac_cv_func_strerror = xno; then
  AC_CHECK_LIB(iberty, main)
fi

dnl Configuration files
AC_CONFIG_HEADERS([include/config.h])
AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([tests/testprepare.sh:tests/testprepare.in], [chmod +x tests/testprepare.sh])

dnl Doxygen documentation
DX_INIT_DOXYGEN([libbind], [Doxyfile])
DX_DOXYGEN_FEATURE([ON])
DX_PS_FEATURE([OFF])
DX_HTML_FEATURE([ON])

AC_OUTPUT
