/**
 * @file bf_detour.h
 * @brief Definition and API of bf_detour.
 * @details bf_detour provides functionality to patch detours and trampolines
 * into x86-32 and x86-64 targets at both the function and basic block
 * granularity.
 * @author Mike Kwan <michael.kwan08@imperial.ac.uk>
 */

#ifndef BF_DETOUR_H
#define BF_DETOUR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "binary_file.h"
#include "bf_func.h"
#include "bf_basic_blk.h"
#include "bf_mem_manager.h"

#define BF_DETOUR_LENGTH32 5
#define BF_DETOUR_LENGTH64 14

/**
 * @brief Detours execution from one bf_func to another.
 * @param src_func The source bf_func (where the execution is detoured from).
 * @param dest_func The destination bf_func (where the execution is detoured
 * to).
 * @details ... Talk about how it refreshes basic blocks...
 */
void bf_detour_func(struct binary_file * bf, struct bf_func * src_func,
		struct bf_func * dest_func);

#ifdef __cplusplus
}
#endif

#endif
