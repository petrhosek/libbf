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
		} else {
			/*asection * asec = src_sec->section;
			bfd_byte   buf[asec->size];
			int i;

			bfd_get_section_contents(bf->abfd, asec, buf,
					0, asec->size);

			for(i = 0; i < 5; i++) {
				buf[src_func->vma - src_sec->buffer_vma + i] =
						0x90;
			}

			bfd_set_section_contents(bf->abfd, asec, buf,
					0, asec-> size);*/
		}
	} else {
		perror("Detouring not yet implemented for x86-64");
	}
}
