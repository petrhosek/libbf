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
 * Perform both a disassembly from 'main' symbol and entry point.
 */
void multi_root_disasm(struct binary_file * bf)
{
	/*
	 * Disassemble main.
	 */
	bf_for_each_symbol(bf, process_symbol, NULL);

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

void perform_timed_disassembly(struct binary_file * bf, long * ms)
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
void run_test(char * target, char * output, long * ms)
{
	struct binary_file * bf  = load_binary_file(target, NULL);

	if(bf == NULL) {
		printf("No BFD backend found for %s.\n", target);
		return;
	}

	printf("Disassembling %s\n", target);

	perform_timed_disassembly(bf, ms);

	create_entire_cfg_dot(bf, output);
	close_binary_file(bf);
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
				char * output_relative =
						"/tests-coreutils-output";
				char * extension       = ".dot";
				char * target = xmalloc(strlen(target_folder) +
						strlen(dir->d_name) + 2);
				char * output = xmalloc(strlen(root) +
						strlen(output_relative) +
						strlen(bitiness) +
						strlen(dir->d_name) +
						strlen(extension) + 2);

				strcpy(target, target_folder);
				strcat(target, "/");
				strcat(target, dir->d_name);

				strcpy(output, root);
				strcat(output, output_relative);
				strcat(output, bitiness);
				strcat(output, "/");

				strcat(output, dir->d_name);
				strcat(output, extension);

				run_test(target, output, &ms);

				free(target);
				free(output);
			}
		}

		closedir(d);

		printf("Total time to disassemble all targets: %ldms\n", ms);
	}
}

int main(int argc, char *argv[])
{
	char target_folder[FILENAME_MAX]       = {0};
	char root[FILENAME_MAX]		       = {0};

	if(argc != 2 || (strcmp(argv[1], "32") != 0 &&
			strcmp(argv[1], "64") != 0)) {
		perror("coreutils_test should be invoked with parameter "\
				"32 or 64 depending on which version of "\
				"coreutils should be tested against.");
	}
	
	if(!get_target_folder(target_folder, ARRAY_SIZE(target_folder), argv[1])) {
		perror("Failed to get path of folder. Make sure "\
				"./testprepare.sh has been run.");
		xexit(-1);
	}

	if(!get_root_folder(root, ARRAY_SIZE(root))) {
		perror("Failed to get root");
		xexit(-1);
	} else {
		char * cmd1 = "cd ";
		char * cmd2 = " && rm -rf tests-coreutils-output";
		char * cmd3 = " && mkdir tests-coreutils-output";

		char create_fresh_folder[strlen(cmd1) + strlen(root) +
				strlen(argv[1]) + strlen(cmd2) +
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
