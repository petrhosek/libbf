#include "bf_insn.h"
#include "bf_basic_blk.h"

struct basic_blk * bf_init_basic_blk(struct bin_file * bf, bfd_vma vma)
{
	struct basic_blk * bb = xmalloc(sizeof(struct basic_blk));
	bb->vma			 = vma;
	bb->target		 = NULL;
	bb->target2		 = NULL;
	bb->sym			 = rb_search_symbol(&bf->sym_table, (void *)vma);

	INIT_LIST_HEAD(&bb->part_list);
	return bb;
}

struct basic_blk * bf_split_blk(struct bin_file * bf,
		struct basic_blk * bb, bfd_vma vma)
{
	struct basic_blk *	bb_new = bf_init_basic_blk(bf, vma);
	struct basic_blk_part * pos;
	struct basic_blk_part * n;
	
	list_for_each_entry_safe(pos, n, &bb->part_list, list) {
		struct bf_insn * insn = pos->insn;
		if(insn->vma >= vma) {
			list_del(&pos->list);
			bf_add_insn_to_bb(bb_new, insn);
			insn->bb = bb_new;
		}
	}

	return bb_new;
}

void bf_add_next_basic_blk(struct basic_blk * bb, struct basic_blk * bb2)
{
	assert(bb && bb2 && (!bb->target || !bb->target2));

	if(bb->target == NULL) {
		bb->target  = bb2;
	} else if(bb->target2 == NULL) {
		bb->target2 = bb2;
	}
}

void bf_add_insn_to_bb(struct basic_blk * bb, struct bf_insn * insn)
{
	struct basic_blk_part * part =
			xmalloc(sizeof(struct basic_blk_part));
	part->insn		     = insn;

	INIT_LIST_HEAD(&part->list);
	list_add_tail(&part->list, &bb->part_list);
}

void bf_print_basic_blk(struct basic_blk * bb)
{
	if(bb != NULL) {
		struct basic_blk_part * pos;

		list_for_each_entry(pos, &bb->part_list, list) {
			printf("\t");
			bf_print_insn(pos->insn);
			printf("\n");
		}
	}
}

void bf_print_basic_blk_dot(FILE * stream, struct basic_blk * bb)
{
	if(bb != NULL) {
		struct basic_blk_part * pos;

		list_for_each_entry(pos, &bb->part_list, list) {
			bf_print_insn_dot(stream, pos->insn);
		}
	}
}

void bf_close_basic_blk(struct basic_blk * bb)
{
	if(bb != NULL) {
		struct basic_blk_part * pos;
		struct basic_blk_part * n;

		list_for_each_entry_safe(pos, n, &bb->part_list, list) {
			list_del(&pos->list);
			free(pos);
		}

		free(bb);
	}
}

void bf_add_bb(struct bin_file * bf, struct basic_blk * bb)
{
	assert(!bf_exists_bb(bf, bb->vma));

	htable_add(&bf->bb_table, &bb->entry, &bb->vma, sizeof(bb->vma));
}

struct basic_blk * bf_get_bb(struct bin_file * bf, bfd_vma vma)
{
	return hash_find_entry(&bf->bb_table, &vma, sizeof(vma),
			struct basic_blk, entry);
}

int bf_get_bb_size(struct bin_file * bf, struct basic_blk * bb)
{
	struct bf_insn * insn;
	int		 size = 0;

	bf_for_each_insn(insn, bf) {
		size += insn->size;
	}

	return size;
}

bool bf_exists_bb(struct bin_file * bf, bfd_vma vma)
{
	return htable_find(&bf->bb_table, &vma, sizeof(vma));
}

void bf_close_bb_table(struct bin_file * bf)
{
	struct htable_entry * cur_entry;
	struct htable_entry * n;
	struct basic_blk * bb;

	htable_for_each_entry_safe(bb, cur_entry, n, &bf->bb_table, entry) {
		htable_del_entry(&bf->bb_table, cur_entry);
		bf_close_basic_blk(bb);
	}
}

void bf_enum_basic_blk(struct bin_file * bf,
		void (*handler)(struct bin_file *, struct basic_blk *,
		void * param), void * param)
{
	struct htable_entry * cur_entry;
	struct basic_blk * bb;

	htable_for_each_entry(bb, cur_entry, &bf->bb_table, entry) {
		handler(bf, bb, param);
	}
}

void bf_enum_basic_blk_insn(struct basic_blk * bb,
		void (*handler)(struct basic_blk *, struct bf_insn *,
		void * param), void * param)
{
	struct basic_blk_part * pos;

	list_for_each_entry(pos, &bb->part_list, list) {
		handler(bb, pos->insn, param);
	}
}
