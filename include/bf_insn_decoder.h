/**
 * @internal
 * @file bf_insn_decoder.h
 * @brief API of bf_insn_decoder.
 * @details This instruction decoder is responsible for analysing the output
 * of libopcodes. It fills in internal libopcodes information about what type
 * an instruction is as well as its branch targets (if any).
 *
 * The decoder is optimised to not repeat checks for instructions. The
 * implementation is written such that an instruction should strictly be
 * checked with breaks_flow() before branches_flow().
 *
 * Overall, we only care to distinguish between five types of instructions:
 *	- The most common type which does not affect control flow
 * (e.g. mov, cmp).
 *	- Instructions that break control flow (e.g. jmp).
 *	- Instructions that branch control flow (e.g. conditional jmps).
 *	- Instructions that call subroutines (e.g. call).
 *	- Instructions that end control flow (e.g. ret, sysret).
 * @author Mike Kwan <michael.kwan08@imperial.ac.uk>
 */

#ifndef BF_INSN_DECODER_H
#define BF_INSN_DECODER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "binary_file.h"

/**
 * @brief Returns whether the instruction breaks flow.
 * @param str The instruction being analysed.
 * @return TRUE if str represents an instruction which breaks flow. FALSE
 * otherwise.
 */
extern bool breaks_flow(char * str);

/**
 * @brief Returns whether the instruction branches flow.
 * @param str The instruction being analysed.
 * @return TRUE if str represents an instruction which branches flow. FALSE
 * otherwise.
 */
extern bool branches_flow(char * str);

/**
 * @brief Returns whether the instruction calls a subroutine.
 * @param str The instruction being analysed.
 * @return TRUE if str represents an instruction which calls a subroutine.
 * FALSE otherwise.
 */
extern bool calls_subroutine(char * str);

/**
 * @brief Returns whether the instruction ends flow.
 * @param str The instruction being analysed.
 * @return TRUE if str represents an instruction which ends flow. FALSE
 * otherwise.
 */
extern bool ends_flow(char * str);

/**
 * @brief Returns the VMA parsed from an operand.
 * @param str The operand being analysed.
 * @return The VMA if it could be successfully parsed. If the VMA is unknown
 * because of indirect branching/calling then 0 is returned.
 */
extern bfd_vma get_vma_target(char * str);

#ifdef __cplusplus
}
#endif

#endif
