#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <libiberty.h>

#include <libbind/binary_file.h>
#include <libbind/basic_blk.h>
#include <libbind/insn.h>
#include <libbind/symbol.h>
#include <libkern/htable.h>
#include "logger.h"

/*
 * The generated .dat files can be plotted with gnuplot with the following:
 * 	plot "X.dat" using 1:2 title 'Removed' with lines,"X.dat" using 1:3
 * 	title 'Added' with lines,"X.dat" using 1:4 title 'Modified' with lines
 *
 * An example global_mod.gp and script.gp file with this script is generated 
 * for all.dat. It can be used by running 'gnuplot' and using "load 'X.gp'".
 */

/*
 * Returns whether a symbol corresponds with a static function.
 */
bool is_func(struct symbol * sym)
{
	if(sym == NULL) {
		return FALSE;
	}

	return (sym->type & SYMBOL_FUNCTION) && (sym->address != 0);
}

/*
 * Generates change statistics between two binaries.
 */
void compare_bins(char * bitiness, char * bin1, char * bin2,
		struct change_info * ci_dir)
{
	struct bin_file * bf  = load_bin_file(bin1, NULL);
	struct bin_file * bf2 = load_bin_file(bin2, NULL);

	struct symbol *	      sym, * sym2;
	struct bf_basic_blk * bb1, * bb2;
	struct bb_cmp_info    info;
	struct change_info    ci = {0};

	printf("Comparing \n%s and \n%s:\n", bin1, bin2);

	htable_init(&info.visited_bbs);

	for_each_symbol(sym, &bf->sym_table) {
		if(is_func(sym)) {
			sym2 = symbol_find(&bf2->sym_table, sym->name);

			// Function name exists in bin1 but not bin2
			if(!is_func(sym2)) {
				ci.removed++;
			// Function name exists in both binaries
			} else {
				// Generate CFGs using the symbols as roots
				bb1 = disasm_bin_file_sym(bf, sym, TRUE);
				bb2 = disasm_bin_file_sym(bf2, sym2, TRUE);
				
				// Functions identical
				if(bb_cmp(&info, bb1, bb2)) {
					ci.same++;
				// Functions different
 				} else {
					ci.modified++;
				}
			}
		}
	}

	release_visited_info(&info);
	htable_destroy(&info.visited_bbs);

	// Function name exists in bin2 but not bin1 (introduced in bin2)
	for_each_symbol(sym, &bf2->sym_table) {
		if(is_func(sym)) {
			if(!is_func(symbol_find(&bf->sym_table, sym->name))) {
				ci.added++;
			}
		}
	}

	printf("%d functions removed\n", ci.removed);
	printf("%d functions added\n", ci.added);
	printf("%d functions modified\n", ci.modified);
	printf("%d functions same\n\n\n", ci.same);

	dump_data(bin1, bin2, bitiness, &ci);

	// Update the change_info for this directory.
	ci_dir->removed	 += ci.removed;
	ci_dir->added	 += ci.added;
	ci_dir->modified += ci.modified;
	ci_dir->same	 += ci.same;

	close_bin_file(bf);
	close_bin_file(bf2);
}

/*
 * Assumes filenames are identical across coreutils versions.
 */
void compare_dirs(char * bitiness, char * dir1, char * dir2,
		char ** coreutils_bins, int num_bins)
{
	struct change_info ci = {0};

	dump_global_mod_new_dir(dir2, bitiness);

	for(int i = 0; i < num_bins; i++) {
		char bin1[strlen(dir1) + strlen(coreutils_bins[i]) + 2];
		char bin2[strlen(dir1) + strlen(coreutils_bins[i]) + 2];

		strcpy(bin1, dir1);
		strcat(bin1, "/");
		strcat(bin1, coreutils_bins[i]);

		strcpy(bin2, dir2);
		strcat(bin2, "/");
		strcat(bin2, coreutils_bins[i]);

		compare_bins(bitiness, bin1, bin2, &ci);
	}

	dump_dir_data(dir1, dir2, bitiness, &ci);
}

int compare_bin(const void * elem1, const void * elem2)
{
	return strcmp(*(char **)elem1, *(char **)elem2);
}

unsigned int get_coreutils_bins(char ** coreutils_bins, size_t num,
		char * coreutils_dir)
{
	DIR * d = opendir(coreutils_dir);
	int   i = 0;

	printf("%s\n", coreutils_dir);

	if(d) {
		struct dirent * dir;

		while((dir = readdir(d)) != NULL) {
			if((strcmp(dir->d_name, ".") != 0) &&
					(strcmp(dir->d_name, "..") != 0)) {
				if(i < num) {
					coreutils_bins[i] =
							xstrdup(dir->d_name);
				} else {
					break;
				}

				i++;
			}
		}
	}

	qsort(coreutils_bins, i, sizeof(char *), compare_bin);
	return i;
}

void close_coreutils_bins(char ** coreutils_bins, size_t num)
{
	for(int i = 0; i < num; i++) {
		free(coreutils_bins[i]);
	}
}

int compare_dir(const void * elem1, const void * elem2)
{
	return atof(strrchr(*(char **)elem1, '.') + 1) -
			atof(strrchr(*(char **)elem2, '.') + 1);
}

/*
 * Returns a sorted list of directories containing coreutils build outputs. The
 * sort is performed by coreutils version.
 */
unsigned int get_coreutils_dirs(char ** coreutils_dirs, size_t num,
		char * bitiness)
{
	char  build_dir[PATH_MAX];
	DIR * d;
	int   i = 0;

	getcwd(build_dir, ARRAY_SIZE(build_dir));

	if(strcmp(bitiness, "32") == 0) {
		strcat(build_dir, "/build32");
	} else {
		strcat(build_dir, "/build64");
	}

	d = opendir(build_dir);
	if(d) {
		char		bin[] = "/bin";
		struct dirent * dir;

		while((dir = readdir(d)) != NULL) {
			if((strcmp(dir->d_name, ".") != 0) &&
					(strcmp(dir->d_name, "..") != 0)) {
				coreutils_dirs[i] = xmalloc(strlen(build_dir) +
						strlen(dir->d_name) +
						sizeof(bin) + 2);
				strcpy(coreutils_dirs[i], build_dir);
				strcat(coreutils_dirs[i], "/");
				strcat(coreutils_dirs[i], dir->d_name);
				strcat(coreutils_dirs[i], bin);

				i++;
			}
		}

		qsort(coreutils_dirs, i, sizeof(char *), compare_dir);
	}

	return i;
}

void close_coreutils_dirs(char ** coreutils_dirs, size_t num)
{
	for(int i = 0; i < num; i++) {
		free(coreutils_dirs[i]);
	}
}

/*
 * Assumes a maximum of 20 coreutils versions and a maximum of 128 coreutils
 * binaries.
 */
int find_coreutils_changes(char * bitiness)
{
	char *	     coreutils_dirs[20] = {0};
	unsigned int num_dirs           = get_coreutils_dirs(coreutils_dirs,
			ARRAY_SIZE(coreutils_dirs), bitiness);

	if(num_dirs == 0) {
		perror("Ensure build-coreutils.sh has been run.");
		return -1;
	} else {
		char *	     coreutils_bins[128] = {0};
		unsigned int num_bins		 = get_coreutils_bins(
				coreutils_bins, ARRAY_SIZE(coreutils_bins),
				coreutils_dirs[0]);

		create_global_mod_file(bitiness, coreutils_bins, num_bins,
				coreutils_dirs[0]);

		for(int i = 0; i < num_dirs - 1; i++) {
			compare_dirs(bitiness, coreutils_dirs[i],
					coreutils_dirs[i+1], coreutils_bins,
					num_bins);
		}

		gen_gnuplot_global_mod_script(bitiness, coreutils_bins, num_bins);
		close_coreutils_bins(coreutils_bins, num_bins);
	}

	close_coreutils_dirs(coreutils_dirs, num_dirs);
	gen_gnuplot_all_script(bitiness);
	return 0;
}

int main(int argc, char * argv[])
{
	if(argc != 2 || (strcmp(argv[1], "32") != 0 &&
			strcmp(argv[1], "64") != 0)) {
		perror("coreutils-change should be invoked with parameter "\
				"32 or 64 depending on which versions of "\
				"coreutils should be tested against.");
		xexit(-1);
	} else {
		char cmd1[] = "rm -rf output";
		char cmd2[] = " && mkdir output";

		char create_fresh_folder[ARRAY_SIZE(cmd1) - 1 +
				strlen(argv[1])* 2 + ARRAY_SIZE(cmd2)];

		strcpy(create_fresh_folder, cmd1);
		strcat(create_fresh_folder, argv[1]);
		strcat(create_fresh_folder, cmd2);
		strcat(create_fresh_folder, argv[1]);

		if(system(create_fresh_folder)) {
			perror("Failed creating fresh folder");
			xexit(-1);
		}
	}

	return find_coreutils_changes(argv[1]);
}
