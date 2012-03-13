#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <bfd.h>
#include <libiberty.h>

struct COPYSECTION_DATA {
	bfd *	   obfd;
	asymbol ** syms;
	int        symsize;
	int        symcount;
};

void copy_section(bfd * ibfd, asection * section, PTR data)
{
	struct COPYSECTION_DATA * csd  = data;
	bfd *			  obfd = csd->obfd;
	asection *		  s;
	long			  size, count, sz_reloc;

	if((bfd_get_section_flags(ibfd, section) & SEC_GROUP) != 0) {
		return;
	}

	/* get output section from input section struct */
	s        = section->output_section;
	/* get sizes for copy */
	size     = bfd_get_section_size(section);
	sz_reloc = bfd_get_reloc_upper_bound(ibfd, section);

	if(!sz_reloc) {
		/* no relocations */
		bfd_set_reloc(obfd, s, NULL, 0);
	} else if(sz_reloc > 0) {
		arelent ** buf;

		/* build relocations */
		buf   = xmalloc(sz_reloc);
		count = bfd_canonicalize_reloc(ibfd, section, buf, csd->syms);
		/* set relocations for the output section */
		bfd_set_reloc(obfd, s, count ? buf : NULL, count);
		free(buf);
	}

	/* get input section contents, set output section contents */
	if(section->flags & SEC_HAS_CONTENTS) {
		bfd_byte * memhunk = NULL;
		bfd_get_full_section_contents(ibfd, section, &memhunk);
		bfd_set_section_contents(obfd, s, memhunk, 0, size);
		free(memhunk);
	}
}

void define_section(bfd * ibfd, asection * section, PTR data)
{
	bfd * 	   obfd = data;
	asection * s	= bfd_make_section_anyway_with_flags(obfd,
			section->name, bfd_get_section_flags(ibfd, section));
	/* set size to same as ibfd section */
	bfd_set_section_size(obfd, s, bfd_section_size(ibfd, section));

	/* set vma */
	bfd_set_section_vma(obfd, s, bfd_section_vma(ibfd, section));
	/* set load address */
	s->lma = section->lma;
	/* set alignment -- the power 2 will be raised to */
	bfd_set_section_alignment(obfd, s,
			bfd_section_alignment(ibfd, section));
	s->alignment_power = section->alignment_power;
	/* link the output section to the input section */
	section->output_section = s;
	section->output_offset  = 0;

	/* copy merge entity size */
	s->entsize = section->entsize;

	/* copy private BFD data from ibfd section to obfd section */
	bfd_copy_private_section_data(ibfd, section, obfd, s);
}

void merge_symtable(bfd * ibfd, bfd * embedbfd, bfd * obfd,
		struct COPYSECTION_DATA * csd)
{
	/* set obfd */
	csd->obfd     = obfd;

	/* get required size for both symbol tables and allocate memory */
	csd->symsize  = bfd_get_symtab_upper_bound(ibfd) +
			bfd_get_symtab_upper_bound(embedbfd);
	csd->syms     = xmalloc(csd->symsize);

	csd->symcount = bfd_canonicalize_symtab (ibfd, csd->syms) +
			bfd_canonicalize_symtab (embedbfd,
			csd->syms + csd->symcount);

	/* copy merged symbol table to obfd */
	bfd_set_symtab(obfd, csd->syms, csd->symcount);
}

bool merge_object(bfd * ibfd, bfd * embedbfd, bfd * obfd)
{
	struct COPYSECTION_DATA csd = {0};

	if(!ibfd || !embedbfd || !obfd) {
		return FALSE;
	}

	/* set output parameters to ibfd settings */
	bfd_set_format(obfd, bfd_get_format(ibfd));
	bfd_set_arch_mach(obfd, bfd_get_arch(ibfd), bfd_get_mach(ibfd));
	bfd_set_file_flags(obfd, bfd_get_file_flags(ibfd) &
			bfd_applicable_file_flags(obfd));

	/* set the entry point of obfd */
	bfd_set_start_address(obfd, bfd_get_start_address(ibfd));

	/* define sections for output file */
	bfd_map_over_sections(ibfd, define_section, obfd);
	bfd_map_over_sections(embedbfd, define_section, obfd);

	/* merge private data into obfd */
	bfd_merge_private_bfd_data(ibfd, obfd);
	bfd_merge_private_bfd_data(embedbfd, obfd);

	merge_symtable(ibfd, embedbfd, obfd, &csd);

	bfd_map_over_sections(ibfd, copy_section, &csd);
	bfd_map_over_sections(embedbfd, copy_section, &csd);

	free(csd.syms);

	/* not sure if the second call overwrites the first */
	bfd_copy_private_bfd_data(ibfd, obfd);
	bfd_copy_private_bfd_data(embedbfd, obfd);

	return TRUE;
}

int main(int argc, char **argv)
{
	bfd * ibfd;
	bfd * embedbfd;
	bfd * obfd;

	if(argc != 4) {
		perror("Usage: infile embedfile outfile\n");
		xexit(-1);
	}

	bfd_init();
	ibfd     = bfd_openr(argv[1], NULL);
	embedbfd = bfd_openr(argv[2], NULL);

	if(ibfd == NULL || embedbfd == NULL) {
		perror("asdfasdf");
		xexit(-1);
	}

	if(!bfd_check_format(ibfd, bfd_object) ||
			!bfd_check_format(embedbfd, bfd_object)) {
		perror("File format error");
		xexit(-1);
	}

	obfd = bfd_openw(argv[3], NULL);
	bfd_set_format(obfd, bfd_object);

	if(!(merge_object(ibfd, embedbfd, obfd))) {
		perror("Error merging input/obj");
		xexit(-1);
	}

	bfd_close(ibfd);
	bfd_close(embedbfd);
	bfd_close(obfd);
	return EXIT_SUCCESS;
}
