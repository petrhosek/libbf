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
 * Architecture-specific wrapper for mapping a virtual memory address to a file
 * offset.
 */
uint64_t vaddr_to_file_offset(struct bin_file * bf, uint64_t vaddr)
{
	return IS_BF_ARCH_32(bf) ?
			vaddr32_to_file_offset(bf->output_path, vaddr) :
			vaddr64_to_file_offset(bf->output_path, vaddr);
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
		struct bf_basic_blk * bb)
{
	int bb_size = bf_get_bb_size(bb);

	/*
	 * From the end of the current basic block, find the next instruction.
	 */
	for(int i = DETOUR_LENGTH(bf); i < bb_size; i++) {
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
		uint64_t offset = vaddr_to_file_offset(bf, vma);
		char	 buf[insn->size];

		memset(buf, 0x90, insn->size);
		patch_file(bf->output_path, offset, buf, insn->size);

		if((IS_BF_ARCH_32(bf) && (insn->mnemonic == ret_insn)) ||
				insn->mnemonic == retq_insn) {
			return insn->vma;
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
static void pad_till_next_insn(struct bin_file * bf, struct bf_basic_blk * bb)
{
	if(bf_exists_insn(bf, bb->vma + DETOUR_LENGTH(bf))) {
		return;
	} else {
		int	 next_insn = get_offset_insn_after_detour(bf, bb);
		uint64_t offset	   = vaddr_to_file_offset(bf,
				bb->vma + DETOUR_LENGTH(bf));
		char buf[next_insn - DETOUR_LENGTH(bf)];

		memset(buf, 0x90, next_insn - DETOUR_LENGTH(bf));
		patch_file(bf->output_path, offset, buf,
				next_insn - DETOUR_LENGTH(bf));
	}
}

/*
 * Places a 32 bit detour from 'from' to `to`. This function also checks
 * whether an instruction has been partially overwritten by the detour.
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
 * whether an instruction has been partially overwritten by the detour.
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

/*
 * Wrapper for architecture-specific detouring.
 */
static bool bf_detour(struct bin_file * bf, bfd_vma from, bfd_vma to)
{
	return IS_BF_ARCH_32(bf) ? bf_detour32(bf, from, to) :
			bf_detour64(bf, from, to);
}

bool bf_detour_basic_blk(struct bin_file * bf, struct bf_basic_blk * src_bb,
		struct bf_basic_blk * dest_bb)
{
	if(bf_get_bb_size(src_bb) < DETOUR_LENGTH(bf)) {
		return FALSE;
	} else {
		bool success = bf_detour(bf, src_bb->vma, dest_bb->vma);
		pad_till_next_insn(bf, src_bb);
		return success;
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
	void *	 trampoline	   = 0;
	int	 trampoline_offset = 0;
	char	 trampoline_block[TRAMPOLINE_LENGTH(bf)];
	bfd_byte buf[sec->size];

	/*
	 * Initialisation of the 'needle' and 'haystack' for memmem.
	 */
	memset(trampoline_block, 0x90, TRAMPOLINE_LENGTH(bf));
	bfd_get_section_contents(bf->abfd, sec, buf, 0, sec->size);

	/*
	 * Convert pointer into haystack into offset.
	 */
	if((trampoline = memmem(buf + (vma - sec->vma),
			sec->size - (vma - sec->vma), trampoline_block,
			TRAMPOLINE_LENGTH(bf))) != NULL) {
		trampoline_offset = (bfd_byte *)trampoline - buf;
	}

	return trampoline_offset;
}

/*
 * Relocates a 32 bit instruction.
 */
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

/*
 * Relocates a 64 bit instruction.
 */
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

/*
 * Wrapper for architecture-specific relocation.
 */
static bool relocate_insn(struct bin_file * bf, struct bf_insn * insn,
		bfd_vma to)
{
	return IS_BF_ARCH_32(bf) ? relocate_insn32(bf, insn, to) :
			relocate_insn64(bf, insn, to);
}

/*
 * Relocate a set of instructions.
 */
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
static bfd_vma relocate_epilogue(struct bin_file * bf, bfd_vma from,
		bfd_vma to)
{
	struct bf_insn * insn = bf_get_insn(bf, from);

	if(insn == NULL) {
		return 0;
	} else if(IS_BF_ARCH_32(bf) && (insn->mnemonic == ret_insn)) {
		return to;
	} else if(insn->mnemonic == retq_insn) {
		return to;
	} else {
		relocate_insn(bf, insn, to);
		return relocate_epilogue(bf, from + insn->size,
				to + insn->size);
	}	
}

/*
 * Returns the address of an epilogue. The input address (from) represents the
 * start of a trampoline block identified through get_trampoline_offset.
 */
static bfd_vma find_epilogue(struct bin_file * bf, bfd_vma from)
{
	struct bf_insn * insn;

	do {
		insn  = bf_get_insn(bf, from);
		from += insn->size;
	} while(insn->mnemonic == nop_insn);

	return insn->vma;
}

/*
 * Populates the contents of a trampoline block. This consists of relocating
 * the epilogue, relocating instructions that will be overwritten from the
 * source detour and writing a detour to go back to the source.
 */
static bool bf_populate_trampoline_block(struct bin_file * bf,
		bfd_vma from, bfd_vma to)
{
	asection * sec		     = load_section_for_vma(bf, to)->section;
	int	   trampoline_offset = get_trampoline_offset(bf, sec, to);

	/*
	 * No trampoline block found.
	 */
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
		return bf_detour(bf, next_nop + next_insn,
				from + DETOUR_LENGTH(bf));
	}
}

bool bf_trampoline_basic_blk(struct bin_file * bf,
		struct bf_basic_blk * src_bb, struct bf_basic_blk * dest_bb)
{
	/*
	 * Check bf_basic_blk is long enough to be detoured.
	 */
	if(bf_get_bb_size(src_bb) < DETOUR_LENGTH(bf)) {
		return FALSE;
	} else {
		bool success;

		if(!bf_populate_trampoline_block(bf, src_bb->vma,
				dest_bb->vma)) {
			return FALSE;
		}

		success = bf_detour(bf, src_bb->vma, dest_bb->vma);
		pad_till_next_insn(bf, src_bb);
		return success;
	}
}

bool bf_trampoline_func(struct bin_file * bf,
		struct bf_func * src_func, struct bf_func * dest_func)
{
	return bf_trampoline_basic_blk(bf, src_func->bb, dest_func->bb);
}
