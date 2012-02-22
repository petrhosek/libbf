/**
 * \internal
 * \file bf_disasm.h
 * \brief API of bf_disasm.
 * \details bf_disasm is responsible for wrapping the libopcodes disassembler
 * and generating useful CFGs. bf_disasm abstracts the callbacks required by
 * libopcodes as well as the internal disassembling functions of libind.
 *
 * The functions provided should not be used by users of libind. They are
 * internal functions exposed for convenience. disassemble_binary_file_entry()
 * and disassemble_binary_file_symbol(), which are thin wrappers around
 * bf_disasm functions, should be called for the generation of CFGs.
 * \author Mike Kwan <michael.kwan08@imperial.ac.uk>
 */

#ifndef BF_DISASM_H
#define BF_DISASM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "binary_file.h"
#include <ctype.h>

/**
 * \brief This is an internal callback invoked by the libopcodes disassembler.
 * \warning This callback should only be used internally.
 */
extern int binary_file_fprintf(void *, const char *, ...);

/**
 * \brief Generate a CFG from a specified root.
 * \warning This function should only be used internally.
 */
extern struct bf_basic_blk * disasm_generate_cflow(struct binary_file * bf,
		bfd_vma vma, bool is_func);

/**
 * \brief Generate a CFG using the address of the symbol as the root.
 * \warning This function should only be used internally.
 */
extern struct bf_basic_blk * disasm_from_sym(struct binary_file *, asymbol *,
		bool);

#ifdef __cplusplus
}
#endif

#endif
