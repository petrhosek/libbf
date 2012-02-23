#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "bf_insn.h"
#include "bf_basic_blk.h"
#include "bf_func.h"
#include "bf_cfg.h"

/*
 * Currently we are generating graph.dot into the current directory.
 */
bool get_dot_path(char * target_path, size_t size)
{
	if(getcwd(target_path, size) == NULL) {
		return FALSE;
	} else {
		strncat(target_path, "/graph.dot",
				size - strlen(target_path) - 1);
		return TRUE;
	}
}

/*
 * Creates a file and dumps the CFG to it in .dot format.
 * The result should be compiled with "dot -Tps graph.dot -o graph.pdf"
 */
void create_cfg_dot(struct bf_basic_blk * bb)
{
	char target_path[FILENAME_MAX] = {0};

	if(get_dot_path(target_path, ARRAY_SIZE(target_path))) {
		FILE * stream = fopen(target_path, "w+");
		print_cfg_dot(stream, bb);
		fclose(stream);
	} else {
		puts("Failed to get path for dot file.");
	}
}

/*
 * Testing print_entire_cfg_dot.
 */
void dump_cfg(struct binary_file * bf)
{
	char target_path[FILENAME_MAX] = {0};

	if(get_dot_path(target_path, ARRAY_SIZE(target_path))) {
		FILE * stream = fopen(target_path, "w+");
		print_entire_cfg_dot(bf, stream);
		fclose(stream);
	} else {
		puts("Failed to get path for dot file.");
	}
}

/*
 * Example usage of the visitor pattern used for bf_for_each_insn_part.
 */
void process_insn_part(struct bf_insn * insn, char * str,
		void * param)
{
	printf("%s", str);
}

/*
 * Example usage of the visitor pattern used for bf_for_each_insn.
 */
void process_each_insn(struct binary_file * bf, struct bf_insn * insn,
		void * param)
{
	bf_for_each_insn_part(insn, process_insn_part, NULL);
	printf("\n");
}

/*
 * Example usage of the visitor pattern used for bf_for_each_basic_blk_insn.
 */
void process_insn(struct bf_basic_blk * bb, struct bf_insn * insn,
		void * param)
{
	bf_print_insn(insn);
	printf("\n");
}

/*
 * Example usage of the visitor pattern used for bf_for_each_basic_blk.
 */
void process_bb(struct binary_file * bf, struct bf_basic_blk * bb,
		void * param)
{
	bf_for_each_basic_blk_insn(bb, process_insn, NULL);
}

/*
 * Example usage of the visitor pattern used for bf_for_each_symbol.
 */
void process_symbol(struct binary_file * bf, asymbol * sym,
		void * param)
{
	if(strcmp(sym->name, "main") == 0) {
		struct bf_basic_blk * bb = disassemble_binary_file_symbol(bf,
				sym, TRUE);

		// print_cfg_stdout(bb);
		// create_cfg_dot(bb);
		// bf_for_each_basic_blk(bf, process_bb, NULL);
		// bf_for_each_insn(bf, process_each_insn, NULL);
	}
}

/*
 * Example usage of the visitor pattern used for bf_for_each_func.
 */
void process_func(struct binary_file * bf, struct bf_func * func,
		void * param)
{
	printf("Function at 0x%lX (%s)\n", func->vma,
			func->sym == NULL ?
			"No symbol information" :
			func->sym->name);
}

/*
 * Currently we are hardcoding the target path based off of the relative path
 * from this executable. This is merely as a convenience for testing.
 */
bool get_target_path(char * target_path, size_t size)
{
	if(getcwd(target_path, size) == NULL) {
		return FALSE;
	} else {
		int target_desc;

/*		strncat(target_path, "/Target/Target_x86-64", size -
				strlen(target_path) - 1);*/
		strncat(target_path, "/coreutils/[", size -
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

int main(void)
{
	struct binary_file *  bf;
	// struct bf_basic_blk * bb;

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
	/*bb = disassemble_binary_file_entry(bf);

	if(!bb) {
		perror("Failed to disassemble binary_file");
	}

	create_cfg_dot(bb);*/

	if(!bf_for_each_symbol(bf, process_symbol, NULL)) {
		perror("Failed during enumeration of symbols");
	}

	bf_for_each_func(bf, process_func, NULL);
	disassemble_binary_file_entry(bf);

	dump_cfg(bf);

	puts("The basic block is:");
	bf_print_basic_blk(bf_get_bb(bf, 0x401715));

	if(!close_binary_file(bf)) {
		perror("Failed to close binary_file");
		xexit(-1);
	}

	return EXIT_SUCCESS;
}



