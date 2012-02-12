#include "bf_sym_tab.h"

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
	binary_file_for_each_symbol(bf, populate_sym_table);
}

struct bf_sym * bf_get_sym(struct binary_file * bf, bfd_vma vma)
{
	struct htable_entry * entry = htable_find(&bf->sym_table, &vma,
			sizeof(vma));
	
	if(entry == NULL) {
		return NULL;
	}

	return hash_entry(entry, struct bf_sym, entry);
}

void close_sym_table(struct binary_file * bf)
{
	struct htable_entry * cur_entry;
	struct htable_entry * n;

	htable_for_each_safe(n, cur_entry, &bf->sym_table, node) {
		struct bf_sym * sym = hash_entry(cur_entry,
				struct bf_sym, entry);

		htable_del_entry(&bf->sym_table, cur_entry);
		free(sym->name);
		free(sym);
	}
}
