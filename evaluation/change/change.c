#include <stdlib.h>
#include <stdio.h>
#include <libbind/binary_file.h>
#include <libbind/basic_blk.h>
#include <libbind/insn.h>
#include <libbind/symbol.h>
#include <libkern/htable.h>

struct bb_cmp_info {
	struct htable	  visited_bbs;
};

struct visited_bb {
	struct bf_basic_blk * bb;
	struct htable_entry   entry;
};

void add_visited_bb(struct bb_cmp_info * info, struct bf_basic_blk * bb)
{
	struct visited_bb * v_bb = malloc(sizeof(struct visited_bb));
	v_bb->bb		 = bb;

	htable_add(&info->visited_bbs, &v_bb->entry, &bb->vma,
			sizeof(bb->vma));
}

void release_visited_info(struct bb_cmp_info * info)
{
	struct htable_entry * cur_entry;
	struct htable_entry * n;
	struct visited_bb *   bb;

	htable_for_each_entry_safe(bb, cur_entry, n, &info->visited_bbs,
			entry) {
		htable_del_entry(&info->visited_bbs, cur_entry);
		free(bb);
	}
}

/*
 * A quick note here. At the moment both bf_get_bb_insn and bf_get_bb_length
 * are O(n). This can (and probably eventually _should_) be changed to O(K).
 */
bool bb_cmp(struct bb_cmp_info * info, struct bf_basic_blk * bb,
		struct bf_basic_blk * bb2)
{
	/*
	 * Both NULL.
	 */
	if(bb == NULL && bb2 == NULL) {
		return TRUE;
	/*
	 * Branch in one but not the other.
	 */
	} else if(bb == NULL || bb2 == NULL) {
		return FALSE;
	/*
	 * Already visited.
	 */
	} else if(htable_find(&info->visited_bbs, &bb->vma, sizeof(bb->vma))) {
		return TRUE;
	} else {
		unsigned int length = bf_get_bb_length(bb);

		/*
		 * Different num instructions.
		 */
		if(bf_get_bb_length(bb2) != length) {
			return FALSE;
		}

		/*
		 * Check each instruction mnemonic.
		 */
		for(int i = 0; i < length; i++) {
			if(bf_get_bb_insn(bb, i)->mnemonic !=
					bf_get_bb_insn(bb2, i)->mnemonic) {
				return FALSE;
			}
		}

		/*
		 * Update visited bbs and compare the next bbs in the CFG.
		 */
		add_visited_bb(info, bb);
		return bb_cmp(info, bb->target, bb2->target) &&
				bb_cmp(info, bb->target2, bb2->target2);
	}
}

int main(void)
{
	struct bin_file * bf  = load_bin_file("target1", NULL);
	struct bin_file * bf2 = load_bin_file("target2", NULL);

	struct symbol * sym;
	struct symbol * sym2;

	struct bf_basic_blk * bb1, * bb2;
	struct bb_cmp_info    info;

	for_each_symbol(sym, &bf->sym_table) {
		if((sym->type & SYMBOL_FUNCTION) && (sym->address != 0)) {
			sym2 = symbol_find(&bf2->sym_table, sym->name);

			if(sym2 == NULL) {
				printf("%s is new in target1\n", sym->name);
			} else {
				bb1 = disasm_bin_file_sym(bf, sym, TRUE);
				bb2 = disasm_bin_file_sym(bf2, sym2, TRUE);

				htable_init(&info.visited_bbs);
				
				if(bb_cmp(&info, bb1, bb2)) {
					printf("%s did not change\n",
							sym->name);
 				} else {
					printf("%s did change\n",
							sym->name);
				}

				release_visited_info(&info);
				htable_destroy(&info.visited_bbs);
			}
		}
	}

	close_bin_file(bf);
	close_bin_file(bf2);
	return EXIT_SUCCESS;
}
