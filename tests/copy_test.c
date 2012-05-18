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

void perform_diff(char * file1, char * file2)
{
	char * cmd = "diff ";
	char diff[strlen(cmd) + strlen(file1) + strlen(file2) + 2];

	sprintf(diff, "%s%s %s", cmd, file1, file2);

	if(system(diff)) {
		printf("Diff failed between %s and %s\n", file1, file2);
		xexit(-1);
	}
}


/*
 * Run a test on an individual target. This loads a binary and saves it again
 * in output. Theoretically the two binaries should be the same, so that is
 * what we test for.
 */
void run_test(char * target, char * output)
{
	struct bin_file * bf  = load_binary_file(target, output);

	if(bf == NULL) {
		printf("No BFD backend found for %s.\n", target);
		return;
	}

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

	d = opendir(target_folder);
	if(d) {
		while((dir = readdir(d)) != NULL) {
			if(!(strcmp(dir->d_name, ".") == 0) &&
					!(strcmp(dir->d_name, "..") == 0)) {
				char * output_relative =
						"/tests-copy-output";
				char * target = xmalloc(strlen(target_folder) +
						strlen(dir->d_name) + 2);
				char * output = xmalloc(strlen(root) +
						strlen(output_relative) +
						strlen(bitiness) +
						strlen(dir->d_name) + 2);

				strcpy(target, target_folder);
				strcat(target, "/");
				strcat(target, dir->d_name);

				strcpy(output, root);
				strcat(output, output_relative);
				strcat(output, bitiness);
				strcat(output, "/");

				strcat(output, dir->d_name);

				printf("Copying %s to %s\n", target, output);

				run_test(target, output);

				perform_diff(target, output);

				free(target);
				free(output);
			}
		}

		closedir(d);
	}
}

int main(int argc, char *argv[])
{
	char target_folder[FILENAME_MAX]       = {0};
	char root[FILENAME_MAX]		       = {0};

	if(argc != 2 || (strcmp(argv[1], "32") != 0 &&
			strcmp(argv[1], "64") != 0)) {
		perror("copy_test should be invoked with parameter "\
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
		char * cmd2 = " && rm -rf tests-copy-output";
		char * cmd3 = " && mkdir tests-copy-output";

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
