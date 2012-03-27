/**
 * @file bf_detour.h
 * @brief Definition and API of bf_detour.
 * @details bf_detour provides functionality to patch detours and trampolines
 * into x86-32 and x86-64 targets at both the function and basic block
 * granularity.
 * @note A binary_file is only eligible for patching and trampolining if
 * an output_file was specified during the load_binary_file() initialisation.
 * @author Mike Kwan <michael.kwan08@imperial.ac.uk>
 */

#ifndef BF_DETOUR_H
#define BF_DETOUR_H

#ifdef __cplusplus
extern "C" {
#endif

#define _GNU_SOURCE
#include <string.h>
#include "binary_file.h"
#include "bf_func.h"
#include "bf_basic_blk.h"
#include "bf_mem_manager.h"

#define BF_DETOUR_LENGTH32     5
#define BF_DETOUR_LENGTH64     14

#define BF_TRAMPOLINE_LENGTH64 41

/**
 * @brief Detours execution from one bf_func to another.
 * @param src_func The source bf_func (where the execution is detoured from).
 * @param dest_func The destination bf_func (where the execution is detoured
 * to).
 * @returns TRUE if the detour was set. FALSE otherwise.
 * @details A bf_func can be detoured only if its first basic block is at least
 * 5 bytes for x86-32 and 14 bytes for x86-64.
 */
bool bf_detour_func(struct binary_file * bf, struct bf_func * src_func,
		struct bf_func * dest_func);

/**
 * @brief Detours execution from one bf_basic_blk to another.
 * @param src_bb The source bf_basic_blk (where the execution is detoured
 * from).
 * @param dest_bb The destintation bf_basic_blk (where the execution is
 * detoured to).
 * @returns TRUE if the detour was set. FALSE otherwise.
 * @details A bf_basic_blk can be detoured only if it is at least 5 bytes for
 * x86-32 and 14 bytes for x86-64.
 */
bool bf_detour_basic_blk(struct binary_file * bf, struct bf_basic_blk * src_bb,
		struct bf_basic_blk * dest_bb);

/**
 * @brief Detours execution from one bf_func to another. Additionally, writes a
 * trampoline from the end of the destination bf_func back to the source
 * bf_func.
 * @param src_func The source bf_func (where the execution is detoured from).
 * @param dest_func The destination bf_func (where the execution is detoured
 * to).
 * @returns TRUE if the detour and trampoline were set. FALSE otherwise.
 * @details A bf_func can be trampolined only if it can be detoured
 * (see bf_detour_func()). Additionally, there must be a special padding at
 * the end of the function for the trampoline to be inserted. This should
 * consist of BF_TRAMPOLINE_LENGTH32 and BF_TRAMPOLINE_LENGTH64 'nop'
 * instructions for x86-32 and x86-64 respectively. If this padding is not
 * present, the behaviour of bf_detour_func_with_trampoline() is undefined.
 */
bool bf_detour_func_with_trampoline(struct binary_file * bf,
		struct bf_func * src_func, struct bf_func * dest_func);

/**
 * @brief Detours execution from one bf_basic_blk to another. Additionally,
 * writes a trampoline from the end of the destination bf_basic_blk back to the
 * source bf_basic_blk.
 * @param src_bb The source bf_basic_blk (where the execution is detoured
 * from).
 * @param dest_bb The destination bf_basic_blk (where the execution is detoured
 * to).
 * @returns TRUE if the detour and trampoline were set. FALSE otherwise.
 * @details A bf_basic_blk can be trampolined only if it can be detoured
 * (see bf_detour_basic_blk()). Additionally, there must be a special padding
 * at the end of the function for the trampoline to be inserted. This should
 * consist of BF_TRAMPOLINE_LENGTH32 and BF_TRAMPOLINE_LENGTH64 'nop'
 * instructions for x86-32 and x86-64 respectively. If this padding is not
 * present, the behaviour of bf_detour_basic_blk_with_trampoline() is
 * undefined.
 */
bool bf_detour_basic_blk_with_trampoline(struct binary_file * bf,
		struct bf_basic_blk * src_bb, struct bf_basic_blk * dest_bb);

#ifdef __cplusplus
}
#endif

#endif
