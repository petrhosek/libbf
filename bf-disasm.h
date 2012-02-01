#include "binary_file.h"

#ifndef BF_DISASM_H
#define BF_DISASM_H

/*
 * This file contains libopcodes callbacks
 */

/*
 * Provides the callback as part of libopcodes initialisation.
 * Will eventually be renamed.
 */
extern int binary_file_fprintf(void *, const char *, ...);

/*
 * Disassemble from the passed in VMA and construct a CFG.
 */
extern bool disassemble_binary_file_cflow(binary_file *, bfd_vma);

#endif
