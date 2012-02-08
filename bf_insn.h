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
 * information so this is our way of storing it.
 */
typedef struct bf_insn {
	/*
	 * Holds the list of parts for the instruction
	 */
	bf_insn_part * part_list;
} bf_insn;

extern bf_insn * init_bf_insn(void);
extern void add_insn_part(bf_insn *, char *);
extern void print_bf_insn(bf_insn *);
extern void close_bf_insn(bf_insn *);

#endif
