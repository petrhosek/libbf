#include "binary_file.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>

/*
 * Example usage of the visitor pattern used for
 * binary_file_for_each_symbol.
 */
void process_symbol(asymbol * sym)
{
	//if(sym->value &= BSF_FUNCTION) {
	//	puts(sym->name);
	//}

	puts(sym->name);
}

/*
 * Currently we are hardcoding the target path based off of
 * the relative path from this executable. This is merely as
 * a convenience for testing.
 */
bool get_target_path(char* target_path, size_t size)
{
	if(getcwd(target_path, size) == NULL) {
		return FALSE;
	} else {
		int target_desc;

		strncat(target_path, "/Target/Target", size - strlen(target_path) - 1);
		target_desc = open(target_path, O_RDONLY);

		if(target_desc == -1) {
			return FALSE;
		} else {
			close(target_desc);
			return TRUE;
		}
	}
}

/*
 * Definition from opdis.
 */
typedef struct {
	bfd_vma    vma;
	asection * sec;
} BFD_VMA_SECTION;

int load_section(binary_file * bf, asection * s)
{
	int 	size	= bfd_section_size(s->owner, s);
	bfd_vma bfd_vma = bfd_section_vma(s->owner, s);

	unsigned char * buf = xmalloc(size);

	if(!bfd_get_section_contents(s->owner, s, buf, 0, size)) {
		puts("failed copying section contents");
		return 0;
	} else {
		puts("section loaded");
	}

	bf->disasm_config.section	= s;
	bf->disasm_config.buffer	= buf;
	bf->disasm_config.buffer_length = size;

	return 1;
}

void vma_in_section(bfd * abfd, asection * s, void * data)
{
	BFD_VMA_SECTION * req = data;

	if(req && req->vma >= s->vma &&
	req->vma < (s->vma + bfd_section_size(abfd, s)) ) {
		req->sec = s;
	}
}

/*
 * Mapping a function over each section to determine which section
 * the VMA lies within.
 */
int load_section_for_vma(binary_file * bf, bfd_vma vma)
{
	BFD_VMA_SECTION req = {vma, NULL};
	bfd_map_over_sections(bf->abfd, vma_in_section, &req);

	if(req.sec) {
		load_section(bf, req.sec);
		bf->disasm_config.buffer_vma	= vma;
	} else {
		puts("section couldn't be found");
	}
}

int main(void)
{
	binary_file * bf;

	char target_path[PATH_MAX] = {0};
	if(!get_target_path(target_path, ARRAY_SIZE(target_path))) {
		perror("Failed to get location of target");
		xexit(-1);
	}

	bf = load_binary_file(target_path);

	if(!bf) {
		perror("Failed loading binary_file");
		xexit(-1);
	}

/*	if(!binary_file_for_each_symbol(bf, process_symbol)) {
		perror("Failed during enumeration of symbols");
		xexit(-1);
	}*/

	bfd_vma vma = bfd_get_start_address(bf->abfd);
	printf("vma = %X", vma);
	load_section_for_vma(bf, vma);
	
	bf->disasm_config.insn_info_valid = 0;
	unsigned int size = bf->disassembler(vma, &bf->disasm_config); 

	if(!close_binary_file(bf)) {
		perror("Failed to close binary_file");
		xexit(-1);
	}

	return EXIT_SUCCESS;
}
