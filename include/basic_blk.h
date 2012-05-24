/**
 * @file basic_blk.h
 * @brief Definition and API of bf_basic_blk.
 * @details bf_basic_blk objects are used to represent the control flow of the
 * target. Each node of a generated control flow graph (<b>CFG</b>) consists of a
 * bf_basic_blk. A CFG can be traversed either:
 *	- Manually through its starting basic block
 * 	- Through visitor callbacks such as bf_for_each_basic_blk()
 *	- Through the CFG navigation APIs provided by bf_cfg.h (for
 * displaying or outputting the CFG)
 *
 * The static analysis performed by <b>libind</b> currently ignores indirect
 * calls. This means that even if a user disassembles from the entry point,
 * there is a chance that not all code is reachable. Hence, a bin_file can
 * hold multiple unconnected CFGs generated from disassembling at different
 * roots.
 * @author Mike Kwan <michael.kwan08@imperial.ac.uk>
 */

#ifndef BF_BASIC_BLK_H
#define BF_BASIC_BLK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#include <libkern/list.h>

#include "insn.h"
#include "symbol.h"

/**
 * @internal
 * @struct bf_basic_blk_part
 * @brief The constituent unit composing a bf_basic_block.
 * @details A bf_basic_blk_part represents a list of bf_insn objects.
 */
struct basic_blk_part {
  /**
   * @internal
   * @var list
   * @brief The list node which is linked onto bf_basic_blk.part_list.
   */
  struct list_head list;

  /**
   * @internal
   * @var insn
   * @brief The bf_insn being encapsulated by this basic_blk_part.
   */
  struct bf_insn * insn;
};

/**
 * @struct bf_basic_blk
 * @brief <b>libind</b>'s abstraction of a basic block.
 * @details A bf_basic_blk consists of a list of its constituent instructions
 * (bf_insn objects).
 */
struct basic_blk {
  /**
   * @var vma
   * @brief The starting VMA of the basic block.
   */
  bfd_vma vma;

  /**
   * @internal
   * @var part_list
   * @brief Start of linked list of parts (instructions/bf_insn objects).
   */
  struct list_head part_list;

  /**
   * @internal
   * @var entry
   * @brief Entry into the bin_file.bb_table hashtable of bin_file.
   */
  struct htable_entry entry;

  /**
   * @var target
   * @brief The next basic block in the CFG.
   * @details This is the basic block linearly subsequent to the current
   * basic block. The only exception to this rule is where the last
   * instruction of the current basic block is an unconditional branch.
   * In this case, bf_basic_blk.target is the basic block of the branch
   * target.
   */
  struct basic_blk * target;

  /**
   * @var target2
   * @brief The next basic block in the CFG.
   * @details If more than one basic block is reachable from the current
   * one (e.g. through conditional branching or function call),
   * bf_basic_blk.target2 points to basic block of the branch or call
   * target. bf_basic_blk.target2 is populated only if
   * bf_basic_blk.target has already been assigned a value.
   */
  struct basic_blk * target2;

  /**
   * @var sym
   * @brief A bf_sym associated with basic_blk.vma.
   */
  struct symbol * sym;
};

/**
 * @internal
 * @brief Creates a new bf_basic_blk object.
 * @param bf The bin_file being analysed.
 * @param vma The starting address of the basic block.
 * @return A bf_basic_blk object.
 * @note bf_close_basic_blk must be called to allow the object to properly
 * clean up.
 */
extern struct basic_blk * bf_init_basic_blk(struct bin_file * bf,
    bfd_vma vma);

/**
 * @internal
 * @brief Splits a bf_basic_blk at the specified VMA.
 * @param bf The bin_file being analysed.
 * @param bb The bf_basic_blk to be split.
 * @param vma The VMA where the split should occur.
 * @return The new basic_blk starting at vma.
 */
extern struct basic_blk * bf_split_blk(struct bin_file * bf,
    struct basic_blk * bb, bfd_vma vma);

/**
 * @internal
 * @brief Creates a link between two bf_basic_blk objects.
 * @param bb The bf_basic_blk the link starts from.
 * @param bb2 The basic_blk the link goes to.
 */
extern void bf_add_next_basic_blk(struct basic_blk * bb,
    struct basic_blk * bb2);

/**
 * @internal
 * @brief Appends to the tail of the instruction list.
 * @param bb The bf_basic_blk whose instruction list is to be appended to.
 * @param insn The instruction to append.
 */
extern void bf_add_insn_to_bb(struct basic_blk * bb, struct bf_insn * insn);

/**
 * @brief Prints the bf_basic_blk to stdout.
 * @param bb The bf_basic_blk to be printed.
 * @details This function would generally be used for debug or demonstration
 * purposes since the format of the output is not easily customisable. The
 * bf_basic_blk should be manually printed if customised output is desired.
 */
extern void bf_print_basic_blk(struct basic_blk * bb);

/**
 * @internal
 * @brief Prints the bf_basic_blk to a FILE in dot format.
 * @param stream An open FILE to be written to.
 * @param bb The basic_blk to be written.
 */
extern void bf_print_basic_blk_dot(FILE * stream, struct basic_blk * bb);

/**
 * @internal
 * @brief Closes a bf_basic_blk object.
 * @param bb The bf_basic_blk to be closed.
 * @note This will not call bf_close_insn for each bf_insn contained in the
 * basic_blk.
 */
extern void bf_close_basic_blk(struct basic_blk * bb);

/**
 * @internal
 * @brief Adds a bf_basic_blk to the bin_file.bb_table.
 * @param bf The bin_file holding the bin_file.bb_table to be added to.
 * @param bb The basic_blk to be added.
 */
extern void bf_add_bb(struct bin_file * bf, struct basic_blk * bb);

/**
 * @brief Gets the bf_basic_blk object for the starting VMA.
 * @param bf The bin_file to be searched.
 * @param vma The VMA of the bf_basic_blk being searched for.
 * @return The bf_basic_blk starting at vma or NULL if no bf_basic_blk has been
 * discovered at that address.
 */
extern struct basic_blk * bf_get_bb(struct bin_file * bf, bfd_vma vma);

/**
 * @brief Gets the size in bytes of a bf_basic_blk object.
 * @param bb The bf_basic_blk to get the size of.
 * @return The size in bytes of bb.
 */
extern unsigned int bf_get_bb_size(struct bin_file * bf,
		struct basic_blk * bb);

/**
 * @brief Gets the length in bf_insn objects of a bf_basic_blk object.
 * @param bb The bf_basic_blk to get the length of.
 * @return The number of bf_insn objects held in bb.
 */
extern unsigned int bf_get_bb_length(struct bin_file * bf,
		struct basic_blk * bb);

/**
 * @brief Gets the instruction at index of the bf_basic_blk object.
 * @param bf The bin_file to be searched.
 * @param bb The bf_basic_blk to examine.
 * @param index The index into the bf_basic_blk to examine.
 * @return The bf_insn at index or NULL if the index is invalid.
 */
extern struct bf_insn * bf_get_bb_insn(struct bin_file * bf,
		struct basic_blk * bb, unsigned int index);

/**
 * @brief Checks whether a discovered bf_basic_blk exists for a VMA.
 * @param bf The bin_file to be searched.
 * @param vma The VMA of the bf_basic_blk being searched for.
 * @return TRUE if a bf_basic_blk could be found, otherwise FALSE.
 */
extern bool bf_exists_bb(struct bin_file * bf, bfd_vma vma);

/**
 * @internal
 * @brief Releases memory for all currently discovered bf_basic_blk objects.
 * @param bf The bin_file holding the bin_file.bb_table to be purged.
 */
extern void bf_close_bb_table(struct bin_file * bf);

/**
 * @brief Invokes a callback for each discovered bf_basic_blk.
 * @param bf The bin_file holding the bf_basic_blk objects.
 * @param handler The callback to be invoked for each bf_basic_blk.
 * @param param This will be passed to the handler each time it is invoked. It
 * can be used to pass data to the callback.
 */
extern void bf_enum_basic_blk(struct bin_file * bf,
    void (*handler)(struct bin_file *, struct basic_blk *, void *),
    void * param);

/**
 * @brief Iterate over the bf_basic_blk objects of a bin_file.
 * @param bb struct bf_basic_blk to use as a loop cursor.
 * @param bf struct bin_file holding the bf_basic_blk objects.
 */
#define bf_for_each_basic_blk(bb, bf) \
	struct htable_entry * cur_entry; \
	htable_for_each_entry(bb, cur_entry, &bf->bb_table, entry)

/**
 * @brief Invokes a callback for each bf_insn in a bf_basic_blk.
 * @param bb The bf_basic_blk being analysed.
 * @param handler The callback to be invoked for each bf_insn.
 * @param param This will be passed to the handler each time it is invoked. It
 * can be used to pass data to the callback.
 */
extern void bf_enum_basic_blk_insn(struct basic_blk * bb,
    void (*handler)(struct basic_blk *, struct bf_insn *, void *),
    void * param);

/**
 * @brief Iterate over the bf_insn objects of a bf_basic_blk.
 * @param insn struct bf_insn to use as a loop cursor.
 * @param bb struct basic_blk holding the bf_insn objects.
 */
#define bf_for_each_basic_blk_insn(insn, bb) \
	struct basic_blk_part * pos; \
	list_for_each_entry(pos, &bb->part_list, list) \
		if((insn = pos->insn))

#ifdef __cplusplus
}
#endif

#endif
