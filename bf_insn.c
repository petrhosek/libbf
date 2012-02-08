#include "bf_insn.h"

bf_insn * init_bf_insn(void)
{
	bf_insn * insn  = xmalloc(sizeof(bf_insn));
	insn->part_list = NULL;
	return insn;
}

void add_insn_part(bf_insn * insn, char * str)
{
	bf_insn_part * part = xmalloc(sizeof(bf_insn_part));
	part->str	    = xstrdup(str);

	if(insn->part_list == NULL) {
		insn->part_list = part;
		INIT_LIST_HEAD(&part->list);
	} else {
		list_add(&part->list, &insn->part_list->list);
	}
}

void print_bf_insn(bf_insn * insn)
{
	if(insn != NULL) {
		bf_insn_part * list = insn->part_list;

		if(list != NULL) {
			struct list_head * pos  = NULL;
			bf_insn_part *	   part = NULL;

			list_for_each(pos, &list->list) {
				/* part = list_entry(pos, struct bf_insn_part, list);
				printf("%s", part->str);*/
			}
		}
	}
}

void close_bf_insn(bf_insn * insn)
{
	if(insn != NULL) {
		struct list_head * pos;
		struct list_head * n;

		bf_insn_part * list = insn->part_list;

		if(list != NULL) {
			list_for_each_safe(pos, n, &list->list) {
				/*bf_insn_part * part = list_entry(pos,
						bf_insn_part, list);
				list_del(pos);
				free(part->str);
				free(part);*/
			}
		}

		free(insn);
	}
}
