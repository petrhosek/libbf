#include <stdlib.h>
#include <bfd.h>

int main(void)
{
	bfd * abfd_target;
	bfd * abfd_embed;

	abfd_target = bfd_openw("/home/mike/Desktop/Embed/Hello", NULL);

	if(abfd_target == NULL) {
		perror("Failed to load target.");
		xexit(-1);
	}

	abfd_embed = bfd_openr("/home/mike/Desktop/Embed/Embed.o", NULL);

	if(abfd_embed == NULL) {
		perror("Failed to load object to be embedded.");
		xexit(-1);
	}


	asection * sec = bfd_make_section(abfd_target, "lalasection");
	if(sec == NULL) {
		puts("sec was NULL");
	}

	if(!bfd_set_section_size(abfd_target, sec, 1024)) {
		perror("bfd_set_section_size returned false.");
	}

	bfd_close(abfd_target);
	bfd_close(abfd_embed);
	return EXIT_SUCCESS;
}
