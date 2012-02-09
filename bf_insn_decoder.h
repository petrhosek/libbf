#ifndef BF_INSN_DECODER_H
#define BF_INSN_DECODER_H

#include "binary_file.h"

/*
 * This instruction decoder is optimised to not repeat checks for instructions.
 * The implementation is written such that an instruction should strictly be
 * checked with breaks_flow() before branches_flow().
 *
 * Overall, we only care to distinguish between five types of instructions:
 *   - The most common type which does not affect control flow (e.g. mov, cmp).
 *   - Instructions that break control flow (e.g. jmp).
 *   - Instructions that branch control flow (e.g. conditional jmps).
 *   - Instructions that call subroutines (e.g. call).
 *   - Instructions that end control flow (e.g. ret, sysret).
 */

/*
 * Returns whether the instruction breaks flow.
 */
extern bool breaks_flow(char *);

/*
 * Returns whether the instruction branches flow.
 */
extern bool branches_flow(char *);

/*
 * Returns whether the instruction calls a subroutine.
 */
extern bool calls_subroutine(char *);

/*
 * Returns whether the instruction ends flow.
 */
extern bool ends_flow(char *);

/*
 * Returns the VMA parsed from an operand. If the VMA is unknown because of
 * indirect branching/calling then return 0.
 */
extern bfd_vma get_vma_target(char *);

#endif
