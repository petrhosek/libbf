#include "binary_file.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

/*
 * Example usage of the visitor pattern used for binary_file_for_each_symbol.
 */
void process_symbol(binary_file * bf, asymbol * sym)
{
	if(strcmp(sym->name, "func1") == 0) {
		disassemble_binary_file_symbol(bf, sym);
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

		strncat(target_path, "/Target/Target_x86-64", size - strlen(target_path) - 1);
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
	binary_file * bf;

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

	/* if(!disassemble_binary_file_entry(bf)) {
		perror("Failed to disassemble binary_file");
	}*/

	if(!binary_file_for_each_symbol(bf, process_symbol)) {
		perror("Failed during enumeration of symbols");
	}

	if(!close_binary_file(bf)) {
		perror("Failed to close binary_file");
		xexit(-1);
	}

	return EXIT_SUCCESS;
}
