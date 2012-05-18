#include "bf_sym.h"

void populate_sym_table(struct bin_file * bf, asymbol * sym, void * param)
{
	struct bin_file_sym * entry = xmalloc(sizeof(struct bin_file_sym));
	symbol_info	info;

	bfd_symbol_info(sym, &info);

	entry->name = xstrdup(sym->name);
	entry->vma  = info.value;
	htable_add(&bf->sym_table, &entry->entry, &entry->vma,
			sizeof(entry->vma));
}

void load_sym_table(struct bin_file * bf)
{
	bf_enum_symbol(bf, populate_sym_table, NULL);
}

struct bin_file_sym * bf_get_sym(struct bin_file * bf, bfd_vma vma)
{
	return hash_find_entry(&bf->sym_table, &vma, sizeof(vma),
			struct bin_file_sym, entry);
}

bool bin_file_sym_exists(struct bin_file * bf, bfd_vma vma)
{
	return htable_find(&bf->sym_table, &vma, sizeof(vma));
}

void close_sym_table(struct bin_file * bf)
{
	struct htable_entry * cur_entry;
	struct htable_entry * n;
	struct bin_file_sym *	      sym;

	htable_for_each_entry_safe(sym, cur_entry, n, &bf->sym_table, entry) {
		htable_del_entry(&bf->sym_table, cur_entry);
		free(sym->name);
		free(sym);
	}
}

bool bf_enum_symbol(struct bin_file * bf,
		void (*handler)(struct bin_file * bf, asymbol *, void *),
		void * param)
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
				handler(bf, symbol_table[i], param);
			}
		}

		free(symbol_table);
		return TRUE;
	}
}
