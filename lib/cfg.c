#include "cfg.h"

/*
 * Currently used as a hashmap entry to signify whether the block has already
 * been printed. We can add more members later if necessary. If not, we can
 * use a bitmap instead.
 */
struct bb_visited {
	struct htable_entry entry;
};

static void print_cfg_bb_stdout(struct bf_basic_blk * bb)
{
	printf("New block: %s\n", bb->sym ? bb->sym->name: "");
	bf_print_basic_blk(bb);
	printf("\n\n");
}

static void print_cfg_bb_stdout_recur(struct htable * table,
		struct bf_basic_blk * bb)
{
	if(bb != NULL) {
		if(htable_find(table, &bb->vma, sizeof(bb->vma))) {
			return;
		}

		struct bb_visited * v = xmalloc(sizeof(struct bb_visited));
		htable_add(table, &v->entry, &bb->vma, sizeof(bb->vma));

		print_cfg_bb_stdout(bb);

		print_cfg_bb_stdout_recur(table, bb->target);
		print_cfg_bb_stdout_recur(table, bb->target2);
	}
}

void print_cfg_stdout(struct bf_basic_blk * bb)
{
	struct htable	      table;
	struct htable_entry * cur_entry;
	struct htable_entry * n;

	htable_init(&table);

	print_cfg_bb_stdout_recur(&table, bb);

	htable_for_each_safe(cur_entry, n, &table) {
		struct bb_visited * v = hash_entry(cur_entry,
				struct bb_visited, entry);
		htable_del_entry(&table, cur_entry);
		free(v);
	}

	htable_destroy(&table);
}

static void print_cfg_bb_dot(FILE * stream, struct bin_file * bf,
		struct bf_basic_blk * bb)
{
	fprintf(stream, "\t\"%lX\" [label=\"", bb->vma);
	if(bb->sym) {
		fprintf(stream, "        %s\\l\\n", bb->sym->name);
	}

	bf_print_basic_blk_dot(stream, bb);
	fprintf(stream, "\",shape=box];\n");

	if(bb->target != 0) {
		fprintf(stream, "\t\"%lX\" -> \"%lX\";\n",
				bb->vma, bb->target->vma);		
	}

	if(bb->target2 != 0) {
		fprintf(stream, "\t\"%lX\" -> \"%lX\";\n",
				bb->vma, bb->target2->vma);
	}
}

static void print_cfg_bb_dot_recur(struct htable * table, FILE * stream,
		struct bin_file * bf, struct bf_basic_blk * bb)
{
	if(bb != NULL) {
		if(htable_find(table, &bb->vma, sizeof(bb->vma))) {
			return;
		}

		struct bb_visited * v = xmalloc(sizeof(struct bb_visited));
		htable_add(table, &v->entry, &bb->vma, sizeof(bb->vma));

		print_cfg_bb_dot(stream, bf, bb);

		if(bb->target != 0) {
			print_cfg_bb_dot_recur(table, stream, bf, bb->target);
		}

		if(bb->target2 != 0) {
			print_cfg_bb_dot_recur(table, stream, bf, bb->target2);
		}
	}
}

void print_cfg_dot(FILE * stream, struct bin_file * bf,
		struct bf_basic_blk * bb)
{
	if(bb != NULL) {
		struct htable	      table;
		struct htable_entry * cur_entry;
		struct htable_entry * n;

		htable_init(&table);

		fprintf(stream, "digraph G {\n");
		print_cfg_bb_dot_recur(&table, stream, bf, bb);
		fprintf(stream, "}");		

		htable_for_each_safe(cur_entry, n, &table) {
			struct bb_visited * v = hash_entry(cur_entry,
					struct bb_visited, entry);

			htable_del_entry(&table, cur_entry);
			free(v);
		}

		htable_destroy(&table);
	}
}

void print_entire_cfg_stdout(struct bin_file * bf)
{
	struct bf_basic_blk * bb;

	bf_for_each_basic_blk(bb, bf) {
		print_cfg_bb_stdout(bb);
	}
}

void print_entire_cfg_dot(struct bin_file * bf, FILE * stream)
{
	struct bf_basic_blk * bb;

	fprintf(stream, "digraph G{\n");

	bf_for_each_basic_blk(bb, bf) {
		print_cfg_bb_dot(stream, bf, bb);
	}

	fprintf(stream, "}");
}

static void print_each_bf_insn(struct bin_file * bf, struct bf_insn * insn,
		void * param)
{
	bf_print_insn_to_file(param, insn);
	fprintf(param, "\n");
}

void print_all_bf_insn(struct bin_file * bf, FILE * stream)
{
	bf_enum_insn(bf, print_each_bf_insn, stream);
}

struct PRINT_INSN_INFO {
	FILE *		   stream;
	enum arch_bitiness bitiness;
};

static void print_each_bf_insn_semantic_gen(struct bin_file * bf,
		struct bf_insn * insn, void * param)
{
	struct PRINT_INSN_INFO * info = param;

	if(!insn->is_data) {
		bf_print_insn_semantic_gen_to_file(info->stream, insn,
				info->bitiness);
		fprintf(info->stream, "\n");
	}
}

void print_all_bf_insn_semantic_gen(struct bin_file * bf, FILE * stream)
{
	struct PRINT_INSN_INFO info;
	info.stream   = stream;
	info.bitiness = bf->bitiness;
	bf_enum_insn(bf, print_each_bf_insn_semantic_gen, &info);
}
