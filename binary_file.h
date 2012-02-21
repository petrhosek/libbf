#ifndef BINARY_FILE_H
#define BINARY_FILE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <bfd.h>
#include <dis-asm.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <libiberty.h>
#include "include/htable.h"

/*
 * Internal context for disassembly. Allows us to pass in more information
 * to our custom fprintf function. Currently only passes in insn but can be
 * extended later.
 */
struct disasm_context {
	/*
	 * Instruction being disassembled.
	 */
	struct bf_insn *      insn;
};

/*
 * Our wrapper around BFD. Eventually more members will be added to this.
 * Currently, we are exposing the definition so users can directly access
 * members but we can change this later.
 */
struct binary_file {
	/*
	 * We are wrapping the BFD object within our own one.
	 */
	struct bfd *		abfd;

	/*
	 * Holds the disassembler corresponding to the BFD object.
	 */
	disassembler_ftype	disassembler;

	/*
	 * Holds the configuration used by libopcodes for disassembly.
	 */
	struct disassemble_info disasm_config;

	/*
	 * Hashtable holding list of all the currently discovered functions.
	 * The implementation is that the address of a function is its key.
	 */
	struct htable		func_table;

	/*
	 * Hashtable holding list of all the currently discovered basic blocks.
	 * The implementation is that the address of a basic block is its key.
	 */
	struct htable		bb_table;

	/*
	 * Hashtable holding list of all currently discovered instructions.
	 * The implementation is that the address of a instruction is its key.
	 */
	struct htable		insn_table;

	/*
	 * Hashtable allowing symbol information to be attached to the CFG.
	 */
	struct htable		sym_table;

	/*
	 * Hashtable holding mappings of sections mapped into memory by the
	 * memory manager.
	 */
	struct htable		mem_table;

	/*
	 * Internal disassembly state.
	 */
	struct disasm_context	context;
};

extern void load_sym_table(struct binary_file *);
extern void close_sym_table(struct binary_file *);

/*
 * Returns a binary_file object for the target passed in. NULL if a matching
 * BFD backend could not be found (because the BFD structure is useless to us
 * in that case).
 * 
 * close_binary_file must be called to allow the object to properly clean up.
 */
extern struct binary_file * load_binary_file(char *);

/*
 * Closes a binary_file object obtained from calling load_binary_file.
 */
extern bool close_binary_file(struct binary_file *);


/*
 * Specify a callback which is invoked for each discovered symbol
 */
extern bool binary_file_for_each_symbol(struct binary_file *,
		void (*)(struct binary_file *, asymbol *));

/*
 * At the moment disassembles the entry point instruction. But eventually we
 * want to fill some internal structure of binary_file with the CFG.
 */
extern struct bf_basic_blk * disassemble_binary_file_entry(
		struct binary_file *);

/*
 * Perform a control flow analysis starting from the address of the symbol.
 */
extern struct bf_basic_blk * disassemble_binary_file_symbol(
		struct binary_file *, asymbol *);

#ifdef __cplusplus
}
#endif

#endif
