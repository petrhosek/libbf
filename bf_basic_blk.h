#ifndef BF_BASIC_BLK_H
#define BF_BASIC_BLK_H

#include "binary_file.h"
#include "include/list.h"

typedef struct bf_basic_blk_part {
	struct list_head list;
	bf_insn *	 insn;
} bf_basic_blk_part;

/*
 * Our abstraction of a basic block. A bf_basic_blk consists of a list of its
 * constituent instructions.
 */
typedef struct bf_basic_blk {
	/*
	 * Holds that the basic block starts at.
	 */
	bfd_vma		 vma;

	/*
	 * Start of linked list of parts (instructions).
	 */
	struct list_head part_list;
} bf_basic_blk;

/*
 * Returns a bf_basic_blk object. close_bf_basic_blk must be called to allow
 * the object to properly clean up.
 */
extern bf_basic_blk * init_bf_basic_blk(bfd_vma);

/*
 * Adds to the tail of the instruction list.
 */
extern void add_insn(bf_basic_blk *, bf_insn *);

/*
 * Prints the bf_basic_blk to stdout.
 */
extern void print_bf_basic_blk(bf_basic_blk *);

/*
 * Closes a bf_basic_blk obtained from calling init_bf_basic_blk. This will
 * also call close_bf_insn for each bf_insn contained in the basic block.
 */
extern void close_bf_basic_blk(bf_basic_blk *);

#endif
