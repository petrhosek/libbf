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

/** @file
 * \enum arch_bitiness
 * \brief Enumeration of architecture bitiness.
 *
 * Since we support x86-32 and x86-64, it is good enough to have just two
 * members.
 */
enum arch_bitiness {
	arch_64, /**< enum value x86-64. */
	arch_32  /**< enum value x86-32. */
};

/**
 * \struct disasm_context
 * \brief Internal context for disassembly.
 * 
 * Allows us to pass in more information to our custom fprintf function.
 * Currently only passes in the bf_insn being disassembled but can be extended
 * if necessary.
 */
struct disasm_context {
	struct bf_insn * insn; /**< Instruction being disassembled. */
};

/**
 * \struct binary_file
 * \brief The abstraction used for a binary file.
 *
 * This structure encapsulates the information necessary to use the tool.
 * Primarily this is our way of wrapping and abstracting away from BFD.
 */
struct binary_file {
	/** Wrapping the BFD object. */
	struct bfd *		abfd;

	/** Flag denoting the bitiness of the target. */
	enum arch_bitiness	bitiness;

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
 * Starts building a control flow graph from entry point.
 */
extern struct bf_basic_blk * disassemble_binary_file_entry(
		struct binary_file *);

/*
 * Perform a control flow analysis starting from the address of the symbol.
 * The bool passed in specifies whether the root passed in should be treated as
 * a function.
 */
extern struct bf_basic_blk * disassemble_binary_file_symbol(
		struct binary_file *, asymbol *, bool);

#ifdef __cplusplus
}
#endif

#endif
