#include "bf_disasm.h"
#include "bf_insn_decoder.h"
#include "bf_basic_blk.h"
#include "bf_mem_manager.h"

static void update_insn_info(struct binary_file * bf, char * str)
{
	if(!bf->disasm_config.insn_info_valid) {
		/*
		 * We come in here if we are analysing the mnemonic part.
		 * In x86 there are never two targets for branching so we
		 * use target2 to keep track of whether the branch target
		 * has been checked yet.
		 */

		bf->disasm_config.insn_info_valid = TRUE;
		bf->disasm_config.target2	  = 0;

		/*
		 * We use dis_condjsr to represent instructions which end flow
		 * even though it is not quite appropriate. It is the best fit.
		 */

		if(breaks_flow(str)) {
			bf->disasm_config.insn_type = dis_branch;
		} else if(branches_flow(str)) {
			bf->disasm_config.insn_type = dis_condbranch;
		} else if(calls_subroutine(str)) {
			bf->disasm_config.insn_type = dis_jsr;
		} else if(ends_flow(str)) {
			bf->disasm_config.insn_type = dis_condjsr;
		} else {
			bf->disasm_config.insn_type = dis_nonbranch;
		}
	} else {
		if(bf->disasm_config.target2 == 0) {
			bf->disasm_config.target2 = 1;

			switch(bf->disasm_config.insn_type) {
			case dis_branch:
			case dis_condbranch:
			case dis_jsr:
				bf->disasm_config.target = get_vma_target(str);
				/* Just fall through */
			default:
				break;
			}
		}
	}
}

int binary_file_fprintf(void * stream, const char * format, ...)
{
	char str[256] = {0};
	int rv;

	struct binary_file * bf   = stream;
	va_list		     args;

	va_start(args, format);
	rv = vsnprintf(str, ARRAY_SIZE(str) - 1, format, args);
	va_end(args);

	bf_add_insn_part(bf->context.insn, str);

	update_insn_info(bf, str);
	return rv;
}

static unsigned int disasm_single_insn(struct binary_file * bf, bfd_vma vma)
{
	bf->disasm_config.insn_info_valid = 0;
	bf->disasm_config.target	  = 0;
	return bf->disassembler(vma, &bf->disasm_config);
}

static struct bf_basic_blk * disasm_block(struct binary_file * bf, bfd_vma vma)
{
	struct bf_mem_block * mem = load_section_for_vma(bf, vma);
	struct bf_basic_blk * bb;

	if(!mem) {
		puts("Failed to load section");
		return NULL;
	} else {
		bf->disasm_config.buffer	= mem->buffer;
		bf->disasm_config.section     	= mem->section;
		bf->disasm_config.buffer_length	= mem->buffer_length;
		bf->disasm_config.buffer_vma	= mem->buffer_vma;
	}

	if(bf_exists_bb(bf, vma)) {
		return bf_get_bb(bf, vma);
	} else if(bf_exists_insn(bf, vma)) {
		struct bf_insn * insn;
		int size;

		bb = bf_split_blk(bf, bf_get_insn(bf, vma)->bb, vma);
		bf_add_bb(bf, bb);

		insn = list_entry(bb->part_list.prev,
				struct bf_basic_blk_part, list)->insn;
		size = disasm_single_insn(bf, insn->vma);

		bfd_vma branch_target = bf->disasm_config.target;

		if(bf->disasm_config.insn_type != dis_branch) {
			bf_add_next_basic_blk(bb,
					disasm_block(bf, insn->vma + size));
		}

		if(branch_target) {
			bf_add_next_basic_blk(bb,
					disasm_block(bf, branch_target));
		}

		return bb;
	} else {
		bb = bf_init_basic_blk(bf, vma);
		bf_add_bb(bf, bb);
	}

	bf->disasm_config.insn_type = dis_noninsn;

	while(bf->disasm_config.insn_type != dis_condjsr) {
		int size;

		bf->context.insn = bf_init_insn(bb, vma);
		bf_add_insn(bf, bf->context.insn);
		bf_add_insn_to_bb(bb, bf->context.insn);

		size = disasm_single_insn(bf, vma);

		if(size == -1 || size == 0) {
			puts("Something went wrong");
			return NULL;
		}

		printf("Disassembled %d bytes at 0x%lX\n\n", size, vma);

		if(bf->disasm_config.insn_type == dis_condbranch ||
				bf->disasm_config.insn_type == dis_jsr ||
				bf->disasm_config.insn_type == dis_branch) {
			/*
			 * For dis_jsr, we should consider adding the target to
			 * a function list for function enumeration. But for
			 * now, we treat a CALL the same as a conditional
			 * branch.
			 */
			bfd_vma branch_vma = bf->disasm_config.target;

			if(bf->disasm_config.insn_type != dis_branch) {
				struct bf_basic_blk * bb_next =
						disasm_block(bf,
						vma + size);
				bf_add_next_basic_blk(bb, bb_next);
			}

			if(branch_vma != 0) {
				struct bf_basic_blk * bb_branch =
						disasm_block(bf,
						branch_vma);
				bf_add_next_basic_blk(bb, bb_branch);
			}

			bf->disasm_config.insn_type = dis_condjsr;
		}

		vma += size;
	}

	return bb;
}

struct bf_basic_blk * disasm_generate_cflow(struct binary_file * bf, bfd_vma vma)
{
	return disasm_block(bf, vma);
}

struct bf_basic_blk * disasm_from_sym(struct binary_file * bf, asymbol * sym)
{
	symbol_info info;

	bfd_symbol_info(sym, &info);
	return disasm_generate_cflow(bf, info.value);
}
