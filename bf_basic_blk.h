#ifndef BF_BASIC_BLK_H
#define BF_BASIC_BLK_H

#include <stdio.h>
#include "include/list.h"
#include "bf_insn.h"
#include "bf_sym_tab.h"

struct bf_basic_blk_part {
	struct list_head list;
	struct bf_insn * insn;
};

/*
 * Our abstraction of a basic block. A bf_basic_blk consists of a list of its
 * constituent instructions.
 */
struct bf_basic_blk {
	/*
	 * Holds the VMA that the basic block starts at.
	 */
	bfd_vma		      vma;

	/*
	 * Start of linked list of parts (instructions).
	 */
	struct list_head      part_list;

	/*
	 * Entry into the hashtable of binary_file.
	 */
	struct htable_entry   entry;

	/*
	 * Points to the next basic block in the CFG.
	 */
	struct bf_basic_blk * target;

	/*
	 * If the basic block conditionally branches in some way, this points
	 * the second basic block reachable from this one.
	 */
	struct bf_basic_blk * target2;

	/*
	 * A symbol associated with the address of the basic block.
	 */
	struct bf_sym *	      sym;
};

typedef struct bf_basic_blk * BF_BASIC_BLK_PTR;

/*
 * Returns a bf_basic_blk object. bf_close_basic_blk must be called to allow
 * the object to properly clean up.
 */
extern struct bf_basic_blk * bf_init_basic_blk(struct binary_file *, bfd_vma);

/*
 * Splits a bf_basic_blk at the specified VMA. Returns the new bf_basic_blk.
 */
extern struct bf_basic_blk * bf_split_blk(struct binary_file *,
		struct bf_basic_blk *, bfd_vma);

/*
 * Creates a link between the from the first basic block to the second one.
 */
extern void bf_add_next_basic_blk(struct bf_basic_blk *,
		struct bf_basic_blk *);

/*
 * Adds to the tail of the instruction list.
 */
extern void bf_add_insn_to_bb(struct bf_basic_blk *, struct bf_insn *);

/*
 * Prints the bf_basic_blk to stdout.
 */
extern void bf_print_basic_blk(struct bf_basic_blk *);

/*
 * Prints the bf_basic_blk to a FILE.
 */
extern void bf_print_basic_blk_dot(FILE *, struct bf_basic_blk *);

/*
 * Closes a bf_basic_blk obtained from calling bf_init_basic_blk. This will
 * not call bf_close_insn for each bf_insn contained in the basic block.
 */
extern void bf_close_basic_blk(struct bf_basic_blk *);

/*
 * Adds a basic block to the bb_table of binary_file.
 */
extern void bf_add_bb(struct binary_file *, struct bf_basic_blk *);

/*
 * Gets basic block for the starting VMA.
 */
extern struct bf_basic_blk * bf_get_bb(struct binary_file *, bfd_vma);

/*
 * Checks for the existence of a basic block in the bb_table of binary_file.
 */
extern bool bf_exists_bb(struct binary_file *, bfd_vma);

/*
 * Releases memory for all basic blocks currently stored.
 */
extern void close_bb_table(struct binary_file *);

#endif
