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

/*
 * The generated .dat files can be plotted with gnuplot with the following:
 * 	plot "X.dat" using 1:2 title 'Removed' with lines,"X.dat" using 1:3
 * 	title 'Added' with lines,"X.dat" using 1:4 title 'Modified' with lines
 *
 * An example all.gp file with this script is generated for all.dat. It can be
 * used by running 'gnuplot' and using 'load all.gp'.
 */

struct bb_cmp_info {
	struct htable visited_bbs;
};

struct visited_bb {
	struct bf_basic_blk * bb;
	struct bf_basic_blk * bb2;
	struct htable_entry   entry;
};

struct change_info {
	int removed;
	int added;
	int modified;
	int same;
};

void get_output_folder(char * folder, size_t size, char * bitiness)
{
	getcwd(folder, size);
	strcat(folder, "/output");
	strcat(folder, bitiness);
	strcat(folder, "/");
}

void get_output_location(char * loc, size_t size, char * bin, char * bitiness)
{
	get_output_folder(loc, size, bitiness);
	strcat(loc, bin);
	strcat(loc, ".dat");
}

void get_global_mod_location(char * loc, size_t size, char * bitiness)
{
	get_output_folder(loc, size, bitiness);
	strcat(loc, "global_mod.dat");
}

void extract_version(char * buf, size_t size, char * bin)
{
	char substr[] = "coreutils-";
	memcpy(buf, strstr(bin, substr) + ARRAY_SIZE(substr) - 1, size);

	for(int i = 0; i < size; i++) {
		if(buf[i] == '/') {
			buf[i] = '\0';
			break;
		}
	}

	/*
	 * This normalises the version numbers so they order properly on the
	 * graph. For example, 8.1 becomes 8.01 so it will not be treated as
	 * 8.10.
	 */
	if(!isdigit(buf[3])) {
		buf[4] = buf[3];
		buf[3] = buf[2];
		buf[2] = '0';
	}
}

void dump_version(FILE * file, char * coreutils_bin)
{
	char ver[8];
	extract_version(ver, ARRAY_SIZE(ver), coreutils_bin);
	fprintf(file, "%s", ver);
}

void dump_individual(char * bin1, char * bin2, char * bitiness,
		struct change_info * ci)
{
	FILE * file;
	char * bin = strrchr(bin1, '/') + 1;
	char   loc[PATH_MAX];

	get_output_location(loc, ARRAY_SIZE(loc), bin,
			bitiness);

	if((file = fopen(loc, "r+")) == NULL) {
		file = fopen(loc, "w");
		fputs("# Gnuplot script file for \"", file);
		fputs(bin, file);
		fputs("\"\n# Version\tRemoved\tAdded\tModified\n", file);

		dump_version(file, bin1);
		fputs("\t0\t0\t0\n", file);
	} else {
		char		   old_v[8];
		struct change_info ci_old;

		fseek(file, -2, SEEK_END);
		while(fgetc(file) != '\n') {
			fseek(file, -2, SEEK_CUR);
		}

		fseek(file, 1, SEEK_CUR);
		fscanf(file, "%s\t%d\t%d\t%d\n", old_v, &ci_old.removed,
				&ci_old.added,	&ci_old.modified);
		fseek(file, 0, SEEK_END);

		dump_version(file, bin2);
		fprintf(file, "\t%d\t%d\t%d\n", ci_old.removed + ci->removed,
				ci_old.added + ci->added,
				ci_old.modified + ci->modified);
	}

	fclose(file);
}

void dump_global_mod_new_dir(char * bin, char * bitiness)
{
	FILE * file;
	char   loc[PATH_MAX];
	get_global_mod_location(loc, ARRAY_SIZE(loc), bitiness);

	file = fopen(loc, "r+");
	dump_version(file, bin);
	fclose(file);
}

void dump_global_mod_next_dir(char * bitiness)
{
	FILE * file;
	char   loc[PATH_MAX];
	get_global_mod_location(loc, ARRAY_SIZE(loc), bitiness);

	file = fopen(loc, "r+");
	fputs("\n", file);
	fclose(file);
}

void dump_global_mod(char * bin, char * bitiness, struct change_info * ci)
{
	FILE * file;
	char   loc[PATH_MAX];
	char   ver[8];

	get_global_mod_location(loc, ARRAY_SIZE(loc), bitiness);
	extract_version(ver, ARRAY_SIZE(ver), bin);

	file = fopen(loc, "r+");
	fseek(file, 0, SEEK_END);
	fprintf(file, "\t%d", ci->modified);
	fclose(file);
}

/*
 * Assumes the maximum version length is 7 characters.
 */
void dump_data(char * bin1, char * bin2, char * bitiness,
		struct change_info * ci)
{
	dump_individual(bin1, bin2, bitiness, ci);
	dump_global_mod(bin2, bitiness, ci);
}

void add_visited_bb(struct bb_cmp_info * info, struct bf_basic_blk * bb,
		struct bf_basic_blk * bb2)
{
	struct visited_bb * v_bb = malloc(sizeof(struct visited_bb));
	v_bb->bb		 = bb;
	v_bb->bb2		 = bb2;

	htable_add(&info->visited_bbs, &v_bb->entry, &bb->vma,
			sizeof(bb->vma));
}

void release_visited_info(struct bb_cmp_info * info)
{
	struct htable_entry * cur_entry;
	struct htable_entry * n;
	struct visited_bb *   v_bb;

	htable_for_each_entry_safe(v_bb, cur_entry, n, &info->visited_bbs,
			entry) {
		htable_del_entry(&info->visited_bbs, cur_entry);
		free(v_bb);
	}
}

bool has_visited_bb(struct bb_cmp_info * info, struct bf_basic_blk * bb,
		struct bf_basic_blk * bb2)
{
	struct visited_bb * v_bb = hash_find_entry(&info->visited_bbs,
			&bb->vma, sizeof(bb->vma), struct visited_bb, entry);
	return ((v_bb != NULL) && (v_bb->bb2->vma == bb2->vma));
}

/*
 * A quick note here. At the moment both bf_get_bb_insn and bf_get_bb_length
 * are O(n). This can (and probably eventually _should_) be changed to O(K).
 */
bool bb_cmp(struct bb_cmp_info * info, struct bf_basic_blk * bb,
		struct bf_basic_blk * bb2)
{
	/*
	 * Both NULL.
	 */
	if(bb == NULL && bb2 == NULL) {
		return TRUE;
	/*
	 * Branch in one but not the other.
	 */
	} else if(bb == NULL || bb2 == NULL) {
		return FALSE;
	/*
	 * Already visited.
	 */
	} else if(has_visited_bb(info, bb, bb2)) {
		return TRUE;
	} else {
		unsigned int length = bf_get_bb_length(bb);

		/*
		 * Different num instructions.
		 */
		if(bf_get_bb_length(bb2) != length) {
			return FALSE;
		}

		/*
		 * Check each instruction mnemonic.
		 */
		for(int i = 0; i < length; i++) {
			if(bf_get_bb_insn(bb, i)->mnemonic !=
					bf_get_bb_insn(bb2, i)->mnemonic) {
				return FALSE;
			}
		}

		/*
		 * Update visited bbs and compare the next bbs in the CFG.
		 */
		add_visited_bb(info, bb, bb2);
		return bb_cmp(info, bb->target, bb2->target) &&
				bb_cmp(info, bb->target2, bb2->target2);
	}
}

bool is_func(struct symbol * sym)
{
	if(sym == NULL) {
		return FALSE;
	}

	return (sym->type & SYMBOL_FUNCTION) && (sym->address != 0);
}

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

			if(!is_func(sym2)) {
				ci.removed++;
			} else {
				bb1 = disasm_bin_file_sym(bf, sym, TRUE);
				bb2 = disasm_bin_file_sym(bf2, sym2, TRUE);
				
				if(!bb_cmp(&info, bb1, bb2)) {
					ci.same++;
 				} else {
					ci.modified++;
				}
			}
		}
	}

	release_visited_info(&info);
	htable_destroy(&info.visited_bbs);

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

void dump_dir_data(char * dir1, char * dir2, char * bitiness,
		struct change_info * ci)
{
	char bin1_all[PATH_MAX];
	char bin2_all[PATH_MAX];

	strcpy(bin1_all, dir1);
	strcat(bin1_all, "/all");

	strcpy(bin2_all, dir2);
	strcat(bin2_all, "/all");

	dump_data(bin1_all, bin2_all, bitiness, ci);
}

void create_global_mod_file(char * bitiness, char ** coreutils_bins,
		int num_bins, char * first_coreutils_dir)
{
	FILE * file;
	char   loc[PATH_MAX];

	get_global_mod_location(loc, ARRAY_SIZE(loc), bitiness);

	file = fopen(loc, "w");
	fputs("# Gnuplot script file for global modifications\n# Version",
			file);

	for(int i = 0; i < num_bins; i++) {
		fprintf(file, "\t%s", coreutils_bins[i]);
	}

	fputs("\n", file);

	dump_version(file, first_coreutils_dir);

	for(int i = 0; i < num_bins; i++) {
		fputs("\t0", file);
	}
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

	dump_global_mod_next_dir(bitiness);
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

void gen_gnuplot_script(char * bitiness)
{
	FILE * file;
	char   buf[] = "plot \"all.dat\" using 1:2 title 'Removed' with lines,"\
			"\"all.dat\" using 1:3 title 'Added' with lines,"\
			"\"all.dat\" using 1:4 title 'Modified' with lines";
	char   output[PATH_MAX];

	get_output_folder(output, ARRAY_SIZE(output), bitiness);
	strcat(output, "script.gp");

	file = fopen(output, "w");
	fputs(buf, file);
	fclose(file);
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

		close_coreutils_bins(coreutils_bins, num_bins);
	}

	close_coreutils_dirs(coreutils_dirs, num_dirs);
	gen_gnuplot_script(bitiness);
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
