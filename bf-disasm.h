#ifndef BF_DISASM_H
#define BF_DISASM_H

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
 * Disassemble from the passed in VMA and construct a CFG.
 */
extern bool disasm_generate_cflow(binary_file *, bfd_vma);

/*
 * Disassemble from a symbol.
 */
extern bool disasm_from_sym(binary_file *, asymbol *);

#endif
