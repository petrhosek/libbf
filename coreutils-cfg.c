#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include "bf_insn.h"
#include "bf_basic_blk.h"
#include "bf_func.h"
#include "bf_cfg.h"

/*
 * Gets the current directory.
 */
bool get_root_folder(char * path, size_t size)
{
	return getcwd(path, size) != NULL;
}

/*
 * Currently we are hardcoding the target path based off of the relative path
 * from this executable. This is merely as a convenience for testing.
 */
bool get_target_folder(char * path, size_t size)
{
	if(!get_root_folder(path, size)) {
		return FALSE;
	} else {
		int target_desc;

		strncat(path, "/coreutils", size -
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
 * Generates the output dot.
 */
void create_entire_cfg_dot(struct binary_file * bf, char * output)
{
	FILE * stream = fopen(output, "w+");
	print_entire_cfg_dot(bf, stream);
	fclose(stream);
}

/*
 * Using the visitor pattern to locate main and start disassembling from there.
 */
void process_symbol(struct binary_file * bf, asymbol * sym, void * param)
{
	if(strcmp(sym->name, "main") == 0) {
		disassemble_binary_file_symbol(bf, sym, TRUE);
	}
}

/*
 * Run a test on an individual target. This attempts the generation of a CFG.
 * Error checking omitted for brevity.
 */
void run_test(char * target, char * output)
{
	struct binary_file * bf  = load_binary_file(target);
	printf("Disassembling %s\n", target);

	/*
	 * Disassemble main.
	 */
	bf_for_each_symbol(bf, process_symbol, NULL);

	/*
	 * Also disassemble entry point. This should result in multiple roots.
	 */
	disassemble_binary_file_entry(bf);

	create_entire_cfg_dot(bf, output);
	close_binary_file(bf);
}

/*
 * Get all the test files and run tests against them.
 */
void enumerate_files_and_run_tests(char * root, char * target_folder)
{
	DIR *		d;
	struct dirent * dir;

	d = opendir(target_folder);
	if(d) {
		while((dir = readdir(d)) != NULL) {
			if(!(strcmp(dir->d_name, ".") == 0) &&
					!(strcmp(dir->d_name, "..") == 0)) {
				char * output_relative = "/tests/";
				char * extension       = ".dot";
				char * target = xmalloc(strlen(target_folder) +
						strlen(dir->d_name) + 2);
				char * output = xmalloc(strlen(root) +
						strlen(output_relative) +
						strlen(dir->d_name) +
						strlen(extension)+ 1);

				strcpy(target, target_folder);
				strcat(target, "/");
				strcat(target, dir->d_name);

				strcpy(output, root);
				strcat(output, output_relative);
				strcat(output, dir->d_name);
				strcat(output, extension);

				run_test(target, output);

				free(target);
			}
		}

		closedir(d);
	}
}

int main(void)
{
	char target_folder[FILENAME_MAX] = {0};
	char root[FILENAME_MAX]		 = {0};

	if(!get_target_folder(target_folder, ARRAY_SIZE(target_folder))) {
		perror("Failed to get path of folder");
		xexit(-1);
	}

	if(!get_root_folder(root, ARRAY_SIZE(root))) {
		perror("Failed to get root");
		xexit(-1);
	}

	enumerate_files_and_run_tests(root, target_folder);
	return EXIT_SUCCESS;
}
