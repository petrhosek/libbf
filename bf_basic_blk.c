#include "bf_insn.h"
#include "bf_basic_blk.h"

bf_basic_blk * init_bf_basic_blk(bfd_vma vma)
{
	bf_basic_blk * bb = xmalloc(sizeof(bf_basic_blk));
	bb->vma		  = vma;

	INIT_LIST_HEAD(&bb->part_list);
	return bb;
}

void add_insn(bf_basic_blk * bb, bf_insn * insn)
{
	bf_basic_blk_part * part = xmalloc(sizeof(bf_basic_blk_part));
	part->insn		 = insn;

	INIT_LIST_HEAD(&part->list);
	list_add_tail(&part->list, &bb->part_list);
}

void print_bf_basic_blk(bf_basic_blk * bb)
{
	if(bb != NULL) {
		bf_basic_blk_part * pos;

		list_for_each_entry(pos, &bb->part_list, list) {
			print_bf_insn(pos->insn);
			printf("\n");
		}
	}
}

void close_bf_basic_blk(bf_basic_blk * bb)
{
	if(bb != NULL) {
		bf_basic_blk_part * pos;
		bf_basic_blk_part * n;

		list_for_each_entry_safe(pos, n, &bb->part_list, list) {
			list_del(&pos->list);
			close_bf_insn(pos->insn);
			free(pos);
		}

		free(bb);
	}
}

void add_bb(binary_file * bf, bf_basic_blk * bb)
{
	if(exists_bb(bf, bb->vma)) {;
		puts("Block already there!!");
	} else {
		htable_add(&bf->bb_table, &bb->entry, &bb->vma, sizeof(bb->vma));
	}
}

bool exists_bb(binary_file * bf, bfd_vma vma)
{
	return htable_find(&bf->bb_table, &vma, sizeof(vma));
}
