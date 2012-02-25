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
	bf_basic_blk.h \
	bf_func.h \
	bf_mem_manager.h \
	bf_cfg.h \
	bf_insn.h \
	bf_sym.h \
	bf_disasm.h \
	bf_insn_decoder.h \
	binary_file.h

EXTRA_DIST = autogen.sh
