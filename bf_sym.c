#include "bf_sym.h"

void populate_sym_table(struct binary_file * bf, asymbol * sym)
{
	struct bf_sym * entry = xmalloc(sizeof(struct bf_sym));
	symbol_info	info;

	bfd_symbol_info(sym, &info);

	entry->name = xstrdup(sym->name);
	entry->vma  = info.value;
	htable_add(&bf->sym_table, &entry->entry, &entry->vma,
			sizeof(entry->vma));
}

void load_sym_table(struct binary_file * bf)
{
	bf_for_each_symbol(bf, populate_sym_table);
}

struct bf_sym * bf_get_sym(struct binary_file * bf, bfd_vma vma)
{
	return hash_find_entry(&bf->sym_table, &vma, sizeof(vma),
			struct bf_sym, entry);
}

bool bf_exists_sym(struct binary_file * bf, bfd_vma vma)
{
	return htable_find(&bf->sym_table, &vma, sizeof(vma));
}

void bf_close_sym_table(struct binary_file * bf)
{
	struct htable_entry * cur_entry;
	struct htable_entry * n;
	struct bf_sym *	      sym;

	htable_for_each_entry_safe(sym, cur_entry, n, &bf->sym_table, entry) {
		htable_del_entry(&bf->sym_table, cur_entry);
		free(sym->name);
		free(sym);
	}
}

bool bf_for_each_symbol(struct binary_file * bf,
		void (*handler)(struct binary_file * bf, asymbol *))
{
	bfd * abfd 	     = bf->abfd;
	long  storage_needed = bfd_get_symtab_upper_bound(abfd);

	if(storage_needed < 0) {
		return FALSE;
	} else if(storage_needed == 0) {
		return TRUE;
	} else {
		asymbol **symbol_table    = xmalloc(storage_needed);
		long    number_of_symbols = bfd_canonicalize_symtab(abfd, symbol_table);

		if(number_of_symbols < 0) {
			free(symbol_table);
			return FALSE;
		} else {
			for(long i = 0; i < number_of_symbols; i++) {
				handler(bf, symbol_table[i]);
			}
		}

		free(symbol_table);
		return TRUE;
	}
}
