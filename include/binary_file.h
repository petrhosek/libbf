/**
 * @file binary_file.h
 * @brief Definition and API of binary_file.
 * @details binary_file is the file abstraction provided by <b>libind</b>. A typical
 * workflow with <b>libind</b> is to initiate a binary_file object with
 * load_binary_file(), perform CFG generations and finally clean up with
 * close_binary_file(). An API will eventually be added to allow injection
 * of foreign code and patching of the original code.
 * @author Mike Kwan <michael.kwan08@imperial.ac.uk>
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
#include "libase/htable.h"

/**
 * @enum arch_bitiness
 * @brief Enumeration of architecture bitiness.
 * @details Since we support x86-32 and x86-64, having two members is sufficient.
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
 * @internal
 * @struct disasm_context
 * @brief Internal context used by disassembler.
 * @details Allows us to pass in extra information to our custom fprintf
 * function.
 */
struct disasm_context {
	/**
	 * @internal
	 * @var insn
	 * @brief bf_insn to hold information about the instruction being
	 * disassembled.
	 */
	struct bf_insn * insn;

	/**
	 * @internal
	 * @var part_counter
	 * @brief A counter for how many times the fprintf function has been
	 * called for the current instruction. Should be initialised to zero
	 * before disassembly of each instruction.
	 */
	int part_counter;

	/**
	 * @internal
	 * @var has_second_operand
	 * @brief This is set by the disassembler engine if the string it
	 * receives on the third pass is a ',' character.
	 */
	bool has_second_operand;

	/**
	 * @internal
	 * @var is_macro_insn
	 * @brief Denotes whether this instruction is a macro instruction. If
	 * TRUE, then the disassembler will expect the second block instruction
	 * part passed to it to be a mnemonic instead of an operand.
	 */
	bool is_macro_insn;
};

/**
 * @struct binary_file
 * @brief The abstraction used for a binary file.
 * @details This structure encapsulates the information necessary to use
 * <b>libind</b>. Primarily this is our way of wrapping and abstracting away
 * from BFD.
 */
struct binary_file {
	/**
	 * @var abfd
	 * @brief Wrapping the BFD object.
	 * @note This is defined in bfd.h in the binutils distribution.
	 */
	struct bfd *		abfd;

	/**
	 * @var bitiness
	 * @brief Flag denoting the bitiness of the target.
	 */
	enum arch_bitiness	bitiness;

	/**
	 * @internal
	 * @var disassembler
	 * @brief Holds the disassembler corresponding to the BFD object.
	 * @note This is defined in dis-asm.h in the binutils distribution.
	 */
	disassembler_ftype	disassembler;

	/**
	 * @internal
	 * @var disasm_config
	 * @brief Holds the configuration used by <b>libopcodes</b> for
	 * disassembly.
	 * @note This is defined in dis-asm.h in the binutils distribution.
	 */
	struct disassemble_info disasm_config;

	/**
	 * @internal
	 * @var func_table
	 * @brief Hashtable holding list of all the currently discovered bf_func
	 * objects.
	 * @details The implementation is that the address of a function is
	 * its key.
	 */
	struct htable		func_table;

	/**
	 * @internal
	 * @var bb_table
	 * @brief Hashtable holding list of all the currently discovered
	 * bf_basic_blk objects.
	 * @details The implementation is that the address of a basic block
	 * is its key.
	 */
	struct htable		bb_table;

	/**
	 * @internal
	 * @var insn_table
	 * @brief Hashtable holding list of all currently discovered bf_insn
	 * objects.
	 * @details The implementation is that the address of a instruction is
	 * its key.
	 */
	struct htable		insn_table;

	/**
	 * @internal
	 * @var sym_table
	 * @brief Hashtable holding list of all discovered bf_sym objects.
	 * @details The implementation is that the address of a symbol is its
	 * key.
	 */
	struct htable		sym_table;

	/**
	 * @internal
	 * @var mem_table
	 * @brief Hashtable holding mappings of sections mapped into memory by
	 * the memory manager.
	 * @details The implementation is that the address of a section is its
	 * key.
	 */
	struct htable		mem_table;

	/**
	 * @internal
	 * @var context
	 * @brief Internal disassembly state.
	 */
	struct disasm_context	context;
};

/**
 * @brief Loads a binary_file object.
 * @param target_path The location of the target to be loaded.
 * @return NULL if a matching BFD backend could not be found. A binary_file
 * object associated with the target otherwise.
 * @note close_binary_file() must be called to allow the object to properly
 * clean up.
 */
extern struct binary_file * load_binary_file(char * target_path);

/**
 * @brief Closes a binary_file object.
 * @param bf The binary_file to be closed.
 * @return Returns TRUE if the close occurred successfully, FALSE otherwise.
 */
extern bool close_binary_file(struct binary_file * bf);

/**
 * @brief Builds a Control Flow Graph (CFG) using the entry point as the root.
 * @param bf The binary_file being analysed.
 * @return The first basic block of the generated CFG.
 * @details The binary_file backend keeps track of all previously analysed
 * instructions. This means there is no need to generate a CFG from the same
 * root more than once.
 */
extern struct bf_basic_blk * disassemble_binary_file_entry(
		struct binary_file * bf);

/**
 * @brief Builds a Control Flow Graph (CFG) using the address of the symbol as
 * the root.
 * @param bf The binary_file being analysed.
 * @param sym The symbol to start analysis from. This can be obtained using 
 * bf_for_each_sym()
 * @param is_func A bool specifying whether the address of sym should be
 * treated as the start of a function.
 * @return The first basic block of the generated CFG.
 * @details The binary_file backend keeps track of all previously analysed
 * instructions. This means there is no need to generate a CFG from the same
 * root more than once. The reason is_func is required is because there is no
 * reliable heuristic to detect whether a bf_basic_blk represents the start of
 * a function other than it being a call target. Since we can not analyse
 * backwards, we need to be instructed how the root should be treated.
 */
extern struct bf_basic_blk * disassemble_binary_file_symbol(
		struct binary_file * bf, asymbol * sym, bool is_func);

#ifdef __cplusplus
}
#endif

#endif
