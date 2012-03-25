#include "bf_detour.h"

void bf_detour_func(struct binary_file * bf, struct bf_func * src_func,
		struct bf_func * dest_func)
{
	struct bf_mem_block * src_sec  =
			load_section_for_vma(bf, src_func->vma);
	struct bf_mem_block * dest_sec =
			load_section_for_vma(bf, dest_func->vma);

	if(bf->bitiness == arch_32) {
		if(bf_get_bb_size(bf, src_func->bb) < 5) {
			perror("The basic block of the source basic block "\
					"is too small to detour.");
		}

		puts("Proceeding with detour!");
	} else {
		perror("Detouring not yet implemented for x86-64");
	}
}
