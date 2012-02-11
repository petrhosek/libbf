#include "bf_insn.h"
#include "bf_basic_blk.h"

struct bf_basic_blk * init_bf_basic_blk(bfd_vma vma)
{
	struct bf_basic_blk * bb = xmalloc(sizeof(struct bf_basic_blk));
	bb->vma			 = vma;
	bb->target		 = NULL;
	bb->target2		 = NULL;

	INIT_LIST_HEAD(&bb->part_list);
	return bb;
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

void add_insn(struct bf_basic_blk * bb, struct bf_insn * insn)
{
	struct bf_basic_blk_part * part =
			xmalloc(sizeof(struct bf_basic_blk_part));
	part->insn			= insn;

	INIT_LIST_HEAD(&part->list);
	list_add_tail(&part->list, &bb->part_list);
}

void print_bf_basic_blk(struct bf_basic_blk * bb)
{
	if(bb != NULL) {
		struct bf_basic_blk_part * pos;

		list_for_each_entry(pos, &bb->part_list, list) {
			printf("\t");
			print_bf_insn(pos->insn);
			printf("\n");
		}
	}
}

void print_bf_basic_blk_dot(FILE * stream, struct bf_basic_blk * bb)
{
	if(bb != NULL) {
		struct bf_basic_blk_part * pos;

		list_for_each_entry(pos, &bb->part_list, list) {
			print_bf_insn_dot(stream, pos->insn);
		}
	}
}

void close_bf_basic_blk(struct bf_basic_blk * bb)
{
	if(bb != NULL) {
		struct bf_basic_blk_part * pos;
		struct bf_basic_blk_part * n;

		list_for_each_entry_safe(pos, n, &bb->part_list, list) {
			list_del(&pos->list);
			close_bf_insn(pos->insn);
			free(pos);
		}

		free(bb);
	}
}

void add_bb(struct binary_file * bf, struct bf_basic_blk * bb)
{
	assert(!exists_bb(bf, bb->vma));

	htable_add(&bf->bb_table, &bb->entry, &bb->vma, sizeof(bb->vma));
}

struct bf_basic_blk * get_bb(struct binary_file * bf, bfd_vma vma)
{
	struct htable_entry * entry = htable_find(&bf->bb_table, &vma,
			sizeof(vma));

	if(entry == NULL) {
		return NULL;
	}

	return hash_entry(entry, struct bf_basic_blk, entry);
}

bool exists_bb(struct binary_file * bf, bfd_vma vma)
{
	return htable_find(&bf->bb_table, &vma, sizeof(vma));
}
