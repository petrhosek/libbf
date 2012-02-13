#include "bf_insn.h"

struct bf_insn * bf_init_insn(struct bf_basic_blk * bb, bfd_vma vma)
{
	struct bf_insn * insn = xmalloc(sizeof(struct bf_insn));
	insn->vma	      = vma;
	insn->bb	      = bb;
	insn->target	      = 0;
	insn->target	      = 0;

	INIT_LIST_HEAD(&insn->part_list);
	return insn;
}

void bf_add_insn_target(struct bf_insn * insn, bfd_vma vma)
{
	assert(insn && (!insn->target || !insn->target2));

	if(insn->target == 0) {
		insn->target = vma;
	} else if(insn->target2 == 0) {
		insn->target2 = vma;
	}
}

void bf_add_insn_part(struct bf_insn * insn, char * str)
{
	struct bf_insn_part * part = xmalloc(sizeof(struct bf_insn_part));
	part->str		   = xstrdup(str);

	INIT_LIST_HEAD(&part->list);
	list_add_tail(&part->list, &insn->part_list);
}

void bf_print_insn(struct bf_insn * insn)
{
	if(insn != NULL) {
		struct bf_insn_part * pos;

		list_for_each_entry(pos, &insn->part_list, list) {
			printf("%s", pos->str);
		}
	}
}

void bf_print_insn_dot(FILE * stream, struct bf_insn * insn)
{
	if(insn != NULL) {
		struct bf_insn_part * pos;

		list_for_each_entry(pos, &insn->part_list, list) {
			fprintf(stream, "%s", pos->str);
		}

		fprintf(stream, "\\l\\n");
	}
}

void bf_close_insn(struct bf_insn * insn)
{
	if(insn != NULL) {
		struct bf_insn_part * pos;
		struct bf_insn_part * n;

		list_for_each_entry_safe(pos, n, &insn->part_list, list) {
			list_del(&pos->list);
			free(pos->str);
			free(pos);
		}

		free(insn);
	}
}

void bf_add_insn(struct binary_file * bf, struct bf_insn * insn)
{
	assert(!bf_exists_insn(bf, insn->vma));

	htable_add(&bf->insn_table, &insn->entry, &insn->vma, sizeof(insn->vma));
}

struct bf_insn * bf_get_insn(struct binary_file * bf, bfd_vma vma)
{
	struct htable_entry * entry = htable_find(&bf->insn_table, &vma,
			sizeof(vma));

	if(entry == NULL) {
		return NULL;
	}

	return hash_entry(entry, struct bf_insn, entry);
}

bool bf_exists_insn(struct binary_file * bf, bfd_vma vma)
{
	return htable_find(&bf->insn_table, &vma, sizeof(vma));
}
