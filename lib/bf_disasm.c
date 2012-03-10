#include "bf_disasm.h"
#include "bf_insn_decoder.h"
#include "bf_func.h"
#include "bf_basic_blk.h"
#include "bf_mem_manager.h"

/*
 * Forward reference.
 */
static struct bf_basic_blk * disasm_block(struct binary_file * bf, bfd_vma vma);

static void update_insn_info(struct binary_file * bf, struct bf_insn * insn, char * str)
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

		if(breaks_flow(insn->mnemonic)) {
			bf->disasm_config.insn_type = dis_branch;
		} else if(branches_flow(insn->mnemonic)) {
			bf->disasm_config.insn_type = dis_condbranch;
		} else if(calls_subroutine(insn->mnemonic)) {
			bf->disasm_config.insn_type = dis_jsr;
		} else if(ends_flow(insn->mnemonic)) {
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
				if(insn->operand1.tag == OP_VAL) {
					bf->disasm_config.target =
							insn->operand1
							.operand_info.val;
				}

				break;
			default:
				break;
			}
		}
	}
}

static bool parse_mnemonic(struct disasm_context * context, char * str)
{
	if(is_mnemonic(str)) {
		bf_set_insn_mnemonic(context->insn, str);

		/*
		 * Next pass no longer expects mnemonic.
		 */
		context->part_types_expected ^= insn_part_mnemonic;

		/*
		 * Except for the special case of a macro mnemonic.
		 */
		if(is_macro_mnemonic(str)) {
			context->part_types_expected |=
					insn_part_secondary_mnemonic;
		/*
		 * Otherwise an operand is expected.
		 */
		} else {
			context->part_types_expected |=
					insn_part_operand;
		}

		return TRUE;
	}

	return FALSE;
}

static bool parse_secondary_mnemonic(struct disasm_context * context, char * str)
{
	if(is_mnemonic(str)) {
		bf_set_insn_secondary_mnemonic(context->insn, str);

		/*
		 * Next pass no longer expects secondary mnemonic.
		 */
		context->part_types_expected ^=
				insn_part_secondary_mnemonic;

		/*
		 * Always expect operand instead.
		 */
		context->part_types_expected |= insn_part_operand;
		return TRUE;
	}

	return FALSE;
}

static bool parse_operand(struct disasm_context * context, char * str)
{
	if(is_operand(str)) {
		/*
		 * Find out how many operands have been stored in the
		 * bf_insn already.
		 */
		int ops_already = bf_get_insn_num_operands(
				context->insn);

		bf_add_insn_operand(context->insn, str);

		/*
		 * Next pass no longer expects an operand.
		 */
		context->part_types_expected ^= insn_part_operand;

		/*
		 * If the bf_insn already holds 3 operands, we can only
		 * expect a comment indicator at the next pass.
		 * Otherwise, we can additionally expect a comma to
		 * indicate more operands.
		 */
		context->part_types_expected =
				insn_part_comment_indicator;

		if(ops_already < 3) {
			context->part_types_expected |=
					insn_part_comma;
		}

		return TRUE;
	}

	return FALSE;
}

static bool parse_comma(struct disasm_context * context, char * str)
{
	if(str[0] != '\0' && str[0] == ',') {
		/*
		 * Next pass no longer expects a comma.
		 */
		context->part_types_expected ^=	insn_part_comma;

		/*
		 * Expect another operand.
		 */
		context->part_types_expected |= insn_part_operand;
		return TRUE;
	}

	return FALSE;
}

static bool parse_comment_indicator(struct disasm_context * context, char * str)
{
	if(str[0] == '\0') {
		/*
		 * Next pass no longer expects a comment indicator.
		 */
		context->part_types_expected ^=
				insn_part_comment_indicator;

		/*
		 * Expect the comment contents. Clear all other flags.
		 */
		context->part_types_expected =
				insn_part_comment_contents;
		return TRUE;
	}

	return FALSE;
}

static bool parse_comment_contents(struct disasm_context * context, char * str)
{
	bfd_vma extra_info = get_vma_target(str);
	if(extra_info != 0) {
		bf_set_insn_extra_info(context->insn, extra_info);

		/*
		 * Not expecting a next pass at all.
		 */
		context->part_types_expected = 0;
		return TRUE;
	}

	return FALSE;
}

static bool parse_insn_info(struct disasm_context * context, char * str)
{
	if(context->part_types_expected & insn_part_mnemonic) {
		if(parse_mnemonic(context, str)) {
			return TRUE;
		}
	}

	if(context->part_types_expected &
			insn_part_secondary_mnemonic) {
		if(parse_secondary_mnemonic(context, str)) {
			return TRUE;
		}
	}

	if(context->part_types_expected & insn_part_operand) {
		if(parse_operand(context, str)) {
			return TRUE;
		}
	}

	if(context->part_types_expected & insn_part_comma) {
		if(parse_comma(context, str)) {
			return TRUE;
		}
	}

	if(context->part_types_expected & insn_part_comment_indicator) {
		if(parse_comment_indicator(context, str)) {
			return TRUE;
		}
	}

	if(context->part_types_expected & insn_part_comment_contents) {
		if(parse_comment_contents(context, str)) {
			return TRUE;
		}
	}

	return FALSE;
}

static void strip_trailing_spaces(char * str, size_t size) {
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

	strip_trailing_spaces(str, ARRAY_SIZE(str));
	
	if(bf->context.part_counter == 0 && strcmp(str, "data32") == 0) {
		bf_set_is_data(bf->context.insn, TRUE);
	}

	/*
	 * parse_insn_info uses the bf->context.part_types_expected to keep a
	 * state machine of expected part types
	 * (e.g. mnemonics, operands, etc.).
	 */
	if(!bf->context.insn->is_data) {
		if(!parse_insn_info(&bf->context, str)) {
			printf("parse_insn_info returned FALSE for 0x%lX. "\
					"The str was %s. The current "\
					"instruction is:\n\t",
					bf->context.insn->vma, str);
			bf_print_insn(bf->context.insn);
			printf("\n\n");
		}
	}

	update_insn_info(bf, bf->context.insn, str);
	bf->context.part_counter++;
	return rv;
}

static unsigned int disasm_single_insn(struct binary_file * bf, bfd_vma vma)
{
	bf->disasm_config.insn_info_valid = 0;
	bf->disasm_config.target	  = 0;

	bf->context.part_counter	= 0;
	bf->context.part_types_expected = insn_part_mnemonic;
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
