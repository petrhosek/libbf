#include "logger.h"

static void extract_version(char * buf, size_t size, char * bin)
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

extern void get_output_folder(char * folder, size_t size, char * bitiness)
{
	getcwd(folder, size);
	strcat(folder, "/output");
	strcat(folder, bitiness);
	strcat(folder, "/");
}

extern void get_output_location(char * loc, size_t size, char * bin,
		char * bitiness)
{
	get_output_folder(loc, size, bitiness);
	strcat(loc, bin);
	strcat(loc, ".dat");
}

extern void get_global_mod_location(char * loc, size_t size, char * bitiness)
{
	get_output_folder(loc, size, bitiness);
	strcat(loc, "global_mod.dat");
}

extern void dump_version(FILE * file, char * coreutils_bin)
{
	char ver[8];
	extract_version(ver, ARRAY_SIZE(ver), coreutils_bin);
	fprintf(file, "%s", ver);
}

extern void dump_individual(char * bin1, char * bin2, char * bitiness,
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

extern void dump_global_mod_new_dir(char * bin, char * bitiness)
{
	FILE * file;
	char   loc[PATH_MAX];
	get_global_mod_location(loc, ARRAY_SIZE(loc), bitiness);

	file = fopen(loc, "r+");
	fseek(file, 0, SEEK_END);
	fputs("\n", file);
	dump_version(file, bin);
	fclose(file);
}

extern void dump_global_mod(char * bin, char * bitiness,
		struct change_info * ci)
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
extern void dump_data(char * bin1, char * bin2, char * bitiness,
		struct change_info * ci)
{
	dump_individual(bin1, bin2, bitiness, ci);
	dump_global_mod(bin2, bitiness, ci);
}

extern void dump_dir_data(char * dir1, char * dir2, char * bitiness,
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

extern void create_global_mod_file(char * bitiness, char ** coreutils_bins,
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

	fclose(file);
}

extern void gen_gnuplot_global_mod_script(char * bitiness, char ** coreutils_bins,
		int num_bins)
{
	FILE * file;
	char   output[PATH_MAX];

	get_output_folder(output, ARRAY_SIZE(output), bitiness);
	strcat(output, "global_mod.gp");

	file = fopen(output, "w");
	
	fputs("set title \"Global Modifications\"\n", file);
	fputs("set key invert reverse Left outside\n", file);
	fputs("set key autotitle columnheader\n", file);
	fputs("set style data histogram\n", file);
	fputs("set style histogram rowstacked\n", file);
	fputs("set style fill solid border -1\n", file);
	fputs("set boxwidth 0.75\n", file);
	fprintf(file, "plot 'global_mod.dat' using 2:xtic(1) title '%s'",
			coreutils_bins[0]);

	for(int i = 1; i < num_bins; i++) {
		fprintf(file, ",'' using %d title '%s'", i + 2,
				coreutils_bins[i]);
	}

	fclose(file);
}

extern void gen_gnuplot_all_script(char * bitiness)
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
