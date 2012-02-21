#ifndef BF_SYM_TAB_H
#define BF_SYM_TAB_H

#ifdef __cplusplus
extern "C" {
#endif

#include "include/htable.h"
#include "binary_file.h"

/*
 * BFD provides access to a symbol table as a list. We can use it more usefully
 * as a hashtable, which is the abstraction this module provides.
 */

/*
 * Our abstraction of a symbol. For now we store only the address and name.
 * This can easily be extended if required.
 */
struct bf_sym {
	/*
	 * VMA of symbol.
	 */
	bfd_vma		    vma;

	/*
	 * Name of symbol.
	 */
	char *		    name;

	/*
	 * Entry into the hashtable of binary_file.
	 */
	struct htable_entry entry;
};

/*
 * Load the symbol table in binary_file.
 */
extern void load_sym_table(struct binary_file *);

/*
 * Returns the symbol associated with a particular address.
 */
extern struct bf_sym * bf_get_sym(struct binary_file *, bfd_vma);

/*
 * Releases resources held by the symbol table.
 */
extern void bf_close_sym_table(struct binary_file *);

/*
 * Specify a callback which is invoked for each discovered symbol.
 */
extern bool bf_for_each_symbol(struct binary_file *,
		void (*)(struct binary_file *, asymbol *));

#ifdef __cplusplus
}
#endif

#endif
