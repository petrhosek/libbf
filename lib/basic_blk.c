#include "insn.h"
#include "basic_blk.h"

struct bf_basic_blk * bf_init_basic_blk(struct bin_file * bf, bfd_vma vma)
{
	struct bf_basic_blk * bb = xmalloc(sizeof(struct bf_basic_blk));
	bb->vma			 = vma;
	bb->target		 = NULL;
	bb->target2		 = NULL;
	bb->sym			 = rb_search_symbol(&bf->sym_table,
			(void *)vma);
	bb->insn_vec		 = NULL;

	vec_init(bb->insn_vec, 100);
	return bb;
}

struct bf_basic_blk * bf_split_blk(struct bin_file * bf,
		struct bf_basic_blk * bb, bfd_vma vma)
{
	struct bf_basic_blk * bb_new = bf_init_basic_blk(bf, vma);

	for(int i = 0; i < vec_size(bb->insn_vec); i++) {
		struct bf_insn * insn = bb->insn_vec[i];

		if(insn->vma >= vma) {
			vec_erase(bb->insn_vec, i);
			bf_add_insn_to_bb(bb_new, insn);
			insn->bb = bb_new;
			i--;
		}
	}

	return bb_new;
}

void bf_add_next_basic_blk(struct bf_basic_blk * bb, struct bf_basic_blk * bb2)
{
	assert(bb && bb2 && (!bb->target || !bb->target2));

	if(bb->target == NULL) {
		bb->target  = bb2;
	} else if(bb->target2 == NULL) {
		bb->target2 = bb2;
	}
}

void bf_add_insn_to_bb(struct bf_basic_blk * bb, struct bf_insn * insn)
{
	vec_push(bb->insn_vec, insn);
}

void bf_print_basic_blk(struct bf_basic_blk * bb)
{
	if(bb != NULL) {
		int len = bf_get_bb_length(bb);

		for(int i = 0; i < len; i++) {
			printf("\t");
			bf_print_insn(bb->insn_vec[i]);
			printf("\n");
		}
	}
}

void bf_print_basic_blk_dot(FILE * stream, struct bf_basic_blk * bb)
{
	if(bb != NULL) {
		int len = bf_get_bb_length(bb);

		for(int i = 0; i < len; i++) {
			bf_print_insn_dot(stream, bb->insn_vec[i]);
		}
	}
}

void bf_close_basic_blk(struct bf_basic_blk * bb)
{
	if(bb != NULL) {
		vec_destroy(bb->insn_vec);
		free(bb);
	}
}

void bf_add_bb(struct bin_file * bf, struct bf_basic_blk * bb)
{
	assert(!bf_exists_bb(bf, bb->vma));

	htable_add(&bf->bb_table, &bb->entry, &bb->vma, sizeof(bb->vma));
}

struct bf_basic_blk * bf_get_bb(struct bin_file * bf, bfd_vma vma)
{
	return hash_find_entry(&bf->bb_table, &vma, sizeof(vma),
			struct bf_basic_blk, entry);
}

unsigned int bf_get_bb_size(struct bf_basic_blk * bb)
{
	unsigned int size = 0;
	int	     len  = bf_get_bb_length(bb);

	for(int i = 0; i < len; i++) {
		size += bb->insn_vec[i]->size;
	}

	return size;
}

unsigned int bf_get_bb_length(struct bf_basic_blk * bb)
{
	return vec_size(bb->insn_vec);
}

struct bf_insn * bf_get_bb_insn(struct bf_basic_blk * bb, unsigned int index)
{
	if(index >= bf_get_bb_size(bb)) {
		return NULL;
	} else {
		return bb->insn_vec[index];
	}
}

bool bf_exists_bb(struct bin_file * bf, bfd_vma vma)
{
	return htable_find(&bf->bb_table, &vma, sizeof(vma));
}

void bf_close_bb_table(struct bin_file * bf)
{
	struct htable_entry * cur_entry;
	struct htable_entry * n;
	struct bf_basic_blk * bb;

	htable_for_each_entry_safe(bb, cur_entry, n, &bf->bb_table, entry) {
		htable_del_entry(&bf->bb_table, cur_entry);
		bf_close_basic_blk(bb);
	}
}

void bf_enum_basic_blk(struct bin_file * bf,
		void (*handler)(struct bin_file *, struct bf_basic_blk *,
		void * param), void * param)
{
	struct htable_entry * cur_entry;
	struct bf_basic_blk * bb;

	htable_for_each_entry(bb, cur_entry, &bf->bb_table, entry) {
		handler(bf, bb, param);
	}
}

void bf_enum_basic_blk_insn(struct bf_basic_blk * bb,
		void (*handler)(struct bf_basic_blk *, struct bf_insn *,
		void * param), void * param)
{
	int len = bf_get_bb_length(bb);

	for(int i = 0; i < len; i++) {
		handler(bb, bb->insn_vec[i], param);
	}
}
