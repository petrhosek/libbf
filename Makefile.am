ACLOCAL_AMFLAGS = -I m4

MAINTAINERCLEANFILES = \
	Makefile.in

ACLOCAL_AMFLAGS = -I m4

lib_LTLIBRARIES = libind.la
libind_la_SOURCES = \
	bf_basic_blk.c \
	bf_func.c \
	bf_mem_manager.c \
	bf_cfg.c \
	bf_insn.c \
	bf_sym.c \
	bf_disasm.c \
	bf_insn_decoder.c \
	binary_file.c
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
