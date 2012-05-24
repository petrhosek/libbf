ACLOCAL_AMFLAGS = -I m4

AM_CPPFLAGS = -I$(top_srcdir)/include

MAINTAINERCLEANFILES = \
	Makefile.in

lib_LTLIBRARIES = libbind.la
libbind_la_SOURCES = \
	lib/detour.c \
	lib/basic_blk.c \
	lib/func.c \
	lib/mem_manager.c \
	lib/cfg.c \
	lib/insn.c \
	lib/disasm.c \
	lib/insn_decoder.c \
	lib/symbol.c \
	lib/section.c \
	lib/segment.c \
	lib/binary_file.c
libbind_la_LDFLAGS = -version-info 0:0:0
libbind_la_LIBADD = \
	@KERN_LIBS@

pkginclude_HEADERS = \
	include/detour.h \
	include/basic_blk.h \
	include/func.h \
	include/mem_manager.h \
	include/cfg.h \
	include/insn.h \
	include/disasm.h \
	include/insn_decoder.h \
	include/symbol.h \
	include/section.h \
	include/segment.h \
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

TESTS += tests/coreutils_test32.test
TESTS += tests/coreutils_test64.test
check_PROGRAMS += tests/coreutils_test
tests_coreutils_test_SOURCES = tests/coreutils_test.c
tests_coreutils_test_CPPFLAGS = $(unit_test_CPPFLAGS)
tests_coreutils_test_LDADD = $(top_builddir)/libbind.la

TESTS += tests/disasm_engine_test32.test
TESTS += tests/disasm_engine_test64.test
check_PROGRAMS += tests/disasm_engine_test
tests_disasm_engine_test_SOURCES = tests/disasm_engine_test.c
tests_disasm_engine_test_CPPFLAGS = $(unit_test_CPPFLAGS)
tests_disasm_engine_test_LDADD = $(top_builddir)/libbind.la

TESTS += tests/detour_test32.test
TESTS += tests/detour_test64.test
check_PROGRAMS += tests/detour_test
tests_detour_test_SOURCES = tests/detour_test.c
tests_detour_test_CPPFLAGS = $(unit_test_CPPFLAGS)
tests_detour_test_LDADD = $(top_builddir)/libbind.la

TESTS += tests/trampoline_test32.test
TESTS += tests/trampoline_test64.test
check_PROGRAMS += tests/trampoline_test
tests_trampoline_test_SOURCES = tests/trampoline_test.c
tests_trampoline_test_CPPFLAGS = $(unit_test_CPPFLAGS)
tests_trampoline_test_LDADD = $(top_builddir)/libbind.la

libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck

EXTRA_DIST = \
	autogen.sh libtool $(SCRIPTS) \
	tests/coreutils_test32.test \
	tests/coreutils_test64.test \
	tests/disasm_engine_test32.test \
	tests/disasm_engine_test64.test \
	tests/detour_test32.test \
	tests/detour_test64.test \
	tests/trampoline_test32.test \
	tests/trampoline_test64.test
