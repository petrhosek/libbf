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

#include "libkern/htable.h"

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
 * @enum insn_part_type
 * @brief Enumeration of the different instruction parts we expect.
 * @details The disassembler engine sets a combination of expected part types
 * as it disassembles. If it receives a type it was not expecting, it can
 * output this.
 */
enum insn_part_type {
  /**
   * Enum value for mnemonic.
   */
  insn_part_mnemonic = 1,
  /**
   * Enum value for secondary mnemonic.
   */
  insn_part_secondary_mnemonic = 2,
  /**
   * Enum value for operand.
   */
  insn_part_operand = 4,
  /**
   * Enum value for comma.
   */
  insn_part_comma = 8,
  /**
   * Enum value for comment indicator.
   */
  insn_part_comment_indicator = 16,
  /**
   * Enum value for comment contents.
   */
  insn_part_comment_contents = 32
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
   * called for the current instruction. Should be initialised to 0
   * before disassembly of each instruction.
   */
  int part_counter;

  /**
   * @internal
   * @var part_types_expected
   * @brief Holds a combination of the insn_part_type flags. Should be
   * initialised to insn_part_mnemonic before disassembly of each
   * instruction.
   */
  int part_types_expected;
};

/**
 * @struct bin_file
 * @brief The abstraction used for a binary file.
 * @details This structure encapsulates the information necessary to use
 * <b>libind</b>. Primarily this is our way of wrapping and abstracting away
 * from BFD.
 */
struct bin_file {
  /**
   * @var abfd
   * @brief Wrapping the BFD object.
   * @note This is defined in bfd.h in the binutils distribution.
   */
  struct bfd * abfd;

  /**
   * @var obfd
   * @brief Wraps the output BFD object, if any.
   * @note This is defined in bfd.h in the binutils distribution.
   */
  struct bfd * obfd;

  /**
   * @var bitiness
   * @brief Flag denoting the bitiness of the target.
   */
  enum arch_bitiness bitiness;

  /**
   * @internal
   * @var disassembler
   * @brief Holds the disassembler corresponding to the BFD object.
   * @note This is defined in dis-asm.h in the binutils distribution.
   */
  disassembler_ftype disassembler;

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
  struct htable func_table;

  /**
   * @internal
   * @var bb_table
   * @brief Hashtable holding list of all the currently discovered
   * bf_basic_blk objects.
   * @details The implementation is that the address of a basic block
   * is its key.
   */
  struct htable bb_table;

  /**
   * @internal
   * @var insn_table
   * @brief Hashtable holding list of all currently discovered bf_insn
   * objects.
   * @details The implementation is that the address of a instruction is
   * its key.
   */
  struct htable insn_table;

  /**
   * @internal
   * @var sym_table
   * @brief Hashtable holding list of all discovered bf_sym objects.
   * @details The implementation is that the address of a symbol is its
   * key.
   */
  struct htable sym_table;

  /**
   * @internal
   * @var mem_table
   * @brief Hashtable holding mappings of sections mapped into memory by
   * the memory manager.
   * @details The implementation is that the address of a section is its
   * key.
   */
  struct htable mem_table;

  /**
   * @internal
   * @var context
   * @brief Internal disassembly state.
   */
  struct disasm_context context;
};

/**
 * @brief Loads a binary_file object.
 * @param target_path The location of the target to be loaded.
 * @param output_path The location of the output binary_file. Any changes made
 * by <b>libind</b> will not modify the original file. Instead an edited copy
 * is saved to output_path. If output_path is NULL, the binary_file returned is
 * read-only.
 * @return NULL if a matching BFD backend could not be found. A binary_file
 * object associated with the target otherwise.
 * @note close_binary_file() must be called to allow the object to properly
 * clean up.
 */
extern struct bin_file * load_binary_file(char * target_path,
    char * output_path);

/**
 * @brief Closes a binary_file object.
 * @param bf The binary_file to be closed.
 * @return Returns TRUE if the close occurred successfully, FALSE otherwise.
 */
extern bool close_binary_file(struct bin_file * bf);

/**
 * @brief Builds a Control Flow Graph (CFG) using the entry point as the root.
 * @param bf The binary_file being analysed.
 * @return The first basic block of the generated CFG.
 * @details The binary_file backend keeps track of all previously analysed
 * instructions. This means there is no need to generate a CFG from the same
 * root more than once.
 */
extern struct basic_blk * disassemble_binary_file_entry(struct bin_file * bf);

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
extern struct basic_blk * disassemble_binary_file_symbol(struct bin_file * bf,
    asymbol * sym, bool is_func);

#ifdef __cplusplus
}
#endif

#endif
