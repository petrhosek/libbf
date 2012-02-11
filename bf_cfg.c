#include "bf_cfg.h"

void print_cfg_stdout(struct bf_basic_blk * bb)
{
	if(bb != NULL) {
		printf("New block:\n");
		print_bf_basic_blk(bb);
		printf("\n\n");
		print_cfg_stdout(bb->target);
		print_cfg_stdout(bb->target2);
	}
}

static void print_cfg_bb_dot(FILE * stream, struct bf_basic_blk * bb)
{
	fprintf(stream, "\t\"%lX\" [label=\"", bb->vma);
	print_bf_basic_blk_dot(stream, bb);
	fprintf(stream, "\",shape=box];\n");

	if(bb->target != 0) {
		fprintf(stream, "\t\"%lX\" -> \"%lX\";\n", bb->vma, bb->target->vma);
		print_cfg_bb_dot(stream, bb->target);
	}

	if(bb->target2 != 0) {
		fprintf(stream, "\t\"%lX\" -> \"%lX\";\n", bb->vma, bb->target2->vma);
		print_cfg_bb_dot(stream, bb->target2);
	}
}

void print_cfg_dot(FILE * stream, struct bf_basic_blk * bb)
{
	if(bb != NULL) {
		fprintf(stream, "digraph G {\n");
		print_cfg_bb_dot(stream, bb);
		fprintf(stream, "}");		
	}
}
