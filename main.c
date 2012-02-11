#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "bf_insn.h"
#include "bf_basic_blk.h"
#include "bf_cfg.h"

/*
 * Example usage of the visitor pattern used for binary_file_for_each_symbol.
 */
void process_symbol(struct binary_file * bf, asymbol * sym)
{
	if(strcmp(sym->name, "main") == 0) {
		struct bf_basic_blk * bb = disassemble_binary_file_symbol(bf, sym);
		FILE *		      stream;

		print_cfg_stdout(bb);
	
		/* Compile the DOT with "dot -Tps graph.dot -o graph.pdf" */
		stream = fopen("/home/mike/Desktop/Linux-Static-Detouring/graph.dot",
				"w+");
		print_cfg_dot(stream, bb);
		fclose(stream);
		/* Write some function to free the CFG */
	}
}

/*
 * Currently we are hardcoding the target path based off of the relative path
 * from this executable. This is merely as a convenience for testing.
 */
bool get_target_path(char* target_path, size_t size)
{
	if(getcwd(target_path, size) == NULL) {
		return FALSE;
	} else {
		int target_desc;

		strncat(target_path, "/Target/Target_x86-64", size -
				strlen(target_path) - 1);
		target_desc = open(target_path, O_RDONLY);

		if(target_desc == -1) {
			return FALSE;
		} else {
			close(target_desc);
			return TRUE;
		}
	}
}

void test_bf_basic_blk(struct binary_file * bf)
{
	struct bf_basic_blk * bb    = init_bf_basic_blk(0);
	struct bf_insn *      insn  = init_bf_insn(0);
	struct bf_insn *      insn2 = init_bf_insn(0);
	struct bf_insn *      insn3 = init_bf_insn(0);
	struct bf_insn *      insn4 = init_bf_insn(0);

	add_insn_part(insn, "mov");
	add_insn_part(insn, "edi");
	add_insn_part(insn, ",");
	add_insn_part(insn, "edi");

	add_insn_part(insn2, "push");
	add_insn_part(insn2, "ebp");

	add_insn_part(insn3, "mov");
	add_insn_part(insn3, "ebp");
	add_insn_part(insn3, ",");
	add_insn_part(insn3, "esp");

	add_insn_part(insn4, "nop");

	add_insn(bb, insn);
	add_insn(bb, insn2);
	add_insn(bb, insn3);
	add_insn(bb, insn4);

	print_bf_basic_blk(bb);
	close_bf_basic_blk(bb);
}

int main(void)
{
	struct binary_file * bf;

	char target_path[FILENAME_MAX] = {0};
	if(!get_target_path(target_path, ARRAY_SIZE(target_path))) {
		perror("Failed to get location of target");
		xexit(-1);
	}

	bf = load_binary_file(target_path);

	if(!bf) {
		perror("Failed loading binary_file");
		xexit(-1);
	}

	/*
	 * Disassembling from entry is not necessarily useful. For example,
	 * if main is called indirectly through __libc_start_main.
	 */
	/*if(!disassemble_binary_file_entry(bf)) {
		perror("Failed to disassemble binary_file");
	}*/

	if(!binary_file_for_each_symbol(bf, process_symbol)) {
		perror("Failed during enumeration of symbols");
	}

	// test_bf_basic_blk(bf);

	if(!close_binary_file(bf)) {
		perror("Failed to close binary_file");
		xexit(-1);
	}

	return EXIT_SUCCESS;
}



