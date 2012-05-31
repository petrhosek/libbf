#ifndef LOGGER_H
#define LOGGER_H

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <libiberty.h>
#include <string.h>
#include "func_analysis.h"

void get_output_folder(char * folder, size_t size, char * bitiness);
void get_output_location(char * loc, size_t size, char * bin, char * bitiness);
void get_global_mod_location(char * loc, size_t size, char * bitiness);
void dump_version(FILE * file, char * coreutils_bin);
void dump_individual(char * bin1, char * bin2, char * bitiness,
		struct change_info * ci);
void dump_global_mod_new_dir(char * bin, char * bitiness);
void dump_global_mod(char * bin, char * bitiness, struct change_info * ci);
void dump_data(char * bin1, char * bin2, char * bitiness,
		struct change_info * ci);
void dump_dir_data(char * dir1, char * dir2, char * bitiness,
		struct change_info * ci);
void create_global_mod_file(char * bitiness, char ** coreutils_bins,
		int num_bins, char * first_coreutils_dir);
void gen_gnuplot_global_mod_script(char * bitiness, char ** coreutils_bins,
		int num_bins);
void gen_gnuplot_all_script(char * bitiness);

#endif
