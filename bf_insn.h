#ifndef BF_INSN_H
#define BF_INSN_H

#include "binary_file.h"
#include "include/list.h"

struct bf_insn_part {
	struct list_head list;
	char *		 str;
};

/*
 * Our abstraction of an instruction. A bf_insn consists of a list of parts
 * which hold the mnemonic and operands.
 */
struct bf_insn {
	/*
	 * Holds the address of the instruction.
	 */
	bfd_vma		 vma;

	/*
	 * Start of linked list of parts.
	 */
	struct list_head part_list;
};

/*
 * Returns a bf_insn object. close_bf_insn must be called to allow the object
 * to properly clean up.
 */
extern struct bf_insn * init_bf_insn(bfd_vma);

/*
 * Adds to the tail of the part list.
 */
extern void add_insn_part(struct bf_insn *, char *);

/*
 * Prints the bf_insn to stdout.
 */
extern void print_bf_insn(struct bf_insn *);

/*
 * Closes a bf_insn obtained from calling init_bf_insn.
 */
extern void close_bf_insn(struct bf_insn *);

#endif
