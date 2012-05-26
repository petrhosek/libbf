#include "func.h"

struct bf_func * bf_init_func(struct bin_file * bf,
		struct bf_basic_blk * bb, bfd_vma vma)
{
	struct bf_func * func = xmalloc(sizeof(struct bf_func));
	func->bb	      = bb;
	func->vma	      = vma;
	func->sym	      = rb_search_symbol(&bf->sym_table, (void *)vma);
	return func;
}

void bf_close_func(struct bf_func * func)
{
	if(func != NULL) {
		free(func);
	}
}

void bf_add_func(struct bin_file * bf, struct bf_func * func)
{
	assert(!bf_exists_func(bf, func->vma));

	htable_add(&bf->func_table, &func->entry, &func->vma,
			sizeof(func->vma));
}

struct bf_func * bf_get_func(struct bin_file * bf, bfd_vma vma)
{
	return hash_find_entry(&bf->func_table, &vma, sizeof(vma),
			struct bf_func, entry);
}

struct BF_FUNC_INFO {
	struct bf_func * func;
	char *		 name;
};

static void func_from_name(struct bin_file * bf, struct bf_func * func,
		void * param)
{
	struct BF_FUNC_INFO * info = param;

	if(func->sym != NULL) {
		if(strcmp(func->sym->name, info->name) == 0) {
			info->func = func;
		}
	}
}

struct bf_func * bf_get_func_from_name(struct bin_file * bf, char * name)
{
	struct BF_FUNC_INFO info;
	info.name = name;
	info.func = NULL;

	bf_enum_func(bf, func_from_name, &info);
	return info.func;
}

bool bf_exists_func(struct bin_file * bf, bfd_vma vma)
{
	return htable_find(&bf->func_table, &vma, sizeof(vma));
}

void bf_close_func_table(struct bin_file * bf)
{
	struct htable_entry * cur_entry;
	struct htable_entry * n;
	struct bf_func *      func;

	htable_for_each_entry_safe(func, cur_entry, n, &bf->func_table,
			entry) {
		htable_del_entry(&bf->func_table, cur_entry);
		bf_close_func(func);
	}
}

void bf_enum_func(struct bin_file * bf,
		void (*handler)(struct bin_file *, struct bf_func *,
		void *), void * param)
{
	struct htable_entry * cur_entry;
	struct bf_func *      func;

	htable_for_each_entry(func, cur_entry, &bf->func_table, entry) {
		handler(bf, func, param);
	}
}
