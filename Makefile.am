ACLOCAL_AMFLAGS = -I m4

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

EXTRA_DIST = autogen.sh
