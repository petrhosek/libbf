/**
 * @file bf_sym.h
 * @brief Definition and API of bf_sym.
 * @details bf_sym is responsible for storing the symbol table of a BFD in a
 * persistent way. The raison d'etre for this module is because although BFD
 * allows us to fetch the symbol table, we would have to free it in its own
 * specific way. By storing our own copy, we are able to customise the
 * interactions to be consistent with the rest of our API.
 *
 * Another reason for not using BFD directly for symbol access is that BFD
 * provides symbols as a list. Given that we need to constantly access symbols
 * whenever bf_basic_blk and bf_func objects are created, it is more efficient
 * to store the symbols in a hashtable.
 *
 * Internally, the bf_sym module interacts with the binary_file.sym_table.
 * The functions for interacting with this table are not exposed however
 * (they will never be used externally), except for bf_close_sym_table().
 * @author Mike Kwan <michael.kwan08@imperial.ac.uk>
 */

#ifndef BF_SYM_H
#define BF_SYM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "libkern/htable.h"
#include "binary_file.h"

/**
 * @struct bf_sym
 * @brief <b>libind</b>'s abstraction of a symbol.
 * @details Currently a bf_sym consists of an address and a name. This can be
 * easily extended if we need more of the information from the original
 * asymbol structure.
 */
struct bin_file_sym {
	/**
	 * @var vma
	 * @brief VMA of symbol.
	 */
	bfd_vma		    vma;

	/**
	 * @var name
	 * @brief Name of symbol.
	 */
	char *		    name;

	/**
	 * @internal
	 * @var entry
	 * @brief Entry into the binary_file.sym_table hashtable of
	 * binary_file.
	 */
	struct htable_entry entry;
};

/**
 * @internal
 * @brief Load the symbol table in binary_file.
 * @param bf The binary_file to load symbols for.
 * @details This takes care of the initial load of symbols and the copying of
 * them into our own structures.
 */
extern void load_sym_table(struct bin_file * bf);

/**
 * @brief Gets the bf_sym for the VMA.
 * @param bf The binary_file to be searched.
 * @param vma The VMA of the bf_sym being searched for.
 * @return The bf_sym starting at vma or NULL if no bf_sym has been discovered
 * at that address.
 */
extern struct bin_file_sym * bf_get_sym(struct bin_file * bf, bfd_vma vma);

/**
 * @brief Checks whether a discovered bf_sym exists for a VMA.
 * @param bf The binary_file to be searched.
 * @param vma The VMA of the bf_sym being searched for.
 * @return TRUE if a bf_sym could be found, otherwise FALSE.
 */
extern bool bin_file_sym_exists(struct bin_file * bf, bfd_vma vma);

/**
 * @internal
 * @brief Releases memory for all currently discovered bf_sym objects.
 * @param bf The binary_file holding the binary_file.sym_table to be purged.
 */
extern void close_sym_table(struct bin_file * bf);

/**
 * @brief Invokes a callback for each discovered bf_sym.
 * @param bf The binary_file holding the bf_sym objects.
 * @param handler The callback to be invoked for each bf_sym.
 */
extern bool bf_enum_symbol(struct bin_file * bf,
		void (*handler)(struct bin_file *, asymbol *, void *),
		void * param);

#ifdef __cplusplus
}
#endif

#endif
