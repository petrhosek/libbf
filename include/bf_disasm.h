/**
 * @internal
 * @file bf_disasm.h
 * @brief API of bf_disasm.
 * @details bf_disasm is responsible for wrapping the <b>libopcodes</b>
 * disassembler and generating useful CFGs. bf_disasm abstracts the callbacks
 * required by <b>libopcodes</b> as well as the internal disassembling
 * functions of <b>libind</b>.
 *
 * The functions provided should not be used by users of <b>libind</b>. They
 * are internal functions exposed for convenience.
 * disassemble_binary_file_entry() and disassemble_binary_file_symbol(),
 * which are thin wrappers around bf_disasm functions, should be called for
 * the generation of CFGs.
 * @author Mike Kwan <michael.kwan08@imperial.ac.uk>
 */

#ifndef BF_DISASM_H
#define BF_DISASM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <ctype.h>

/**
 * @brief This is an internal callback invoked by the <b>libopcodes</b>
 * disassembler.
 * @details The callback is invoked for each part of an instruction. How an 
 * instruction is split is an implementation detail of <b>libopcodes</b>.
 */
extern int binary_file_fprintf(void *, const char *, ...);

/**
 * @brief Generate a CFG from a specified root.
 * @param bf The binary file being analysed.
 * @param vma The VMA to start analysis from.
 * @param is_func A bool specifying whether the VMA passed in should be treated
 * as the start of a function.
 * @return The first basic block of the generated CFG.
 */
extern struct basic_blk * disasm_generate_cflow(struct bin_file * bf,
		bfd_vma vma, bool is_func);

/**
 * @brief Generate a CFG using the address of the symbol as the root.
 * @param bf The binary file being analysed.
 * @param sym The symbol to start analysis from.
 * @param is_func A bool specifying whether the VMA passed in should be treated
 * as the start of a function.
 * @return The first basic block of the generated CFG.
 */
extern struct basic_blk * disasm_from_sym(struct bin_file * bf,
		struct symbol * sym, bool is_func);

#ifdef __cplusplus
}
#endif

#endif
