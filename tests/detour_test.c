#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/stat.h> 
#include <fcntl.h>

#include <bf_detour.h>
#include <bf_cfg.h>

/*
 * Gets the current directory.
 */
bool get_root_folder(char * path, size_t size)
{
	char * dir = getenv("TEST_BUILD_DIR");
	if (!dir)
		return FALSE;

	strncpy(path, dir, size);
	return TRUE;
}

/*
 * Currently we are hardcoding the target path based off of the relative path
 * from this executable. This is merely as a convenience for testing.
 */
bool get_output_doc_path(char * path, size_t size)
{
	if(!get_root_folder(path, size)) {
		return FALSE;
	} else {
		int target_desc;

		strncat(path, "/detour_test.dot", size -
				strlen(path) - 1);
		target_desc = open(path, O_RDONLY);

		if(target_desc == -1) {
			return FALSE;
		} else {
			close(target_desc);
			return TRUE;
		}
	}
}

/*
 * Gets path to currently running executable.
 */
bool get_target_path(char * target_path, size_t size)
{
	if(!get_root_folder(target_path, size)) {
		return FALSE;
	} else {
		int target_desc;

		strncat(target_path, "/detour_targets/detour_target_32", size -
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

/*
 * Using the visitor pattern to locate the functions we care about and generate
 * CFG using those places as roots.
 */
void process_symbol(struct binary_file * bf, asymbol * sym, void * param)
{
	if(strcmp(sym->name, "main") == 0 ||
			strcmp(sym->name, "func1") == 0 ||
			strcmp(sym->name, "func2") == 0) {
		disassemble_binary_file_symbol(bf, sym, TRUE);
	}
}

/*
 * Generates the output dot.
 */
void create_entire_cfg_dot(struct binary_file * bf, char * output)
{
	FILE * stream = fopen(output, "w+");
	print_entire_cfg_dot(bf, stream);
	fclose(stream);
}

/*
 * Generate disassembly for the functions we are interested in.
 */
void gen_disasm(struct binary_file * bf)
{
	bf_for_each_symbol(bf, process_symbol, NULL);
}

/*
 * Dump disasm to .dot file.
 */
void dump_disasm(struct binary_file * bf)
{
	char output[FILENAME_MAX] = {0};
	get_output_doc_path(output, ARRAY_SIZE(output));

	create_entire_cfg_dot(bf, output);

	if(get_root_folder(output, ARRAY_SIZE(output))) {
		char * cd_cmd  = "cd ";
		char * gen_pdf = "; dot -Tpdf detour_test.dot -o "\
				"detour_test.pdf";
		char cmd[strlen(cd_cmd) + strlen(output) +
				strlen(gen_pdf) + 1];

		strcpy(cmd, cd_cmd);
		strcat(cmd, output);
		strcat(cmd, gen_pdf);

		if(system(cmd)) {
			perror("Failed generating detour_test.pdf");
			xexit(-1);
		}
	}
}

/*
 * The aim of this program is to detour execution so func2 is called instead of
 * func1. It is an elementary test for the basic functionality of bf_detour.
 */
void patch_func1_func2(void)
{
	struct binary_file * bf			   = NULL;
	struct bf_func *     bf_func1		   = NULL;
	struct bf_func *     bf_func2		   = NULL;
	char		     target_path[PATH_MAX] = {0};

	/* Get path of the target program */
	if(!get_target_path(target_path, ARRAY_SIZE(target_path))) {
		perror("Unable to find detour_target_32. Run 'make all' in "\
				"tests/detour_targets");
		xexit(-1);
	}

	bf = load_binary_file(target_path);
	printf("binary = %s\n", target_path);
	gen_disasm(bf);

	dump_disasm(bf);

	bf_func1 = bf_get_func_from_name(bf, "func1");
	bf_func2 = bf_get_func_from_name(bf, "func2");

	if(bf_func1 == NULL || bf_func2 == NULL) {
		perror("Unable to locate func1 or func2 through disassembly.");
		xexit(-1);
	}

	bf_detour_func(bf, bf_func1, bf_func2);
	close_binary_file(bf);
}

int main(int argc, char *argv[])
{
	patch_func1_func2();
	return EXIT_SUCCESS;
}
