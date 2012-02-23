/**
 * \file bf_cfg.h
 * \brief High level CFG dumping APIs.
 * \details bf_cfg contains functions designed for dumping an entire CFG
 * conveniently. These functions are useful for general debugging and
 * demonstration. If a more customised output is desired, the CFG can be
 * manually traversed through its starting bf_basic_blk.
 * \author Mike Kwan <michael.kwan08@imperial.ac.uk>
 */

#ifndef BF_CFG_H
#define BF_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "bf_basic_blk.h"

/**
 * \brief Prints the CFG starting at a bf_basic_blk to stdout.
 * \param bb The bf_basic_blk of the root of the CFG.
 */
extern void print_cfg_stdout(struct bf_basic_blk * bb);

/**
 * \brief Prints the CFG starting at a bf_basic_blk as a DOT file.
 * \param stream An open FILE to be written to.
 * \param bb The bf_basic_blk of the root of the CFG.
 */
extern void print_cfg_dot(FILE * stream, struct bf_basic_blk * bb);

/**
 * \brief Prints all discovered bf_basic_blk objects to stdout.
 * \param bf The binary_file holding the bf_basic_blk objects.
 */
extern void print_entire_cfg_stdout(struct binary_file * bf);

/**
 * \brief Prints all discovered bf_basic_blk objects as a DOT file.
 * \param bf The binary_file holding the bf_basic_blk objects.
 * \param stream An open FILE to be written to.
 */
extern void print_entire_cfg_dot(struct binary_file * bf, FILE * stream);

#ifdef __cplusplus
}
#endif

#endif
