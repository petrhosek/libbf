#include "bf_detour.h"

/*
 * Returns the offset into the bf_basic_blk of the next whole instruction
 * from the end of the patched code.
 */
static int get_offset_next_insn(struct binary_file * bf,
		struct bf_basic_blk * bb)
{
	int bb_size = bf_get_bb_size(bf, bb);
	int i;

	for(i = bf->bitiness == arch_32
			? BF_DETOUR_LENGTH32 : BF_DETOUR_LENGTH64;
			i < bb_size; i++) {
		if(bf_exists_insn(bf, bb->vma + i)) {
			return i;
		}
	}

	return bb_size + 1;
}

/*
 * After the detour, we pad up to the next instruction with NOP. This is not
 * necessary but it helps improve how the disassembly looks afterwards.
 */
static void pad_till_next_insn(struct binary_file * bf,
		struct bf_mem_block * sec, struct bf_basic_blk * bb)
{
	int bb_size = bf_get_bb_size(bf, bb);

	if(bf->bitiness == arch_32) {
	} else {
		if(bb_size > BF_DETOUR_LENGTH64 ||
				!bf_exists_insn(bf, bb->vma +
				BF_DETOUR_LENGTH64)) {
			asection * asec = sec->section;
			bfd_byte   buf[asec->size];
			int i;

			int offset_next_insn = get_offset_next_insn(bf, bb);

			bfd_get_section_contents(bf->obfd, asec, buf,
					0, asec->size);

			for(i = BF_DETOUR_LENGTH64; i < offset_next_insn; i++) {
				buf[bb->vma - sec->buffer_vma + i] =
						0x90;				
			}

			bfd_set_section_contents(bf->obfd, asec, buf,
					0, asec-> size);
		}
	}
}

/*
 * TODO: Add proper comments once the 32 bit part is written.
 */
void bf_detour_func(struct binary_file * bf, struct bf_func * src_func,
		struct bf_func * dest_func)
{
	struct bf_mem_block * src_sec  =
			load_section_for_vma(bf, src_func->vma);
	struct bf_mem_block * dest_sec =
			load_section_for_vma(bf, dest_func->vma);

	if(bf->obfd == NULL) {
		return;
	}

	if(bf->bitiness == arch_32) {
		perror("Detouring not implemented for 32 bit yet");
	} else {
		if(bf_get_bb_size(bf, src_func->bb) < BF_DETOUR_LENGTH64) {
			perror("The basic block of the source basic block "\
					"is too small to detour.");
		} else {
			asection * asec   = src_sec->section;
			bfd_byte   buf[asec->size];
			int i;
			uint64_t   target = dest_func->vma;

			bfd_get_section_contents(bf->obfd, asec, buf,
					0, asec->size);

			buf[src_func->vma - src_sec->buffer_vma] = 0x68;

			for(i = 1; i < 5; i++) {
				buf[src_func->vma - src_sec->buffer_vma + i] =
						((char *)&target)[i - 1];
			}

			buf[src_func->vma - src_sec->buffer_vma + 5] = 0xc7;
			buf[src_func->vma - src_sec->buffer_vma + 6] = 0x44;
			buf[src_func->vma - src_sec->buffer_vma + 7] = 0x24;
			buf[src_func->vma - src_sec->buffer_vma + 8] = 0x04;

			for(i = 9; i < 13; i++) {
				buf[src_func->vma - src_sec->buffer_vma + i] =
						((char *)&target)[i - 5];
			}

			buf[src_func->vma - src_sec->buffer_vma + 13] = 0xc3;

			bfd_set_section_contents(bf->obfd, asec, buf,
					0, asec-> size);

			pad_till_next_insn(bf, src_sec, src_func->bb);
		}
	}
}
