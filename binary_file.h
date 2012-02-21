/**
 * \file binary_file.h
 */

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

/**
 * \file
 * \enum arch_bitiness
 * \brief Enumeration of architecture bitiness.
 * \details Since we support x86-32 and x86-64, having two members is sufficient.
 */
enum arch_bitiness {
	/**
	 * Enum value for x86-64.
	 */
	arch_64,
	/**
	 * Enum value for x86-32.
	 */
	arch_32
};

/**
 * \struct disasm_context
 * \brief Internal context used by disassembler.
 * \details Allows us to pass in extra information to our custom fprintf
 * function. Currently only passes in the bf_insn being disassembled but
 * can be extended if necessary.
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
	/**
	 * Wrapping the BFD object.
	 */
	struct bfd *		abfd;

	/**
	 * Flag denoting the bitiness of the target.
	 */
	enum arch_bitiness	bitiness;

	/**
	 * Holds the disassembler corresponding to the BFD object.
	 */
	disassembler_ftype	disassembler;

	/**
	 * Holds the configuration used by libopcodes for disassembly.
	 */
	struct disassemble_info disasm_config;

	/**
	 * Hashtable holding list of all the currently discovered bf_func
	 * objects. The implementation is that the address of a function
	 * is its key.
	 */
	struct htable		func_table;

	/**
	 * Hashtable holding list of all the currently discovered bf_basic_blk
	 * objects. The implementation is that the address of a basic block
	 * is its key.
	 */
	struct htable		bb_table;

	/**
	 * Hashtable holding list of all currently discovered bf_insn objects.
	 * The implementation is that the address of a instruction is its key.
	 */
	struct htable		insn_table;

	/**
	 * Hashtable holding list of all discovered bf_sym objects.
	 * The implementation is that the address of a symbol is its key.
	 */
	struct htable		sym_table;

	/**
	 * Hashtable holding mappings of sections mapped into memory by the
	 * memory manager.
	 */
	struct htable		mem_table;

	/**
	 * Internal disassembly state.
	 */
	struct disasm_context	context;
};

/**
 * \brief Loads a binary_file object.
 * \param target_path The location of the target to be loaded.
 * \return NULL if a matching BFD backend could not be found. A binary_file
 * object associated with the target otherwise.
 * \note close_binary_file() must be called to allow the object to properly
 * clean up.
 */
extern struct binary_file * load_binary_file(char * target_path);

/**
 * \brief Closes a binary_file object.
 * \param bf The binary file to be closed.
 * \return Returns TRUE if the close occurred successfully, FALSE otherwise.
 */
extern bool close_binary_file(struct binary_file * bf);

/**
 * \brief Builds a control flow graph using the entry point as the root.
 * \param bf The binary file to be analysed.
 * \return The first basic block of the generated CFG.
 * \details The binary_file backend keeps track of all previously analysed
 * instructions. This means there is no need to generate a CFG from the same
 * root more than once.
 */
extern struct bf_basic_blk * disassemble_binary_file_entry(
		struct binary_file * bf);

/**
 * \brief Builds a control flow graph using the address of the symbol as the
 * root.
 * \param bf The binary file to be analysed.
 * \param sym The symbol to start analysis from.
 * \param is_func A bool specifying whether the address of sym should be
 * treated as the start of a function.
 * \return The first basic block of the generated CFG.
 * \details The binary_file backend keeps track of all previously analysed
 * instructions. This means there is no need to generate a CFG from the same
 * root more than once.
 */
extern struct bf_basic_blk * disassemble_binary_file_symbol(
		struct binary_file * bf, asymbol * sym, bool is_func);

#ifdef __cplusplus
}
#endif

#endif
