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
 * @file cfg.h
 * @brief High level CFG dumping APIs.
 * @details bf_cfg contains functions designed for dumping an entire CFG
 * conveniently. These functions are useful for general debugging and
 * demonstration. If a more customised output is desired, the CFG can be
 * manually traversed through its starting bf_basic_blk.
 * @author Mike Kwan <michael.kwan08@imperial.ac.uk>
 */

#ifndef BF_CFG_H
#define BF_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "basic_blk.h"

/**
 * @brief Prints the CFG starting at a bf_basic_blk to stdout.
 * @param bb The bf_basic_blk of the root of the CFG.
 */
extern void print_cfg_stdout(struct bf_basic_blk * bb);

/**
 * @brief Prints the CFG starting at a bf_basic_blk as a DOT file.
 * @param stream An open FILE to be written to.
 * @param bf The bin_file holding the bf_basic_blk objects.
 * @param bb The bf_basic_blk of the root of the CFG.
 */
extern void print_cfg_dot(FILE * stream, struct bin_file * bf,
		struct bf_basic_blk * bb);

/**
 * @brief Prints all discovered bf_basic_blk objects to stdout.
 * @param bf The bin_file holding the bf_basic_blk objects.
 */
extern void print_entire_cfg_stdout(struct bin_file * bf);

/**
 * @brief Prints all discovered bf_basic_blk objects as a DOT file.
 * @param bf The bin_file holding the bf_basic_blk objects.
 * @param stream An open FILE to be written to.
 */
extern void print_entire_cfg_dot(struct bin_file * bf, FILE * stream);

/**
 * @brief Prints all discovered bf_insn objects to a stream.
 * @param bf The bin_file holding the bf_insn objects.
 * @param stream An open FILE to be written to.
 */
extern void print_all_bf_insn(struct bin_file * bf, FILE * stream);

/**
 * @brief Prints all discovered bf_insn objects to a stream. The printed text
 * is generated from the internal semantic information stored in each bf_insn.
 * @param bf The bin_file holding the bf_insn objects.
 * @param stream An open FILE to be written to.
 * @note Theoretically, if the disassembler engine performs lossless parsing of
 * instructions, the output from this function should be the same as the output
 * from print_all_bf_insn (minus any spaces).
 */
extern void print_all_bf_insn_semantic_gen(struct bin_file * bf,
		FILE * stream);

#ifdef __cplusplus
}
#endif

#endif
