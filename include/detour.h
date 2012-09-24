/*
 * This file is part of libbf.
 *
 * libbf is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * libbf is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with libbf.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file detour.h
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

#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "binary_file.h"
#include "func.h"
#include "basic_blk.h"
#include "mem_manager.h"

#define BF_DETOUR_LENGTH32     5
#define BF_DETOUR_LENGTH64     14

#define DETOUR_LENGTH(BF)      (IS_BF_ARCH_32(BF) ? BF_DETOUR_LENGTH32 : \
						BF_DETOUR_LENGTH64)

#define BF_TRAMPOLINE_LENGTH32 24
#define BF_TRAMPOLINE_LENGTH64 42

#define TRAMPOLINE_LENGTH(BF)  (IS_BF_ARCH_32(BF) ? BF_TRAMPOLINE_LENGTH32 : \
						BF_TRAMPOLINE_LENGTH64)

#define BF_MAX_INSN_LENGTH     15

/**
 * @brief Detours execution from one bf_func to another.
 * @param src_func The source bf_func (where the execution is detoured from).
 * @param dest_func The destination bf_func (where the execution is detoured
 * to).
 * @returns TRUE if the detour was set. FALSE otherwise.
 * @details A bf_func can be detoured only if its first basic block is at least
 * 5 bytes for x86-32 and 14 bytes for x86-64.
 */
bool bf_detour_func(struct bin_file * bf, struct bf_func * src_func,
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
bool bf_detour_basic_blk(struct bin_file * bf, struct bf_basic_blk * src_bb,
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
 * present, the behaviour of bf_trampoline_func() is undefined.
 */
bool bf_trampoline_func(struct bin_file * bf,
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
 * present, the behaviour of bf_trampoline_basic_blk() is undefined.
 */
bool bf_trampoline_basic_blk(struct bin_file * bf,
		struct bf_basic_blk * src_bb, struct bf_basic_blk * dest_bb);

#ifdef __cplusplus
}
#endif

#endif
