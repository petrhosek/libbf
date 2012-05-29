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
 */

struct bb_cmp_info {
	struct htable visited_bbs;
};

struct visited_bb {
	struct bf_basic_blk * bb;
	struct htable_entry   entry;
};

struct change_info {
	int removed;
	int added;
	int modified;
	int same;
};

void get_output_location(char * loc, size_t size, char * bin, char * bitiness)
{
	getcwd(loc, size);
	strcat(loc, "/output");
	strcat(loc, bitiness);
	strcat(loc, "/");
	strcat(loc, bin);
	strcat(loc, ".dat");
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

/*
 * Assumes the maximum version length is 7 characters.
 */
void dump_data(char * bin1, char * bin2, char * bitiness,
		struct change_info * ci)
{
	FILE * file;
	char * bin = strrchr(bin1, '/') + 1;
	char   loc[PATH_MAX];
	char   v1[8];
	char   v2[8];

	get_output_location(loc, ARRAY_SIZE(loc), bin,
			bitiness);
	extract_version(v1, ARRAY_SIZE(v1), bin1);
	extract_version(v2, ARRAY_SIZE(v2), bin2);

	if((file = fopen(loc, "r+")) == NULL) {
		char buf[]  = "# Gnuplot script file for \"";
		char buf2[] = "\"\n# Version\tRemoved\tAdded\tModified\n";
		char buf3[] = "\t0\t0\t0\n";

		file = fopen(loc, "w");
		fwrite(buf, 1, ARRAY_SIZE(buf) - 1, file);
		fwrite(bin, 1, strlen(bin), file);
		fwrite(buf2, 1, ARRAY_SIZE(buf2) - 1, file);
		fwrite(v1, 1, strlen(v1), file);
		fwrite(buf3, 1, ARRAY_SIZE(buf3) - 1, file);
	} else {
		struct change_info ci_old;

		fseek(file, -2, SEEK_END);
		while(fgetc(file) != '\n') {
			fseek(file, -2, SEEK_CUR);
		}

		fseek(file, 1, SEEK_CUR);
		fscanf(file, "%s\t%d\t%d\t%d\n", v1, &ci_old.removed,
				&ci_old.added,	&ci_old.modified);
		fseek(file, 0, SEEK_END);
		fprintf(file, "%s\t%d\t%d\t%d\n", v2,
				ci_old.removed + ci->removed,
				ci_old.added + ci->added,
				ci_old.modified + ci->modified);
	}

	fclose(file);
}

void add_visited_bb(struct bb_cmp_info * info, struct bf_basic_blk * bb)
{
	struct visited_bb * v_bb = malloc(sizeof(struct visited_bb));
	v_bb->bb		 = bb;

	htable_add(&info->visited_bbs, &v_bb->entry, &bb->vma,
			sizeof(bb->vma));
}

void release_visited_info(struct bb_cmp_info * info)
{
	struct htable_entry * cur_entry;
	struct htable_entry * n;
	struct visited_bb *   bb;

	htable_for_each_entry_safe(bb, cur_entry, n, &info->visited_bbs,
			entry) {
		htable_del_entry(&info->visited_bbs, cur_entry);
		free(bb);
	}
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
	} else if(htable_find(&info->visited_bbs, &bb->vma, sizeof(bb->vma))) {
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
		add_visited_bb(info, bb);
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

	for_each_symbol(sym, &bf->sym_table) {
		if(is_func(sym)) {
			sym2 = symbol_find(&bf2->sym_table, sym->name);

			if(!is_func(sym2)) {
				ci.removed++;
			} else {
				bb1 = disasm_bin_file_sym(bf, sym, TRUE);
				bb2 = disasm_bin_file_sym(bf2, sym2, TRUE);

				htable_init(&info.visited_bbs);
				
				if(!bb_cmp(&info, bb1, bb2)) {
					ci.same++;
 				} else {
					ci.modified++;
				}

				release_visited_info(&info);
				htable_destroy(&info.visited_bbs);
			}
		}
	}

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

/*
 * Assumes filenames are identical across coreutils versions.
 */
void compare_dirs(char * bitiness, char * dir1, char * dir2)
{
	struct change_info ci = {0};

	DIR * d = opendir(dir1);
	if(d) {
		struct dirent * dir;

		while((dir = readdir(d)) != NULL) {
			if((strcmp(dir->d_name, ".") != 0) &&
					(strcmp(dir->d_name, "..") != 0)) {
				char bin1[strlen(dir1) +
						strlen(dir->d_name) + 2];
				char bin2[strlen(dir2) +
						strlen(dir->d_name) + 2];

				strcpy(bin1, dir1);
				strcat(bin1, "/");
				strcat(bin1, dir->d_name);

				strcpy(bin2, dir2);
				strcat(bin2, "/");
				strcat(bin2, dir->d_name);

				compare_bins(bitiness, bin1, bin2, &ci);
			}
		}
	}

	dump_dir_data(dir1, dir2, bitiness, &ci);
}

int compare(const void * elem1, const void * elem2)
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
	char		build_dir[PATH_MAX];
	DIR *		d;

	getcwd(build_dir, ARRAY_SIZE(build_dir));

	if(strcmp(bitiness, "32") == 0) {
		strcat(build_dir, "/build32");
	} else {
		strcat(build_dir, "/build64");
	}

	d = opendir(build_dir);
	if(d) {
		char		bin[] = "/bin";
		int		i     = 0;
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

		qsort(coreutils_dirs, i, sizeof(char *), compare);
		return i;
	} else {
		return 0;
	}
}

void close_coreutils_dirs(char ** coreutils_dirs, size_t num)
{
	for(int i = 0; i < num; i++) {
		free(coreutils_dirs[i]);
	}
}


int find_coreutils_changes(char * bitiness)
{
	char *	     coreutils_dirs[20] = {0};
	unsigned int num_dirs           = get_coreutils_dirs(coreutils_dirs,
			ARRAY_SIZE(coreutils_dirs), bitiness);

	if(num_dirs == 0) {
		perror("Ensure build-coreutils.sh has been run.");
		return -1;
	}

	for(int i = 0; i < num_dirs - 1; i++) {
		compare_dirs(bitiness, coreutils_dirs[i], coreutils_dirs[i+1]);
	}

	close_coreutils_dirs(coreutils_dirs, num_dirs);
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
