#include "func_analysis.h"

void add_visited_bb(struct bb_cmp_info * info, struct bf_basic_blk * bb,
		struct bf_basic_blk * bb2)
{
	struct visited_bb * v_bb = malloc(sizeof(struct visited_bb));
	v_bb->bb		 = bb;
	v_bb->bb2		 = bb2;

	htable_add(&info->visited_bbs, &v_bb->entry, &bb->vma,
			sizeof(bb->vma));
}

extern void release_visited_info(struct bb_cmp_info * info)
{
	struct htable_entry * cur_entry;
	struct htable_entry * n;
	struct visited_bb *   v_bb;

	htable_for_each_entry_safe(v_bb, cur_entry, n, &info->visited_bbs,
			entry) {
		htable_del_entry(&info->visited_bbs, cur_entry);
		free(v_bb);
	}
}

bool has_visited_bb(struct bb_cmp_info * info, struct bf_basic_blk * bb,
		struct bf_basic_blk * bb2)
{
	struct visited_bb * v_bb = hash_find_entry(&info->visited_bbs,
			&bb->vma, sizeof(bb->vma), struct visited_bb, entry);
	return ((v_bb != NULL) && (v_bb->bb2->vma == bb2->vma));
}

/*
 * Recursive CFG comparison.
 */
extern bool bb_cmp(struct bb_cmp_info * info, struct bf_basic_blk * bb,
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
	} else if(has_visited_bb(info, bb, bb2)) {
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
		add_visited_bb(info, bb, bb2);
		return bb_cmp(info, bb->target, bb2->target) &&
				bb_cmp(info, bb->target2, bb2->target2);
	}
}
