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
 * Gets path to target program.
 */
bool get_target_path(char * target_path, size_t size, char * bitiness)
{
	if(!get_root_folder(target_path, size)) {
		return FALSE;
	} else {
		int target_desc;

		if(strcmp(bitiness, "32") == 0) {
			strncat(target_path,
					"/detour_targets/detour_target_32",
					size - strlen(target_path) - 1);
		} else {
			strncat(target_path,
					"/detour_targets/detour_target_64",
					size - strlen(target_path) - 1);
		}
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
 * Gets folder to put output into.
 */
bool get_output_folder(char * output_folder, size_t size, char * bitiness)
{
	if(!get_root_folder(output_folder, size)) {
		return FALSE;
	} else {
		if(strcmp(bitiness, "32") == 0) {
			strncat(output_folder, "/tests-detour-output32",
					size - strlen(output_folder) - 1);
		} else {
			strncat(output_folder, "/tests-detour-output64",
					size - strlen(output_folder) - 1);
		}

		return TRUE;
	}
}

/*
 * Gets output path.
 */
bool get_output_path(char * output_path, size_t size, char * bitiness)
{
	if(!get_output_folder(output_path, size, bitiness)) {
		return FALSE;
	} else {
		strcat(output_path, "/detour_target");
		return TRUE;
	}
}

/*
 * Currently we are hardcoding the target path based off of the relative path
 * from this executable. This is merely as a convenience for testing.
 */
bool get_output_doc_path(char * path, size_t size, char * bitiness)
{
	if(!get_output_folder(path, size, bitiness)) {
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
void dump_disasm(struct binary_file * bf, char * bitiness)
{
	char output[FILENAME_MAX] = {0};
	get_output_doc_path(output, ARRAY_SIZE(output), bitiness);

	create_entire_cfg_dot(bf, output);

	if(get_output_folder(output, ARRAY_SIZE(output), bitiness)) {
		char * cd_cmd  = "cd ";
		char * gen_pdf = "; dot -Tpdf detour_test.dot -o "\
				"detour_test.pdf";
		char cmd[strlen(cd_cmd) + strlen(output) +
				strlen(gen_pdf) + 1];

		strcpy(cmd, cd_cmd);
		strcat(cmd, output);
		strcat(cmd, gen_pdf);

		printf("cmd = %s\n", cmd);

		if(system(cmd)) {
			perror("Failed generating detour_test.pdf");
			xexit(-1);
		}
	}
}

void create_fresh_output_folder(char * bitiness)
{
	char output_folder[PATH_MAX] = {0};

	if(!get_output_folder(output_folder,
			ARRAY_SIZE(output_folder), bitiness)) {
		perror("Unable to get target folder.");
		xexit(-1);
	} else {
		char * cmd1 = "rm -rf ";
		char * cmd2 = "; mkdir ";
		char   create_fresh_folder[strlen(cmd1) +
				strlen(output_folder) +
				strlen(cmd2) +
				strlen(output_folder) + 1];

		strcpy(create_fresh_folder, cmd1);
		strcat(create_fresh_folder, output_folder);
		strcat(create_fresh_folder, cmd2);
		strcat(create_fresh_folder, output_folder);

		if(system(create_fresh_folder)) {
			perror("Problem creating fresh output folder.");
			xexit(-1);
		}
	}
}

/*
 * The aim of this program is to detour execution so func2 is called instead of
 * func1. It is an elementary test for the basic functionality of bf_detour.
 */
void patch_func1_func2(char * bitiness)
{
	struct binary_file * bf			   = NULL;
	struct bf_func *     bf_func1		   = NULL;
	struct bf_func *     bf_func2		   = NULL;
	char		     target_path[PATH_MAX] = {0};
	char		     output_path[PATH_MAX] = {0};

	/* Get path of the target program */
	if(!get_target_path(target_path, ARRAY_SIZE(target_path), bitiness)) {
		perror("Unable to find detour target.");
		xexit(-1);
	}

	/* Get path of the output program */
	if(!get_output_path(output_path, ARRAY_SIZE(output_path), bitiness)) {
		perror("Unable to get path of output.");
		xexit(-1);
	}

	create_fresh_output_folder(bitiness);

	printf("target = %s, output = %s\n", target_path, output_path);
	bf = load_binary_file(target_path, output_path);
	gen_disasm(bf);

	dump_disasm(bf, bitiness);

	bf_func1 = bf_get_func_from_name(bf, "func1");
	bf_func2 = bf_get_func_from_name(bf, "func2");

	if(bf_func1 == NULL || bf_func2 == NULL) {
		perror("Unable to locate func1 or func2 through disassembly.");
		xexit(-1);
	}

	bf_detour_func(bf, bf_func1, bf_func2);
	close_binary_file(bf);
}

void perform_diff(char * file1, char * file2)
{
	char * cmd = "diff ";
	char diff[strlen(cmd) + strlen(file1) + strlen(file2) + 2];

	sprintf(diff, "%s%s %s", cmd, file1, file2);

	if(system(diff)) {
		perror("Diff failed");
		xexit(-1);
	}
}

/*
 * Creates an expected output file. This is used for comparison against the
 * actual output from running the patched file.
 */
void create_expected_output_file(char * output_path)
{
	FILE * stream = fopen(output_path, "w+");
	fprintf(stream, "func2 was invoked\n");
	fclose(stream);
}

/*
 * Runs the patched program and dumps the output.
 */
void dump_output(char * target, char * dump)
{
	char * cmd = " > ";
	char   run_and_dump[strlen(target) + strlen(cmd) + strlen(dump) + 1];

	strcpy(run_and_dump, target);
	strcat(run_and_dump, cmd);
	strcat(run_and_dump, dump);

	if(system(run_and_dump)) {
		perror("Failed running target");
		xexit(-1);
	}
}

/*
 * Runs the patched program and compares the output to an expected output.
 */
void test_output(char * bitiness)
{
	char output_path[PATH_MAX] = {0};

	/* Get path of the output program */
	if(!get_output_path(output_path, ARRAY_SIZE(output_path), bitiness)) {
		perror("Unable to get path of output.");
		xexit(-1);
	} else {
		char expected_path[PATH_MAX] = {0};
		char output_dump[PATH_MAX]   = {0};

		if(!get_output_folder(expected_path,
				ARRAY_SIZE(expected_path), bitiness)) {
			perror("Unable to get output folder.");
			xexit(-1);
		} else {
			strcpy(output_dump, expected_path);
			strcat(expected_path, "/expected.output");
			create_expected_output_file(expected_path);
		}

		strcat(output_dump, "/actual.output");
		dump_output(output_path, output_dump);

		perform_diff(output_dump, expected_path);
	}
}

int main(int argc, char *argv[])
{
	if(argc != 2 || (strcmp(argv[1], "32") != 0 &&
			strcmp(argv[1], "64") != 0)) {
		perror("detour_test should be invoked with parameter "\
				"32 or 64 depending on which version of "\
				"the target should be tested against.");
	}	

	patch_func1_func2(argv[1]);
	test_output(argv[1]);
	return EXIT_SUCCESS;
}
