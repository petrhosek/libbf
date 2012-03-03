#include "bf_disasm.h"
#include "bf_insn_decoder.h"
#include "bf_func.h"
#include "bf_basic_blk.h"
#include "bf_mem_manager.h"

static struct bf_basic_blk * disasm_block(struct binary_file * bf, bfd_vma vma);

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

static inline void strip_trailing_spaces(char * str, size_t size) {
	int i;

	for(i = 0; i < size; i++) {
		if(!isgraph(str[i])) {
			str[i] = '\0';
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
	strip_trailing_spaces(str, ARRAY_SIZE(str));

	switch(bf->context.part_counter) {
	case 0: {
		if(is_mnemonic(str)) {
			bf_set_insn_mnemonic(bf->context.insn, str);
		} else if(strcmp(str, "data32") != 0) {
			printf("%s mnemonic not enumerated by libind\n", str);
		}

		break;
	}
	case 1: {
		if(!is_operand(str)) {
			printf("%s operand not enumerated by libind\n", str);
		}

		break;
	}
	default:
		break;
	}

	bf->context.part_counter++;
	return rv;
}

static unsigned int disasm_single_insn(struct binary_file * bf, bfd_vma vma)
{
	bf->disasm_config.insn_info_valid = 0;
	bf->disasm_config.target	  = 0;
	bf->context.part_counter	  = 0;
	return bf->disassembler(vma, &bf->disasm_config);
}

static struct bf_basic_blk * split_block(struct binary_file * bf, bfd_vma vma)
{
	struct bf_basic_blk * bb     = bf_split_blk(bf,
			bf_get_insn(bf, vma)->bb, vma);
	struct bf_insn *      insn   = list_entry(bb->part_list.prev,
			struct bf_basic_blk_part, list)->insn;
	int		      size;
	bfd_vma		      target;

	/*
	 * Use a dummy instruction as the context to prevent a real one being
	 * modified.
	 */
	bf->context.insn = bf_init_insn(bb, vma);
	size = disasm_single_insn(bf, insn->vma);
	bf_close_insn(bf->context.insn);

	target = bf->disasm_config.target;

	bf_add_bb(bf, bb);

	if(bf->disasm_config.insn_type != dis_branch) {
		bf_add_next_basic_blk(bb, disasm_block(bf, insn->vma + size));
	}

	if(target) {
		bf_add_next_basic_blk(bb, disasm_block(bf, target));
	}

	return bb;
}

static struct bf_func * add_new_func(struct binary_file * bf,
		struct bf_basic_blk * bb, bfd_vma vma)
{
	if(bf_exists_func(bf, vma)) {
		return bf_get_func(bf, vma);
	} else {
		struct bf_func * func = bf_init_func(bf, bb, vma);
		bf_add_func(bf, func);
		return func;
	}
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
		return split_block(bf, vma);
	} else {
		bb = bf_init_basic_blk(bf, vma);
		bf_add_bb(bf, bb);
	}

	bf->disasm_config.insn_type = dis_noninsn;

	while(bf->disasm_config.insn_type != dis_condjsr) {
		int size;

		if(bf_exists_insn(bf, vma)) {
			if(!bf_exists_bb(bf, vma)) {
				printf("The current basic block (0x%lX) is \
						overlapping to the \
						instruction at 0x%lX \
						which has already been \
						disassembled but is not \
						the start of an existing \
						basic block", bb->vma, vma);
			} else {
				struct bf_basic_blk * bb_next =
						bf_get_bb(bf, vma);
				bf_add_next_basic_blk(bb, bb_next);
				return bb;
			}
		}

		bf->context.insn = bf_init_insn(bb, vma);
		bf_add_insn(bf, bf->context.insn);
		bf_add_insn_to_bb(bb, bf->context.insn);

		size = disasm_single_insn(bf, vma);

		if(size == -1 || size == 0) {
			puts("Something went wrong");
			return NULL;
		}

		// printf("Disassembled %d bytes at 0x%lX\n\n", size, vma);

		if(bf->disasm_config.insn_type == dis_condbranch ||
				bf->disasm_config.insn_type == dis_jsr ||
				bf->disasm_config.insn_type == dis_branch) {
			enum dis_insn_type insn_type   =
					bf->disasm_config.insn_type;
			bfd_vma		   branch_vma  =
					bf->disasm_config.target;

			if(insn_type != dis_branch) {
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

				if(insn_type == dis_jsr) {
					/*
					 * Putting the intialisation of bf_func
					 * here means that we will never detect
					 * the first basic block as a function,
					 * only subsequent call targets.
					 */
					add_new_func(bf, bb_branch, branch_vma);
				}
			}

			bf->disasm_config.insn_type = dis_condjsr;
		}

		vma += size;
	}

	return bb;
}

struct bf_basic_blk * disasm_generate_cflow(struct binary_file * bf,
		bfd_vma vma, bool is_function)
{
	struct bf_basic_blk * bb = disasm_block(bf, vma);

	if(is_function) {
		add_new_func(bf, bb, vma);
	}

	return bb;
}

struct bf_basic_blk * disasm_from_sym(struct binary_file * bf, asymbol * sym,
		bool is_function)
{
	symbol_info info;

	bfd_symbol_info(sym, &info);
	return disasm_generate_cflow(bf, info.value, is_function);
}
