#include "bf_cfg.h"

/*
 * Currently used as a hashmap entry to signify whether the block has already
 * been printed. We can add more members later if necessary. If not, we can
 * use a bitmap instead.
 */
struct bb_visited {
	struct htable_entry entry;
};

static void print_cfg_bb_stdout(struct htable * table, struct bf_basic_blk * bb)
{
	if(bb != NULL) {
		if(htable_find(table, &bb->vma, sizeof(bb->vma))) {
			return;
		}

		struct bb_visited * v = xmalloc(sizeof(struct bb_visited));
		htable_add(table, &v->entry, &bb->vma, sizeof(bb->vma));

		printf("New block: %s\n", bb->sym ? bb->sym->name: "");
		print_bf_basic_blk(bb);
		printf("\n\n");

		print_cfg_bb_stdout(table, bb->target);
		print_cfg_bb_stdout(table, bb->target2);
	}
}

void print_cfg_stdout(struct bf_basic_blk * bb)
{
	struct htable	      table;
	struct htable_entry * cur_entry;
	struct htable_entry * n;

	htable_init(&table);

	print_cfg_bb_stdout(&table, bb);

	htable_for_each_safe(n, cur_entry, &table, node) {
		struct bb_visited * v = hash_entry(cur_entry,
				struct bb_visited, entry);

		htable_del_entry(&table, cur_entry);
		free(v);
	}

	htable_finit(&table);
}

static void print_cfg_bb_dot(struct htable * table, FILE * stream,
		struct bf_basic_blk * bb)
{
	if(bb != NULL) {
		if(htable_find(table, &bb->vma, sizeof(bb->vma))) {
			return;
		}

		struct bb_visited * v = xmalloc(sizeof(struct bb_visited));
		htable_add(table, &v->entry, &bb->vma, sizeof(bb->vma));

		fprintf(stream, "\t\"%lX\" [label=\"", bb->vma);
		if(bb->sym) {
			fprintf(stream, "        %s\\l\\n", bb->sym->name);
		}

		print_bf_basic_blk_dot(stream, bb);
		fprintf(stream, "\",shape=box];\n");

		if(bb->target != 0) {
			fprintf(stream, "\t\"%lX\" -> \"%lX\";\n",
					bb->vma, bb->target->vma);
			print_cfg_bb_dot(table, stream, bb->target);
		}

		if(bb->target2 != 0) {
			fprintf(stream, "\t\"%lX\" -> \"%lX\";\n",
					bb->vma, bb->target2->vma);
			print_cfg_bb_dot(table, stream, bb->target2);
		}
	}
}

void print_cfg_dot(FILE * stream, struct bf_basic_blk * bb)
{
	if(bb != NULL) {
		struct htable	      table;
		struct htable_entry * cur_entry;
		struct htable_entry * n;

		htable_init(&table);

		fprintf(stream, "digraph G {\n");
		print_cfg_bb_dot(&table, stream, bb);
		fprintf(stream, "}");		

		htable_for_each_safe(n, cur_entry, &table, node) {
			struct bb_visited * v = hash_entry(cur_entry,
					struct bb_visited, entry);

			htable_del_entry(&table, cur_entry);
			free(v);
		}

		htable_finit(&table);
	}
}
