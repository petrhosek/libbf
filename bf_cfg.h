#ifndef BF_CFG_H
#define BF_CFG_H

#include "bf_basic_blk.h"

/*
 * Prints the CFG starting from the basic block to stdout.
 */
extern void print_cfg_stdout(struct bf_basic_blk *);

/*
 * Prints a DOT file representing the CFG starting from the basic block.
 */
extern void print_cfg_dot(FILE *, struct bf_basic_blk *);

#endif
