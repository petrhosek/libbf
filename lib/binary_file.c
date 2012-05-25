#include "binary_file.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/param.h>
#include <unistd.h>

#include "disasm.h"
#include "func.h"
#include "basic_blk.h"
#include "mem_manager.h"

static const char * resolve_file(const char * filename) {
	struct stat statbuf;
	char *	    pathname = malloc(MAXPATHLEN);

	if(strchr(filename, '/')) {
		if(strlen(filename) > sizeof(pathname) - 1) {
			errno = ENAMETOOLONG;
			goto error;
		}

		strcpy(pathname, filename);
	} else {
		int m, n, len;

		for(char * path = getenv("PATH"); path && *path; path += m) {
			if(strchr(path, ':')) {
				n = strchr(path, ':') - path;
				m = n + 1;
			} else {
				m = n = strlen(path);
			}

			if(n == 0) {
				if(!getcwd(pathname, MAXPATHLEN)) {
					goto error;
				}

				len = strlen(pathname);
			} else if (n > sizeof pathname - 1) {
				continue;
			} else {
				strncpy(pathname, path, n);
				len = n;
			}

			if(len && pathname[len - 1] != '/') {
				pathname[len++] = '/';
			}

			strcpy(pathname + len, filename);

			/*
			 * Accept only regular files with some read bits set.
			 */
			if(stat(pathname, &statbuf) == 0 &&
					S_ISREG(statbuf.st_mode) &&
					(statbuf.st_mode & 0444)) {
				break;
			}
		}
	}

	if(stat(pathname, &statbuf) == 0) {
		return pathname;
	}

error:
	free(pathname);
	return NULL;
}

/**
 * Returns the size of the named file.
 *
 * @param filename The file name.
 * @return The file size if file exists, -1 otherwise.
 */
static off_t get_file_size(const char *filename) {
	struct stat statbuf;

	if(stat(filename, &statbuf) < 0) {
		if(errno == ENOENT) {
			fprintf(stderr, "sea: no such file '%s'", filename);
		} else {
			fprintf(stderr, "Warning: could not locate '%s'",
					filename);
		}
	} else if(!S_ISREG(statbuf.st_mode)) {
		fprintf(stderr, "Warning: '%s' is not an ordinary file",
			filename);
	} else if (statbuf.st_size < 0) {
		fprintf(stderr, "Warning: '%s' has negative size, probably "\
				"it is too large", filename);
	} else {
		return statbuf.st_size;
	}

	return (off_t)-1;
}

/*
 * Initialising the opcodes disassembler. Instead of providing the real
 * fprintf, we redirect to our own version which writes to our bf object.
 * (Concept borrowed from opdis).
 */
static void init_bf_disassembler(struct bin_file * bf)
{
	init_disassemble_info(&bf->disasm_config, bf, binary_file_fprintf);

	bf->disasm_config.flavour = bfd_get_flavour(bf->abfd);
	bf->disasm_config.arch	  = bfd_get_arch(bf->abfd);
	bf->disasm_config.mach	  = bfd_get_mach(bf->abfd);
	bf->disasm_config.endian  = bf->abfd->xvec->byteorder;
	disassemble_init_for_target(&bf->disasm_config);

	bf->disassembler = disassembler(bf->abfd);
}

/*
 * Initialises the internal hashtables of binary_file.
 */
static void init_bf(struct bin_file * bf)
{
	htable_init(&bf->func_table);
	htable_init(&bf->bb_table);
	htable_init(&bf->insn_table);
	symbol_table_init(&bf->sym_table);
	htable_init(&bf->mem_table);

	bf->bitiness = bfd_arch_bits_per_address(bf->abfd) == 64 ?
			arch_64 : arch_32;

	if(elf_version(EV_CURRENT) == EV_NONE) {
		printf("Warning: ELF library out of date.");
	}
}

/*
 * Copies a file from source to dest.
 */
static bool copy(char * source, char * dest)
{
	/*
	 * TODO: This is VERY VERY BAD and allows an attacker to execute shell
	 * code. The input needs to be sanitised or a safer method used.
	 */
	char cp[] = "/bin/cp ";
	char command[sizeof(cp) + strlen(source) + strlen(dest) + 2];

	strcpy(command, cp);
	strcat(command, source);
	strcat(command, " ");
	strcat(command, dest);

	return system(command) == 0;
}

struct bin_file * load_bin_file(char * target_path, char * output_path)
{
	struct bin_file * bf   = xmalloc(sizeof(struct bin_file));
	bfd *		  abfd = NULL;

	memset(&bf->disasm_config, 0, sizeof(bf->disasm_config));
	bfd_init();

	bf->abfd = abfd = bfd_openr(target_path, NULL);

	if(abfd) {
		if(!bfd_check_format(abfd, bfd_object)) {
			bfd_close(abfd);
			free(bf);
			bf = NULL;

			printf("%s could not be matched to a BFD object\n",
					target_path);
		} else {
			if(output_path != NULL) {
				if(!copy(target_path, output_path)) {
					printf("Failed copying %s to %s\n",
							target_path,
							output_path);
				}
			}

			bf->output_path = xstrdup(output_path != NULL ?
					output_path : target_path);

			init_bf(bf);
			init_bf_disassembler(bf);
			load_sym_table(bf);
		}
	}

	return bf;
}

bool close_bin_file(struct bin_file * bf)
{
	bool success;

	close_sym_table(bf);
	bf_close_func_table(bf);
	bf_close_bb_table(bf);
	bf_close_insn_table(bf);
	unload_all_sections(bf);

	htable_destroy(&bf->func_table);
	htable_destroy(&bf->bb_table);
	htable_destroy(&bf->insn_table);
	symbol_table_destroy(&bf->sym_table);
	htable_destroy(&bf->mem_table);
	success = bfd_close(bf->abfd);

	free(bf->output_path);
	free(bf);
	return success;
}

struct basic_blk * disasm_bin_file_entry(struct bin_file * bf)
{
	bfd_vma vma = bfd_get_start_address(bf->abfd);
	return disasm_generate_cflow(bf, vma, TRUE);
}

struct basic_blk * disasm_bin_file_sym(struct bin_file * bf,
		struct symbol * sym, bool is_func)
{
	return disasm_from_sym(bf, sym, is_func);
}
