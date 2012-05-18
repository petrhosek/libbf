#include "bf_detour.h"

/*
 * This function takes a bf_basic_blk that has been detoured or will be.
 * It considers the bytes directly after the end of the detour. It returns
 * the offset of the next whole instruction. This offset is from the start of
 * the section containing the bf_basic_blk. It is assumed that the size of the
 * bf_basic_blk is larger than BF_DETOUR_LENGTH32 and BF_DETOUR_LENGTH64 for
 * 32 and 64 bit targets respectively.
 */
static int get_offset_insn_after_detour(struct bin_file * bf,
		struct basic_blk * bb)
{
	int bb_size = bf_get_bb_size(bf, bb);
	int i;

	/*
	 * From the end of the current basic block, find the next instruction.
	 */
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
 * This function takes in a bf_basic_blk that has been detoured or will be.
 * It considers the bytes directly after the end of the detour. If these bytes
 * represent the end of an instruction which was partially overwritten, they
 * are replaced by NOP up to the next whole instruction.
 */
static void pad_till_next_insn(struct bin_file * bf,
		asection * sec, struct basic_blk * bb)
{
	int bb_size = bf_get_bb_size(bf, bb);

	if(bf->bitiness == arch_32) {
	} else {
		/* This case should not really be hit. */
		if(bb_size <= BF_DETOUR_LENGTH64) {
			return;
		/*
		 * If the byte directly after the detour is already the start
		 * of an instruction, there is no need to pad at all!
		 */
		} else if(bf_exists_insn(bf, bb->vma + BF_DETOUR_LENGTH64)) {
			return;
		} else {
			bfd_byte   buf[sec->size];
			int 	   i;
			int 	   offset_next_insn =
					get_offset_insn_after_detour(bf, bb);

			bfd_get_section_contents(bf->obfd, sec, buf,
					0, sec->size);

			for(i = BF_DETOUR_LENGTH64; i < offset_next_insn;
					i++) {
				buf[bb->vma - sec->vma + i] = 0x90;
			}

			bfd_set_section_contents(bf->obfd, sec, buf,
					0, sec->size);
		}
	}
}

/*
 * Places a 64 bit detour from 'from' to `to`. This function also checks
 * whether an instruction has been partially overwritten by the detour. If yes,
 * the rest of the instruction is padded with NOP. This helps readability of
 * the final disassembly.
 */
static bool bf_detour64(struct bin_file * bf, bfd_vma from, bfd_vma to)
{
	asection * sec = load_section_for_vma(bf, from)->section;
	int i;

	/*
	 * Load the section in which the detour will be written to into memory.
	 */
	bfd_byte buf[sec->size];
	bfd_get_section_contents(bf->obfd, sec, buf, 0, sec->size);

	/*
	 * The 64 bit detour works by injecting the following instructions:
	 *  - PUSH <Low DWORD of absolute destination>
	 *  - MOVL <High DWORD of absolute destination>, 4(rsp)
	 *  - RET
	 *
	 * This pushes the absolute QWORD address of destination onto the stack
	 * and then calls RET which uses it as a return address. This method
	 * uses 14 bytes and does not trash any registers.
	 */
	buf[from - sec->vma] = 0x68;

	for(i = 1; i < 5; i++) {
		buf[from - sec->vma + i] = ((char *)&to)[i - 1];
	}

	buf[from - sec->vma + 5] = 0xc7;
	buf[from - sec->vma + 6] = 0x44;
	buf[from - sec->vma + 7] = 0x24;
	buf[from - sec->vma + 8] = 0x04;

	for(i = 9; i < 13; i++) {
		buf[from - sec->vma + i] = ((char *)&to)[i - 5];
	}

	buf[from - sec->vma + 13] = 0xc3;

	bfd_set_section_contents(bf->obfd, sec, buf, 0, sec->size);
	return TRUE;
}

bool bf_detour_basic_blk(struct bin_file * bf, struct basic_blk * src_bb,
		struct basic_blk * dest_bb)
{
	if(bf->obfd == NULL) {
		return FALSE;
	}

	if(bf->bitiness == arch_32) {
		/*
		 * Check bf_basic_blk is long enough to be detoured.
		 */
		if(bf_get_bb_size(bf, src_bb) < BF_DETOUR_LENGTH32) {
			return FALSE;
		} else {
			// bf_detour32()...
			return TRUE;
		}
	} else {
		/*
		 * Check bf_basic_blk is long enough to be detoured.
		 */
		if(bf_get_bb_size(bf, src_bb) < BF_DETOUR_LENGTH64) {
			return FALSE;
		} else {
			bool success = bf_detour64(bf, src_bb->vma,
					dest_bb->vma);
			pad_till_next_insn(bf,
					load_section_for_vma(bf,
					src_bb->vma)->section, src_bb);
			return success;
		}
	}
}

bool bf_detour_func(struct bin_file * bf, struct bf_func * src_func,
		struct bf_func * dest_func)
{
	return bf_detour_basic_blk(bf, src_func->bb, dest_func->bb);
}

/*
 * Returns the offset into the section of the next trampoline. Returns 0 if not
 * found.
 */
static int get_trampoline_offset(struct bin_file * bf,
		asection * sec, bfd_vma vma)
{
	void * trampoline        = 0;
	int    trampoline_offset = 0;

	if(bf->bitiness == arch_32) {
	} else {
		/*
		 * Initialisation of the 'needle' and 'haystack' for memmem.
		 */
		char	 trampoline_block[BF_TRAMPOLINE_LENGTH64];
		bfd_byte buf[sec->size];

		memset(trampoline_block, 0x90, BF_TRAMPOLINE_LENGTH64);

		bfd_get_section_contents(bf->obfd, sec, buf,
				0, sec->size);

		trampoline = memmem(buf + (vma - sec->vma),
				sec->size - ((vma - sec->vma)),
				trampoline_block, BF_TRAMPOLINE_LENGTH64);

		/*
		 * Convert pointer into haystack into offset.
		 */
		if(trampoline != NULL) {
			trampoline_offset = ((bfd_byte *)trampoline) - buf;
		}
	}

	return trampoline_offset;
}

struct RELOC_INFO {
	bfd_byte * src_buf;
	bfd_byte * dest_buf;
	bfd_vma	   from;
	bfd_vma	   to;
	int	   src_bb_offset;
	int	   dest_bb_offset;
	bfd_vma    stop;
};

void relocate_insn(struct bf_insn * insn, struct RELOC_INFO * ri)
{
	if(insn->vma >= ri->stop) {
		return;
	} else {
		int offset = insn->vma - ri->from;

		memcpy(ri->dest_buf + ri->dest_bb_offset + offset,
				ri->src_buf + ri->src_bb_offset + offset,
				insn->size);

		if(insn->mnemonic == callq_insn) {
			int reloc_diff = ri->to - ri->from;

			*(uint32_t *)(ri->dest_buf + ri->dest_bb_offset +
					offset + 1) -= reloc_diff;
			*(ri->dest_buf + ri->dest_bb_offset +
					offset + 5) = 0x5d;
		}
	}
}

static void relocate_insns(struct bin_file * bf, bfd_vma from, bfd_vma to,
		bfd_vma stop)
{
	struct bf_insn *  insn;
	struct RELOC_INFO ri	= {0};
	asection *	  src_sec  = load_section_for_vma(bf, from)->section;
	asection *	  dest_sec = load_section_for_vma(bf, to)->section;	

	bfd_byte src_buf[src_sec->size];
	bfd_byte dest_buf[dest_sec->size];

	bfd_get_section_contents(bf->obfd, src_sec,
			src_buf, 0, src_sec->size);
	bfd_get_section_contents(bf->obfd, dest_sec,
			dest_buf, 0, dest_sec->size);

	ri.src_buf	  = src_buf;
	ri.dest_buf	  = dest_buf;
	ri.from		  = from;
	ri.to		  = to;
	ri.src_bb_offset  = from - src_sec->vma;
	ri.dest_bb_offset = to - dest_sec->vma;
	ri.stop		  = stop;

	bf_for_each_basic_blk_insn(insn, bf_get_bb(bf, from)) {
		relocate_insn(insn, &ri);
	}

	bfd_set_section_contents(bf->obfd, dest_sec,
			dest_buf, 0, dest_sec->size);
}

static bool bf_populate_trampoline_block(struct bin_file * bf,
		bfd_vma from, bfd_vma to)
{
	asection * src_sec  = load_section_for_vma(bf, from)->section;
	asection * dest_sec = load_section_for_vma(bf, to)->section;

	int trampoline_offset = get_trampoline_offset(bf, dest_sec, to);

	if(trampoline_offset == 0) {
		return FALSE;
	} else {
		/*
		 * Check how many bytes we need to copy from the source to the
		 * destination. If placing a detour partially overwrites an
		 * instruction, we need to copy that whole instruction to the
		 * trampoline because it will be NOP padded by the detour.
		 */
		int offset_next_insn =
				get_offset_insn_after_detour(bf,
				bf_get_bb(bf, from));
		relocate_insns(bf, from, dest_sec->vma + trampoline_offset,
				from + offset_next_insn);

		/*
		 * Now set the detour to go back.
		 */
		if(bf->bitiness == arch_32) {
			return TRUE;
		} else {
			/*
			 * The 27 signifies maximimum number of bytes that
			 * can be copied from the source to the trampoline.
			 */
			return bf_detour64(bf,
					dest_sec->vma + trampoline_offset + 27,
					from + BF_DETOUR_LENGTH64);
		}
	}
}

bool bf_detour_basic_blk_with_trampoline(struct bin_file * bf,
		struct basic_blk * src_bb, struct basic_blk * dest_bb)
{
	if(bf->obfd == NULL) {
		return FALSE;
	}

	if(bf->bitiness == arch_32) {
		perror("Trampolining not implemented for 32 bit yet");
	} else {
		/*
		 * Check bf_basic_blk is long enough to be detoured.
		 */
		if(bf_get_bb_size(bf, src_bb) < BF_DETOUR_LENGTH64) {
			return FALSE;
		} else {
			bool success;

			if(!bf_populate_trampoline_block(bf, src_bb->vma,
					dest_bb->vma)) {
				return FALSE;
			}

			success = bf_detour64(bf, src_bb->vma,
					dest_bb->vma);
			pad_till_next_insn(bf,
					load_section_for_vma(bf,
					src_bb->vma)->section, src_bb);
			return success;
		}
	}
	return TRUE;
}

bool bf_detour_func_with_trampoline(struct bin_file * bf,
		struct bf_func * src_func, struct bf_func * dest_func)
{
	return bf_detour_basic_blk_with_trampoline(bf, src_func->bb,
			dest_func->bb);
}
