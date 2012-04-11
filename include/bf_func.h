/**
 * @file bf_func.h
 * @brief Definition and API of bf_func.
 * @details bf_func objects are used to represent discovered functions in a
 * target. A function is recognised during static analysis as anything that
 * is the result of a direct call or was explicitly denoted as a function by
 * the caller of disassemble_binary_file_symbol().
 * @author Mike Kwan <michael.kwan08@imperial.ac.uk>
 */

#ifndef BF_FUNC_H
#define BF_FUNC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "binary_file.h"
#include "bf_sym.h"

/**
 * @struct bf_func
 * @brief <b>libind</b>'s abstraction of a function.
 * @details A function consists of a pointer to the bf_basic_blk at its start
 * address.
 */
struct bf_func {
	/**
	 * @var vma
	 * @brief The VMA that the function starts at.
	 */
	bfd_vma		      vma;

	/**
	 * @internal
	 * @var entry
	 * @brief Entry into the binary_file.func_table hashtable of
	 * binary_file.
	 */
	struct htable_entry   entry;


	/**
	 * @var bb
	 * @brief Basic block at the start of this function.
	 */
	struct bf_basic_blk * bb;

	/*
	 * @var sym
	 * @brief A symbol associated with bf_func.vma.
	 * @note This is the same as bf_func.bb->sym.
	 */
	struct bf_sym *	      sym;
};

/**
 * @internal
 * @brief Creates a new bf_func object.
 * @param bf The binary_file being analysed.
 * @param bb The bf_basic_blk starting at vma.
 * @param vma The VMA of the bf_func/bf_basic_blk.
 * @return A bf_func object.
 * @note bf_close_func must be called to allow the object to properly clean up.
 */
extern struct bf_func * bf_init_func(struct binary_file * bf,
		struct bf_basic_blk * bb, bfd_vma vma);

/**
 * @internal
 * @brief Closes a bf_func object.
 * @param func The bf_func to be closed.
 * @note This will not call bf_close_basic_blk for each bf_basic_blk contained
 * in the bf_func.
 */
extern void bf_close_func(struct bf_func * func);

/**
 * @internal
 * @brief Adds a bf_func to the binary_file.func_table.
 * @param bf The binary_file holding the binary_file.func_table to be added to.
 * @param func The bf_func to be added.
 */
extern void bf_add_func(struct binary_file * bf, struct bf_func * func);

/**
 * @brief Gets the bf_func object for the starting VMA.
 * @param bf The binary_file to be searched.
 * @param vma The VMA of the bf_func being searched for.
 * @return The bf_func starting at vma or NULL if no bf_func has been
 * discovered at that address.
 */
extern struct bf_func * bf_get_func(struct binary_file * bf, bfd_vma vma);

/**
 * @brief Gets the bf_func object with symbol information corresponding to a
 * particular name.
 * @param bf The binary_file to be searched.
 * @param name The name information to be searched for.
 * @return The bf_func corresponding to name or NULL if no bf_func has contains
 * such information.
 */
extern struct bf_func * bf_get_func_from_name(struct binary_file * bf,
		char * name);

/**
 * @brief Checks whether a discovered bf_func exists for a VMA.
 * @param bf The binary_file to be searched.
 * @param vma The VMA of the bf_basic_blk being searched for.
 * @return TRUE if a bf_basic_blk could be found, otherwise FALSE.
 */
extern bool bf_exists_func(struct binary_file * bf, bfd_vma vma);

/**
 * @internal
 * @brief Releases memory for all currently discovered bf_func objects.
 * @param bf The binary_file holding the binary_file.func_table to be purged.
 */
extern void bf_close_func_table(struct binary_file * bf);

/**
 * @brief Invokes a callback for each discovered bf_func.
 * @param bf The binary_file holding the bf_func objects.
 * @param handler The callback to be invoked for each bf_basic_blk.
 * @param param This will be passed to the handler each time it is invoked. It
 * can be used to pass data to the callback.
 */
extern void bf_enum_func(struct binary_file * bf,
		void (*handler)(struct binary_file *, struct bf_func *,
		void *), void * param);

/**
 * @brief Iterate over the bf_func objects of a binary_file.
 * @param func struct bf_func to use as a loop cursor.
 * @param bf struct binary_file holding the bf_func objects.
 */
#define bf_for_each_func(func, bf) \
	struct htable_entry * cur_entry; \
	htable_for_each_entry(func, cur_entry, &bf->func_table, entry)

#ifdef __cplusplus
}
#endif

#endif
