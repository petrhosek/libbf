ACLOCAL_AMFLAGS = -I m4

AM_CPPFLAGS = -I$(top_srcdir)/include

MAINTAINERCLEANFILES = \
	Makefile.in

lib_LTLIBRARIES = libind.la
libind_la_SOURCES = \
	lib/bf_copy.c \
	lib/bf_detour.c \
	lib/bf_basic_blk.c \
	lib/bf_func.c \
	lib/bf_mem_manager.c \
	lib/bf_cfg.c \
	lib/bf_insn.c \
	lib/bf_sym.c \
	lib/bf_disasm.c \
	lib/bf_insn_decoder.c \
	lib/binary_file.c
libind_la_LDFLAGS = -version-info 0:0:0
pkginclude_HEADERS = \
	include/bf_copy.h \
	include/bf_detour.h \
	include/bf_basic_blk.h \
	include/bf_func.h \
	include/bf_mem_manager.h \
	include/bf_cfg.h \
	include/bf_insn.h \
	include/bf_sym.h \
	include/bf_disasm.h \
	include/bf_insn_decoder.h \
	include/binary_file.h \
	include/bfd/elf-bfd.h \
	include/bfd/elf/common.h \
	include/bfd/elf/external.h \
	include/bfd/elf/internal.h \
	include/bfd/bfdlink.h

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

TESTS += tests/coreutils_test32.test
TESTS += tests/coreutils_test64.test
check_PROGRAMS += tests/coreutils_test
tests_coreutils_test_SOURCES = tests/coreutils_test.c
tests_coreutils_test_CPPFLAGS = $(unit_test_CPPFLAGS)
tests_coreutils_test_LDADD = $(top_builddir)/libind.la

TESTS += tests/disasm_engine_test32.test
TESTS += tests/disasm_engine_test64.test
check_PROGRAMS += tests/disasm_engine_test
tests_disasm_engine_test_SOURCES = tests/disasm_engine_test.c
tests_disasm_engine_test_CPPFLAGS = $(unit_test_CPPFLAGS)
tests_disasm_engine_test_LDADD = $(top_builddir)/libind.la

TESTS += tests/copy_test32.test
TESTS += tests/copy_test64.test
check_PROGRAMS += tests/copy_test
tests_copy_test_SOURCES = tests/copy_test.c
tests_copy_test_CPPFLAGS = $(unit_test_CPPFLAGS)
tests_copy_test_LDADD = $(top_builddir)/libind.la

TESTS += tests/detour_test32.test
TESTS += tests/detour_test64.test
check_PROGRAMS += tests/detour_test
tests_detour_test_SOURCES = tests/detour_test.c
tests_detour_test_CPPFLAGS = $(unit_test_CPPFLAGS)
tests_detour_test_LDADD = $(top_builddir)/libind.la

TESTS += tests/trampoline_test32.test
TESTS += tests/trampoline_test64.test
check_PROGRAMS += tests/trampoline_test
tests_trampoline_test_SOURCES = tests/trampoline_test.c
tests_trampoline_test_CPPFLAGS = $(unit_test_CPPFLAGS)
tests_trampoline_test_LDADD = $(top_builddir)/libind.la

libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck

EXTRA_DIST = \
	autogen.sh libtool $(SCRIPTS) \
	tests/coreutils_test32.test \
	tests/coreutils_test64.test \
	tests/disasm_engine_test32.test \
	tests/disasm_engine_test64.test \
	tests/copy_test32.test \
	tests/copy_test64.test \
	tests/detour_test32.test \
	tests/detour_test64.test \
	tests/trampoline_test32.test \
	tests/trampoline_test64.test
