#include "bf_func.h"

struct bf_func * bf_init_func(struct bf_basic_blk * bb, bfd_vma vma)
{
	struct bf_func * func = xmalloc(sizeof(struct bf_func));
	func->bb	      = bb;
	func->vma	      = vma;

	return func;
}

void bf_close_func(struct bf_func * func)
{
	if(func != NULL) {
		free(func);
	}
}

void bf_add_func(struct binary_file * bf, struct bf_func * func)
{
	assert(!bf_exists_func(bf, func->vma));

	htable_add(&bf->func_table, &func->entry, &func->vma, sizeof(func->vma));
}

struct bf_func * bf_get_func(struct binary_file * bf, bfd_vma vma)
{
	return hash_find_entry(&bf->func_table, &vma, sizeof(vma),
			struct bf_func, entry);
}

bool bf_exists_func(struct binary_file * bf, bfd_vma vma)
{
	return htable_find(&bf->func_table, &vma, sizeof(vma));
}

void bf_close_func_table(struct binary_file * bf)
{
	struct htable_entry * cur_entry;
	struct htable_entry * n;
	struct bf_func *      func;

	htable_for_each_entry_safe(func, cur_entry, n, &bf->func_table, entry) {
		htable_del_entry(&bf->func_table, cur_entry);
		bf_close_func(func);
	}
}
