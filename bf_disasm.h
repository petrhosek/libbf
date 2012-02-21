#ifndef BF_DISASM_H
#define BF_DISASM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "binary_file.h"
#include <ctype.h>

/*
 * This file contains libopcodes callbacks and internal disassembling functions
 */

/*
 * Provides the callback as part of libopcodes initialisation. Will eventually
 * be renamed.
 */
extern int binary_file_fprintf(void *, const char *, ...);

/*
 * Disassemble from the passed in VMA and construct a CFG. The bool specifies
 * whether the symbol should be treated as a function.
 */
extern struct bf_basic_blk * disasm_generate_cflow(struct binary_file *,
		bfd_vma, bool);

/*
 * Disassemble from a symbol. The bool specifies whether the symbol should be
 * treated as a function.
 */
extern struct bf_basic_blk * disasm_from_sym(struct binary_file *, asymbol *,
		bool);

#ifdef __cplusplus
}
#endif

#endif
