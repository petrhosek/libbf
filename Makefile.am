ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = -I$(top_builddir)/include

MAINTAINERCLEANFILES = \
	Makefile.in

lib_LTLIBRARIES = libind.la
libind_la_SOURCES = \
	lib/bf_basic_blk.c \
	lib/bf_func.c \
	lib/bf_mem_manager.c \
	lib/bf_cfg.c \
	lib/bf_insn.c \
	lib/bf_sym.c \
	lib/bf_disasm.c \
	lib/bf_insn_decoder.c \
	lib/binary_file.c
pkginclude_HEADERS = \
	include/bf_basic_blk.h \
	include/bf_func.h \
	include/bf_mem_manager.h \
	include/bf_cfg.h \
	include/bf_insn.h \
	include/bf_sym.h \
	include/bf_disasm.h \
	include/bf_insn_decoder.h \
	include/binary_file.h

include aminclude.am

# Build rules for samples and tests
# TESTS: Programs run automatically by "make check"
# check_PROGRAMS: Programs built by "make check" but not necessarily run
# check_SCRIPTS: Scripts run by "make check"

TESTS =
TESTS_ENVIRONMENT = \
	TEST_SOURCE_DIR=$(srcdir)/tests \
	TEST_BUILD_DIR=$(top_builddir)/tests

check_PROGRAMS =
check_SCRIPTS =

unit_test_CPPFLAGS = -I$(top_srcdir)/tests -DUNIT_TESTING=1

TESTS += tests/coreutils_test
check_PROGRAMS += tests/coreutils_test
tests_coreutils_test_SOURCES = tests/coreutils_test.c
tests_coreutils_test_CPPFLAGS = $(unit_test_CPPFLAGS)
tests_coreutils_test_LDADD = $(top_builddir)/libind.la

libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck

EXTRA_DIST = autogen.sh libtool $(SCRIPTS)
