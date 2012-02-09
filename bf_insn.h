#ifndef BF_INSN_H
#define BF_INSN_H

#include "binary_file.h"
#include "include/list.h"

typedef struct bf_insn_part {
	struct list_head list;
	char *		 str;
} bf_insn_part;

/*
 * Our abstraction of an instruction. libopcodes discards instruction
 * information so this is our way of storing it. A bf_insn consists of a list
 * of parts which hold the mnemonic and operands.
 */
typedef struct bf_insn {
	/*
	 * Holds the address of the instruction.
	 */
	bfd_vma		 vma;

	/*
	 * Start of linked list of parts.
	 */
	struct list_head part_list;
} bf_insn;

/*
 * Returns a bf_insn object. close_bf_insn must be called to allow the object
 * to properly clean up.
 */
extern bf_insn * init_bf_insn(bfd_vma);

/*
 * Adds to the tail of the part list.
 */
extern void add_insn_part(bf_insn *, char *);

/*
 * Prints the bf_insn to stdout.
 */
extern void print_bf_insn(bf_insn *);

/*
 * Closes a bf_insn obtained from calling init_bf_insn.
 */
extern void close_bf_insn(bf_insn *);

#endif
