#ifndef BF_FUNC_H
#define BF_FUNC_H

#include "binary_file.h"

/*
 * Our abstraction of a function. A function consists of a pointer to its
 * starting block.
 */
struct bf_func {
	/*
	 * Holds the VMA that the function starts at.
	 */
	bfd_vma		      vma;

	/*
	 * Entry into the hashtable of binary_file.
	 */
	struct htable_entry   entry;


	/*
	 * Basic block at the start of this function.
	 */
	struct bf_basic_blk * bb;
};

/*
 * Returns a bf_func object. bf_close_func must be called to allow the object
 * to properly clean up.
 */
extern struct bf_func * bf_init_func(struct bf_basic_blk *, bfd_vma);

/*
 * Closes a bf_func obtained from calling bf_init_func. This will not call
 * bf_close_basic_blk for the bf_basic_blk contained in the function.
 */
extern void bf_close_func(struct bf_func *);

/*
 * Adds a function to the func_table of binary_file.
 */
extern void bf_add_func(struct binary_file *, struct bf_func *);

/*
 * Gets function for the starting VMA.
 */
extern struct bf_func * bf_get_func(struct binary_file *, bfd_vma);

/*
 * Checks for the existence of a function in the func_table of binary_file.
 */
extern bool bf_exists_func(struct binary_file *, bfd_vma);

/*
 * Releases memory for all functions currently stored.
 */
extern void bf_close_func_table(struct binary_file *);
#endif
