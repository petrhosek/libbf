#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <bfd.h>
#include <libiberty.h>

bool get_target_path(char * target_path, size_t size)
{
	if(getcwd(target_path, size) == NULL) {
		return FALSE;
	} else {
		strncat(target_path, "/example-target",
				size - strlen(target_path) - 1);
		return TRUE;
	}
}

bool get_obj_path(char * target_path, size_t size)
{
	if(getcwd(target_path, size) == NULL) {
		return FALSE;
	} else {
		strncat(target_path, "/example-embedobj",	
				size - strlen(target_path) - 1);
		return TRUE;
	}
}

bfd * load_obj()
{
	bfd * abfd;
	char  target_path[FILENAME_MAX] = {0};
	get_obj_path(target_path, ARRAY_SIZE(target_path));

	abfd = bfd_openr(target_path, NULL);
	bfd_check_format(abfd, bfd_object);
	return abfd;
}

bfd * load_target()
{
	bfd * abfd;
	char  target_path[FILENAME_MAX] = {0};
	get_target_path(target_path, ARRAY_SIZE(target_path));

	abfd = bfd_openr(target_path, NULL);
	bfd_check_format(abfd, bfd_object);
	return abfd;
}

void print_sec_name(bfd * abfd, asection * s, void * data)
{
	printf("--section-start=%s=0x%lX,", s->name, s->vma +
			*(bfd_vma *)data);
}

void gen_linker_command(bfd_vma vma)
{
	bfd * abfd = load_obj();

	puts("Edit the makefile with the following link command:");
	printf("-Wl,");
	bfd_map_over_sections(abfd, print_sec_name, &vma);
	puts("");

	bfd_close(abfd);
}

void get_suitable_base(bfd * abfd, asection * s, void * data)
{
	if(s->vma + s->size > *(bfd_vma *)data) {
		*(bfd_vma *)data = s->vma + s->size;
	}
}

bfd_vma get_base_of_embed_obj(void)
{
	bfd *   abfd = load_target();
	bfd_vma vma  = 0;

	bfd_map_over_sections(abfd, get_suitable_base, &vma);

	bfd_close(abfd);
	return vma;
}

int main(void)
{
	bfd_vma vma;

	bfd_init();
	vma = get_base_of_embed_obj();

	printf("Found upper limit of target's sections to be 0x%lx\n", vma);

	if((vma / 0x1000) * 0x1000 != vma) {
		vma = ((vma / 0x1000) + 1) * 0x1000;
		printf("Rounded this to 0x%lx for alignment\n\n", vma);
	}

	gen_linker_command(vma);
	return EXIT_SUCCESS;
}
