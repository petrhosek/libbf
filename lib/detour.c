#include "detour.h"

/*
 * Sets the bytes at an arbitrary offset of a file to the contents of buffer.
 */
static bool patch_file(char * filepath, uint64_t offset, void * buffer,
		size_t size)
{
		FILE * pFile = fopen(filepath, "r+");

		if(pFile == NULL) {
			return FALSE;
		}

		fseek(pFile, offset, SEEK_SET);
		fwrite(buffer, 1, size, pFile);
		fclose(pFile);
		return TRUE;
}

/*
 * Returns the corresponding 32 bit executable file offset of a virtual memory
 * address.
 */
uint32_t vaddr32_to_file_offset(char * filepath, uint32_t vaddr)
{
	int      fd     = open(filepath, O_RDONLY);
	Elf *    e      = elf_begin(fd, ELF_C_READ, NULL);
	uint32_t offset = 0;

	Elf_Scn * scn = NULL;
	while((scn = elf_nextscn(e, scn)) != NULL) {
		Elf32_Shdr * shdr = elf32_getshdr(scn);
		if(vaddr >= shdr->sh_addr &&
				(vaddr <= (shdr->sh_addr + shdr->sh_size))) {
			offset = shdr->sh_offset + (vaddr - shdr->sh_addr);
			break;
		}
	}

	elf_end(e);
	close(fd);
	return offset;
}

/*
 * Returns the corresponding 64 bit executable file offset of a virtual memory
 * address.
 */
uint64_t vaddr64_to_file_offset(char * filepath, uint64_t vaddr)
{
	int      fd     = open(filepath, O_RDONLY);
	Elf *    e      = elf_begin(fd, ELF_C_READ, NULL);
	uint64_t offset = 0;

	Elf_Scn * scn = NULL;
	while((scn = elf_nextscn(e, scn)) != NULL) {
		Elf64_Shdr * shdr = elf64_getshdr(scn);
		if(vaddr >= shdr->sh_addr &&
				(vaddr <= (shdr->sh_addr + shdr->sh_size))) {
			offset = shdr->sh_offset + (vaddr - shdr->sh_addr);
			break;
		}
	}

	elf_end(e);
	close(fd);
	return offset;
}

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
 * This function replaces all instructions from vma to the next RET/RETQ with
 * the NOP instruction. It returns the address of the final instruction
 * replaced.
 */
static bfd_vma pad_till_return(struct bin_file * bf, bfd_vma vma)
{
	struct bf_insn * insn = bf_get_insn(bf, vma);

	if(insn != NULL) {
		char buf[insn->size];
		memset(buf, 0x90, insn->size);

		if(bf->bitiness == arch_32) {
			uint32_t offset = vaddr32_to_file_offset(
					bf->output_path, insn->vma);
			patch_file(bf->output_path, offset, buf, insn->size);

			if(insn->mnemonic == ret_insn) {
				return insn->vma;
			}
		} else {
			uint64_t offset = vaddr64_to_file_offset(
				bf->output_path, insn->vma);
			patch_file(bf->output_path, offset, buf, insn->size);

			if(insn->mnemonic == retq_insn) {
				return insn->vma;
			}
		}

		return pad_till_return(bf, vma + insn->size);
	}

	return 0;
}

/*
 * This function takes in a bf_basic_blk that has been detoured or will be.
 * It considers the bytes directly after the end of the detour. If these bytes
 * represent the end of an instruction which was partially overwritten, they
 * are replaced by NOP up to the next whole instruction.
 */
static void pad_till_next_insn(struct bin_file * bf, struct basic_blk * bb)
{
	int bb_size = bf_get_bb_size(bf, bb);

	if(bf->bitiness == arch_32) {
		if(bb_size <= BF_DETOUR_LENGTH32) {
			return;
		} else if(bf_exists_insn(bf, bb->vma + BF_DETOUR_LENGTH32)) {
			return;
		} else {
			int	 offset_next_insn =
					get_offset_insn_after_detour(bf, bb);
			char	 buf[offset_next_insn - BF_DETOUR_LENGTH32];
			uint32_t offset	   =
					vaddr32_to_file_offset(bf->output_path,
					bb->vma + BF_DETOUR_LENGTH32);

			memset(buf, 0x90,
					offset_next_insn - BF_DETOUR_LENGTH32);
			patch_file(bf->output_path, offset, buf,
					offset_next_insn - BF_DETOUR_LENGTH32);
		}
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
			int	 offset_next_insn =
					get_offset_insn_after_detour(bf, bb);
			char	 buf[offset_next_insn - BF_DETOUR_LENGTH64];
			uint64_t offset	   =
					vaddr64_to_file_offset(bf->output_path,
					bb->vma + BF_DETOUR_LENGTH64);

			memset(buf, 0x90,
					offset_next_insn - BF_DETOUR_LENGTH64);
			patch_file(bf->output_path, offset, buf,
					offset_next_insn - BF_DETOUR_LENGTH64);
		}
	}
}

/*
 * Places a 32 bit detour from 'from' to `to`. This function also checks
 * whether an instruction has been partially overwritten by the detour. If so,
 * the rest of the instruction is padded with NOP. This helps readability of
 * the final disassembly.Furthermore, it makes it convenient to trampoline
 * back.
 */
static bool bf_detour32(struct bin_file * bf, bfd_vma from, bfd_vma to)
{
	uint32_t offset = vaddr32_to_file_offset(bf->output_path, from);

	if(offset == 0) {
		return FALSE;
	} else {
		/*
		 * The 32 bit detour works by injecting a relative JMP. This
		 * method uses 5 bytes and does not trash any registers.
		 */
		char buffer[] = {0xe9, 0x0, 0x0, 0x0, 0x0};

		*(uint32_t *)&buffer[1] = (uint32_t)to - (uint32_t)from - 5;
		return patch_file(bf->output_path, offset, buffer,
				ARRAY_SIZE(buffer));
	}
}

/*
 * Places a 64 bit detour from 'from' to `to`. This function also checks
 * whether an instruction has been partially overwritten by the detour. If so,
 * the rest of the instruction is padded with NOP. This helps readability of
 * the final disassembly. Furthermore, it makes it convenient to trampoline
 * back.
 */
static bool bf_detour64(struct bin_file * bf, bfd_vma from, bfd_vma to)
{
	uint64_t offset = vaddr64_to_file_offset(bf->output_path, from);

	if(offset == 0) {
		return FALSE;
	} else {
		/*
		 * The 64 bit detour works by injecting the following
		 * instructions:
		 *  - PUSH <Low DWORD of absolute destination>
		 *  - MOVL <High DWORD of absolute destination>, 4(rsp)
		 *  - RET
		 *
		 * This pushes the absolute QWORD address of destination onto
		 * the stack and then calls RET which uses it as a return
		 * address. This method uses 14 bytes and does not trash any
		 * registers.
		 */
		char buffer[] = {0x68,
				 0x0, 0x0, 0x0, 0x0,
				 0xc7, 0x44, 0x24, 0x04,
				 0x0, 0x0, 0x0, 0x0,
				 0xc3};

		*(uint32_t *)&buffer[1] = *(uint32_t *)&to;
		*(uint32_t *)&buffer[9] = *(uint32_t *)(((char *)&to) + 4);
		return patch_file(bf->output_path, offset, buffer,
				ARRAY_SIZE(buffer));
	}
}

bool bf_detour_basic_blk(struct bin_file * bf, struct basic_blk * src_bb,
		struct basic_blk * dest_bb)
{
	if(bf->bitiness == arch_32) {
		/*
		 * Check bf_basic_blk is long enough to be detoured.
		 */
		if(bf_get_bb_size(bf, src_bb) < BF_DETOUR_LENGTH32) {
			return FALSE;
		} else {
			bool success = bf_detour32(bf, src_bb->vma,
					dest_bb->vma);
			pad_till_next_insn(bf, src_bb);
			return success;
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
			pad_till_next_insn(bf, src_bb);
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
static int get_trampoline_offset(struct bin_file * bf, asection * sec,
		bfd_vma vma)
{
	void * trampoline	= 0;
	int    trampoline_offset = 0;

	if(bf->bitiness == arch_32) {
		/*
		 * Initialisation of the 'needle' and 'haystack' for memmem.
		 */
		char	 trampoline_block[BF_TRAMPOLINE_LENGTH32];
		bfd_byte buf[sec->size];

		memset(trampoline_block, 0x90, BF_TRAMPOLINE_LENGTH32);

		bfd_get_section_contents(bf->abfd, sec, buf,
				0, sec->size);

		trampoline = memmem(buf + (vma - sec->vma),
				sec->size - ((vma - sec->vma)),
				trampoline_block, BF_TRAMPOLINE_LENGTH32);

		/*
		 * Convert pointer into haystack into offset.
		 */
		if(trampoline != NULL) {
			trampoline_offset = ((bfd_byte *)trampoline) - buf;
		}
	} else {
		/*
		 * Initialisation of the 'needle' and 'haystack' for memmem.
		 */
		char	 trampoline_block[BF_TRAMPOLINE_LENGTH64];
		bfd_byte buf[sec->size];

		memset(trampoline_block, 0x90, BF_TRAMPOLINE_LENGTH64);

		bfd_get_section_contents(bf->abfd, sec, buf,
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

static bool relocate_insn32(struct bin_file * bf, struct bf_insn * insn,
		bfd_vma to)
{
	uint32_t offset = vaddr32_to_file_offset(bf->output_path, to);

	if(offset == 0) {
		return FALSE;
	} else {
		asection * sec = load_section_for_vma(bf, insn->vma)->section;
		bfd_byte   buf[sec->size];
		char       insn_buf[insn->size];

		bfd_get_section_contents(bf->abfd, sec,	buf, 0, sec->size);
		memcpy(insn_buf, buf + (insn->vma - sec->vma), insn->size);
		return patch_file(bf->output_path, offset, insn_buf,
				insn->size);
	}
}

static bool relocate_insn64(struct bin_file * bf, struct bf_insn * insn,
		bfd_vma to)
{
	uint64_t offset = vaddr64_to_file_offset(bf->output_path, to);

	if(offset == 0) {
		return FALSE;
	} else {
		asection * sec = load_section_for_vma(bf, insn->vma)->section;
		bfd_byte   buf[sec->size];
		char       insn_buf[insn->size];

		bfd_get_section_contents(bf->abfd, sec,	buf, 0, sec->size);
		memcpy(insn_buf, buf + (insn->vma - sec->vma), insn->size);

		if(insn->mnemonic == callq_insn) {
			int reloc_diff		   = to - insn->vma;
			*(uint32_t *)&insn_buf[1] -= reloc_diff;
		}

		return patch_file(bf->output_path, offset, insn_buf,
				insn->size);
	}
}

static bool relocate_insn(struct bin_file * bf, struct bf_insn * insn,
		bfd_vma to)
{
	if(bf->bitiness == arch_32) {
		return relocate_insn32(bf, insn, to);
	} else {
		return relocate_insn64(bf, insn, to);
	}
}

static bool relocate_insns(struct bin_file * bf, bfd_vma from, bfd_vma to,
		bfd_vma stop)
{
	bool		 success  = TRUE;
	struct bf_insn * insn;

	bf_for_each_basic_blk_insn(insn, bf_get_bb(bf, from)) {
		if(insn->vma >= stop) {
			break;
		} else if(!relocate_insn(bf, insn, to + (insn->vma - from))) {
			success = FALSE;
		}
	}

	return success;
}

/*
 * Relocate the epilogue. This function returns the next untouched instruction
 * in the destination.
 */
static bfd_vma relocate_epilogue(struct bin_file * bf, bfd_vma from, bfd_vma to)
{
	struct bf_insn * insn = bf_get_insn(bf, from);

	if(insn == NULL) {
		return 0;
	} else if((bf->bitiness == arch_32) && (insn->mnemonic == ret_insn)) {
		return to;
	} else if(insn->mnemonic == retq_insn) {
		return to;
	} else {
		relocate_insn(bf, insn, to);
		return relocate_epilogue(bf, from + insn->size,
				to + insn->size);
	}	
}

static bfd_vma find_epilogue(struct bin_file * bf, bfd_vma from)
{
	struct bf_insn * insn;

	do {
		insn  = bf_get_insn(bf, from);
		from += insn->size;
	} while(insn->mnemonic == nop_insn);

	return insn->vma;
}

static bool bf_populate_trampoline_block(struct bin_file * bf,
		bfd_vma from, bfd_vma to)
{
	asection * sec		     = load_section_for_vma(bf, to)->section;
	int	   trampoline_offset = get_trampoline_offset(bf, sec, to);

	if(trampoline_offset == 0) {
		return FALSE;
	} else {
		/*
		 * Relocate the epilogue so the stack gets cleaned up.
		 */
		bfd_vma epilogue = find_epilogue(bf,
				sec->vma + trampoline_offset);
		bfd_vma next_nop = relocate_epilogue(bf, epilogue,
				sec->vma + trampoline_offset);
		pad_till_return(bf, next_nop);

		/*
		 * Now relocate the bytes the detour is going to overwrite.
		 * First check how many bytes we need to copy from the source
		 * to the destination. If placing a detour partially overwrites
		 * an instruction, we need to copy that whole instruction to
		 * the trampoline because it will be NOP padded by the detour.
		 */
		int next_insn =	get_offset_insn_after_detour(bf,
				bf_get_bb(bf, from));

		if(!relocate_insns(bf, from, next_nop, from + next_insn)) {
			return FALSE;
		}

		/*
		 * Now set the detour to go back.
		 */
		if(bf->bitiness == arch_32) {
			return bf_detour32(bf, next_nop + next_insn,
					from + BF_DETOUR_LENGTH32);
		} else {
			return bf_detour64(bf, next_nop + next_insn,
					from + BF_DETOUR_LENGTH64);
		}
	}
}

bool bf_trampoline_basic_blk(struct bin_file * bf, struct basic_blk * src_bb,
		struct basic_blk * dest_bb)
{
	if(bf->bitiness == arch_32) {
		/*
		 * Check bf_basic_blk is long enough to be detoured.
		 */
		if(bf_get_bb_size(bf, src_bb) < BF_DETOUR_LENGTH32) {
			return FALSE;
		} else {
			bool success;

			if(!bf_populate_trampoline_block(bf, src_bb->vma,
					dest_bb->vma)) {
				return FALSE;
			}

			success = bf_detour32(bf, src_bb->vma,
					dest_bb->vma);
			pad_till_next_insn(bf, src_bb);
			return success;
		}
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
			pad_till_next_insn(bf, src_bb);
			return success;
		}
	}
	return TRUE;
}

bool bf_trampoline_func(struct bin_file * bf,
		struct bf_func * src_func, struct bf_func * dest_func)
{
	return bf_trampoline_basic_blk(bf, src_func->bb, dest_func->bb);
}
