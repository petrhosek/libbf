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

struct bb_cmp_info {
	struct htable visited_bbs;
};

struct visited_bb {
	struct bf_basic_blk * bb;
	struct htable_entry   entry;
};

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

void compare_bins(char * bin1, char * bin2)
{
	struct bin_file * bf  = load_bin_file(bin1, NULL);
	struct bin_file * bf2 = load_bin_file(bin2, NULL);

	struct symbol *	      sym, * sym2;
	struct bf_basic_blk * bb1, * bb2;
	struct bb_cmp_info    info;

	int removed = 0, added = 0, modified = 0, same = 0;

	printf("Comparing \n%s and \n%s:\n", bin1, bin2);

	for_each_symbol(sym, &bf->sym_table) {
		if(is_func(sym)) {
			sym2 = symbol_find(&bf2->sym_table, sym->name);

			if(!is_func(sym2)) {
				removed++;
			} else {
				bb1 = disasm_bin_file_sym(bf, sym, TRUE);
				bb2 = disasm_bin_file_sym(bf2, sym2, TRUE);

				htable_init(&info.visited_bbs);
				
				if(bb_cmp(&info, bb1, bb2)) {
					same++;
 				} else {
					modified++;
				}

				release_visited_info(&info);
				htable_destroy(&info.visited_bbs);
			}
		}
	}

	for_each_symbol(sym, &bf2->sym_table) {
		if(is_func(sym)) {
			if(!is_func(symbol_find(&bf->sym_table, sym->name))) {
				added++;
			}
		}
	}

	printf("%d functions removed\n", removed);
	printf("%d functions added\n", added);
	printf("%d functions modified\n", modified);
	printf("%d functions same\n\n\n", same);

	close_bin_file(bf);
	close_bin_file(bf2);
}

/*
 * Assumes filenames are identical across coreutils versions.
 */
void compare_dirs(char * dir1, char * dir2)
{
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

				compare_bins(bin1, bin2);
			}
		}
	}
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
		compare_dirs(coreutils_dirs[i], coreutils_dirs[i+1]);
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
	}

	return find_coreutils_changes(argv[1]);
}
