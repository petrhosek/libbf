#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>

#include <bf_insn.h>
#include <bf_basic_blk.h>
#include <bf_func.h>
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
bool get_target_folder(char * path, size_t size, char * bitiness)
{
	if(!get_root_folder(path, size)) {
		return FALSE;
	} else {
		int target_desc;

		if(strcmp(bitiness, "32") == 0) {
			strncat(path, "/coreutils32/bin", size -
					strlen(path) - 1);
		} else {
			strncat(path, "/coreutils64/bin", size -
					strlen(path) - 1);
		}

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
 * Generates the output.
 */
void dump_bf(struct bin_file * bf, char * output)
{
	FILE * stream = fopen(output, "w+");
	print_all_bf_insn(bf, stream);
	fclose(stream);
}

/*
 * Generates the output from the semantic information stored in each bf_insn.
 */
void dump_bf_semantic_gen(struct bin_file * bf, char * output)
{
	FILE * stream = fopen(output, "w+");
	print_all_bf_insn_semantic_gen(bf, stream);
	fclose(stream);
}

/*
 * Using the visitor pattern to locate main and start disassembling from there.
 */
void process_symbol(struct bin_file * bf, asymbol * sym, void * param)
{
	if(strcmp(sym->name, "main") == 0) {
		disassemble_binary_file_symbol(bf, sym, TRUE);
	}
}

/*
 * Perform both a disassembly from 'main' symbol and entry point.
 */
void multi_root_disasm(struct bin_file * bf)
{
	/*
	 * Disassemble main.
	 */
	bf_enum_symbol(bf, process_symbol, NULL);

	/*
	 * Also disassemble entry point. This should result in multiple roots.
	 */
	disassemble_binary_file_entry(bf);
}

/*
 * Get millisecond difference between two timevals.
 */
long timevaldiff(struct timeval * start, struct timeval * finish)
{
	long ms;
	ms  = (finish->tv_sec - start->tv_sec) * 1000;
	ms += (finish->tv_usec - start->tv_usec) / 1000;
	return ms;
}

void perform_timed_disassembly(struct bin_file * bf, long * ms)
{
	struct timeval start;
	struct timeval end;

	gettimeofday(&start, NULL);
	multi_root_disasm(bf);
	gettimeofday(&end, NULL);

	printf("Disassembly took: %ldms\n", timevaldiff(&start, &end));
	*ms += timevaldiff(&start, &end);
}

/*
 * Run a test on an individual target. This attempts the generation of a CFG.
 * Error checking omitted for brevity.
 */
void run_test(char * target, char * output, char * output2, long * ms)
{
	struct bin_file * bf = load_binary_file(target, NULL);

	if(bf == NULL) {
		printf("No BFD backend found for %s.\n", target);
		return;
	}

	printf("Disassembling %s\n", target);

	perform_timed_disassembly(bf, ms);

	dump_bf(bf, output);
	dump_bf_semantic_gen(bf, output2);
	close_binary_file(bf);
}

void strip_output_spaces(char * file)
{
	char * cmd = "sed -i \"s/ *//g\" ";
	char   strip[strlen(cmd) + strlen(file) + 1];

	sprintf(strip, "%s%s", cmd, file);

	if(system(strip)) {
		printf("Failed stripping %s\n", file);
	}
}

void delete_data_insns(char * file)
{
	char * cmd  = "grep -Ev 'data32' ";
	char * cmd2 = "> tmp.txt && mv tmp.txt ";
	char delete_data[strlen(cmd) + strlen(file) * 2 + strlen(cmd2) + 1];

	sprintf(delete_data, "%s%s%s%s", cmd, file, cmd2, file);

	if(system(delete_data)) {
		printf("Failed deleting data insns %s\n", file);
	}
}

void normalize_output(char * file)
{
	strip_output_spaces(file);
	delete_data_insns(file);
}

void perform_diff(char * file1, char * file2)
{
	char * cmd = "diff ";
	char diff[strlen(cmd) + strlen(file1) + strlen(file2) + 2];

	sprintf(diff, "%s%s %s", cmd, file1, file2);

	if(system(diff)) {
		printf("Diff failed\n");
		xexit(-1);
	}
}

/*
 * Get all the test files and run tests against them.
 */
void enumerate_files_and_run_tests(char * root, char * target_folder,
		char * bitiness)
{
	DIR *		d;
	struct dirent * dir;
	long		ms = 0;

	d = opendir(target_folder);
	if(d) {
		while((dir = readdir(d)) != NULL) {
			if(!(strcmp(dir->d_name, ".") == 0) &&
					!(strcmp(dir->d_name, "..") == 0)) {
				char * output_relative = "/tests-disasm-output";
				char * extension       = ".txt";
				char * extension2      = "-semantic-gen.txt";
				char * target  = xmalloc(strlen(target_folder) +
						strlen(dir->d_name) + 2);
				char * output  = xmalloc(strlen(root) +
						strlen(output_relative) +
						strlen(bitiness) +
						strlen(dir->d_name) +
						strlen(extension) + 2);
				char * output2 = xmalloc(strlen(root) +
						strlen(output_relative) +
						strlen(bitiness) +
						strlen(dir->d_name) +
						strlen(extension2) + 2);

				strcpy(target, target_folder);
				strcat(target, "/");
				strcat(target, dir->d_name);

				strcpy(output, root);
				strcat(output, output_relative);
				strcat(output, bitiness);
				strcat(output, "/");
				strcat(output, dir->d_name);

				strcpy(output2, output);

				strcat(output, extension);
				strcat(output2, extension2);

				run_test(target, output, output2, &ms);
				normalize_output(output);

				perform_diff(output, output2);

				free(target);
				free(output);
				free(output2);
			}
		}

		closedir(d);

		printf("Total time to disassemble all targets: %ldms\n", ms);
	}
}

int main(int argc, char *argv[])
{
	char target_folder[FILENAME_MAX] = {0};
	char root[FILENAME_MAX]		 = {0};
	
	if(argc != 2 || (strcmp(argv[1], "32") != 0 &&
			strcmp(argv[1], "64") != 0)) {
		perror("disasm_engine_test should be invoked with parameter "\
				"32 or 64 depending on which version of "\
				"coreutils should be tested against.");
	}

	if(!get_target_folder(target_folder,
			ARRAY_SIZE(target_folder), argv[1])) {
		perror("Failed to get path of folder. Make sure "\
				"./testprepare.sh has been run.");
		xexit(-1);
	}

	if(!get_root_folder(root, ARRAY_SIZE(root))) {
		perror("Failed to get root");
		xexit(-1);
	} else {
		char * cmd1 = "cd ";
		char * cmd2 = " && rm -rf tests-disasm-output";
		char * cmd3 = " && mkdir tests-disasm-output";

		char create_fresh_folder[strlen(cmd1) + strlen(root) +
				strlen(cmd2) + strlen(argv[1]) +
				strlen(cmd3) + strlen(argv[1]) + 1];

		strcpy(create_fresh_folder, cmd1);
		strcat(create_fresh_folder, root);
		strcat(create_fresh_folder, cmd2);
		strcat(create_fresh_folder, argv[1]);
		strcat(create_fresh_folder, cmd3);
		strcat(create_fresh_folder, argv[1]);

		if(system(create_fresh_folder)) {
			perror("Failed creating fresh folder");
			xexit(-1);
		}
	}

	enumerate_files_and_run_tests(root, target_folder, argv[1]);
	return EXIT_SUCCESS;
}
