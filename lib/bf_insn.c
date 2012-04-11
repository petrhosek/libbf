#include "bf_insn.h"

struct bf_insn * bf_init_insn(struct bf_basic_blk * bb, bfd_vma vma)
{
	struct bf_insn * insn	 = xmalloc(sizeof(struct bf_insn));
	insn->vma		 = vma;
	insn->bb		 = bb;
	insn->mnemonic		 = 0;
	insn->secondary_mnemonic = 0;
	insn->extra_info	 = 0;
	insn->is_data		 = FALSE;

	memset(&insn->operand1, '\0', sizeof(insn->operand1));
	memset(&insn->operand2, '\0', sizeof(insn->operand2));
	memset(&insn->operand3, '\0', sizeof(insn->operand3));
	INIT_LIST_HEAD(&insn->part_list);
	return insn;
}

void bf_add_insn_part(struct bf_insn * insn, char * str)
{
	struct bf_insn_part * part = xmalloc(sizeof(struct bf_insn_part));
	part->str		   = xstrdup(str);

	INIT_LIST_HEAD(&part->list);
	list_add_tail(&part->list, &insn->part_list);
}

void bf_set_insn_mnemonic(struct bf_insn * insn, char * str)
{
	insn->mnemonic = 0;
	strncpy((char *)&insn->mnemonic, str, sizeof(uint64_t));

	switch(insn->mnemonic) {
	/*
	 * The one special case where the first 8 characters do not represent
	 * a unique instruction.
	 */
	case cvtsi2sd_insn:
		if(str[8] == 'q') {
			insn->mnemonic = cvtsi2sdq_insn;
		}

		break;
	default:
		break;
	}
}

void bf_set_insn_secondary_mnemonic(struct bf_insn * insn, char * str)
{
	insn->secondary_mnemonic = 0;
	strncpy((char *)&insn->secondary_mnemonic, str, sizeof(uint64_t));
}

void bf_set_insn_operand(struct bf_insn * insn, char * str)
{
	set_operand_info(&insn->operand1, str);
}

void bf_set_insn_operand2(struct bf_insn * insn, char * str)
{
	set_operand_info(&insn->operand2, str);
}

void bf_set_insn_operand3(struct bf_insn * insn, char * str)
{
	set_operand_info(&insn->operand3, str);
}

void bf_add_insn_operand(struct bf_insn * insn, char * str)
{
	switch(bf_get_insn_num_operands(insn)) {
	case 0:
		bf_set_insn_operand(insn, str);
		break;
	case 1:
		bf_set_insn_operand2(insn, str);
		break;
	case 2:
		bf_set_insn_operand3(insn, str);
		break;
	default:
		printf("Attempted to add more than 3 operands to an "\
				"instruction: %s\n", str);
		break;
	}
}

void bf_set_insn_extra_info(struct bf_insn * insn, bfd_vma vma)
{
	insn->extra_info = vma;
}

int bf_get_insn_num_operands(struct bf_insn * insn)
{
	if(insn->operand3.tag != 0) {
		return 3;
	} else if(insn->operand2.tag != 0) {
		return 2;
	} else if(insn->operand1.tag != 0) {
		return 1;
	} else {
		return 0;
	}
}

void bf_set_is_data(struct bf_insn * insn, bool is_data)
{
	insn->is_data = is_data;
}

void bf_print_insn(struct bf_insn * insn)
{
	if(insn != NULL) {
		struct bf_insn_part * pos;

		list_for_each_entry(pos, &insn->part_list, list) {
			printf("%s", pos->str);
		}
	}
}

void bf_print_insn_to_file(FILE * stream, struct bf_insn * insn)
{
	if(insn != NULL) {
		struct bf_insn_part * pos;

		list_for_each_entry(pos, &insn->part_list, list) {
			fprintf(stream, "%s", pos->str);
		}
	}
}

void bf_print_insn_semantic_gen_to_file(FILE * stream, struct bf_insn * insn,
		enum arch_bitiness bitiness)
{
	/*
	 * This is a horrible hack to make our output match the one from
	 * objdump.
	 */
	if(is_jmp_or_call(insn->mnemonic) &&
			insn->operand1.tag != OP_ADDR_PTR) {
		bitiness = arch_64;
	}

	if(insn != NULL) {
		if(insn->mnemonic != 0) {
			print_mnemonic_to_file(stream, insn->mnemonic);
		}

		if(insn->secondary_mnemonic != 0) {
			print_mnemonic_to_file(stream,
				insn->secondary_mnemonic);
		}

		if(insn->operand1.tag != 0) {
			print_operand_to_file(stream, &insn->operand1,
					bitiness == arch_64);
		}

		if(insn->operand2.tag != 0) {
			fprintf(stream, ",");
			print_operand_to_file(stream, &insn->operand2,
					bitiness == arch_64);
		}

		if(insn->operand3.tag != 0) {
			fprintf(stream, ",");
			print_operand_to_file(stream, &insn->operand3,
					bitiness == arch_64);
		}

		if(insn->extra_info != 0) {
			fprintf(stream, "#");
			print_comment_to_file(stream, insn->extra_info);
		}
	}
}

void bf_print_insn_dot(FILE * stream, struct bf_insn * insn)
{
	if(insn != NULL) {
		struct bf_insn_part * pos;

		list_for_each_entry(pos, &insn->part_list, list) {
			fprintf(stream, "%s", pos->str);
		}

		fprintf(stream, "\\l\\n");
	}
}

void bf_close_insn(struct bf_insn * insn)
{
	if(insn != NULL) {
		struct bf_insn_part * pos;
		struct bf_insn_part * n;

		list_for_each_entry_safe(pos, n, &insn->part_list, list) {
			list_del(&pos->list);
			free(pos->str);
			free(pos);
		}

		free(insn);
	}
}

void bf_add_insn(struct binary_file * bf, struct bf_insn * insn)
{
	assert(!bf_exists_insn(bf, insn->vma));

	htable_add(&bf->insn_table, &insn->entry, &insn->vma, sizeof(insn->vma));
}

struct bf_insn * bf_get_insn(struct binary_file * bf, bfd_vma vma)
{
	return hash_find_entry(&bf->insn_table, &vma, sizeof(vma),
			struct bf_insn, entry);
}

bool bf_exists_insn(struct binary_file * bf, bfd_vma vma)
{
	return htable_find(&bf->insn_table, &vma, sizeof(vma)) != NULL;
}

void bf_close_insn_table(struct binary_file * bf)
{
	struct htable_entry * cur_entry;
	struct htable_entry * n;
	struct bf_insn *      insn;

	htable_for_each_entry_safe(insn, cur_entry, n, &bf->insn_table, entry) {
		htable_del_entry(&bf->insn_table, cur_entry);
		bf_close_insn(insn);
	}	
}

void bf_enum_insn(struct binary_file * bf,
		void (*handler)(struct binary_file *, struct bf_insn *,
		void *), void * param)
{
	struct htable_entry * cur_entry;
	struct bf_insn *      insn;

	htable_for_each_entry(insn, cur_entry, &bf->insn_table, entry) {
		handler(bf, insn, param);
	}
}

void bf_enum_insn_part(struct bf_insn * insn,
		void (*handler)(struct bf_insn *, char *, void *),
		void * param)
{
	struct bf_insn_part * pos;

	list_for_each_entry(pos, &insn->part_list, list) {
		handler(insn, pos->str, param);
	}
}
