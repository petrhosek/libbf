#include "bf_insn.h"

struct bf_insn * init_bf_insn(bfd_vma vma)
{
	struct bf_insn * insn = xmalloc(sizeof(struct bf_insn));
	insn->vma	      = vma;

	INIT_LIST_HEAD(&insn->part_list);
	return insn;
}

void add_insn_part(struct bf_insn * insn, char * str)
{
	struct bf_insn_part * part = xmalloc(sizeof(struct bf_insn_part));
	part->str		   = xstrdup(str);

	INIT_LIST_HEAD(&part->list);
	list_add_tail(&part->list, &insn->part_list);
}

void print_bf_insn(struct bf_insn * insn)
{
	if(insn != NULL) {
		struct bf_insn_part * pos;

		list_for_each_entry(pos, &insn->part_list, list) {
			printf("%s", pos->str);
		}
	}
}

void close_bf_insn(struct bf_insn * insn)
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
