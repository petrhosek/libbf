#ifndef FUNC_ANALYSIS_H
#define FUNC_ANALYSIS_H

#include <libbf/basic_blk.h>
#include <libbf/insn.h>

struct bb_cmp_info {
	struct htable visited_bbs;
};

struct visited_bb {
	struct bf_basic_blk * bb;
	struct bf_basic_blk * bb2;
	struct htable_entry   entry;
};

struct change_info {
	int removed;
	int added;
	int modified;
	int same;
};

bool bb_cmp(struct bb_cmp_info * info, struct bf_basic_blk * bb,
		struct bf_basic_blk * bb2);
void release_visited_info(struct bb_cmp_info * info);

#endif
