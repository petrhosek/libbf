#include "bf_copy.h"

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

void copy_symtable(bfd * ibfd, bfd * obfd, struct COPYSECTION_DATA * csd)
{
	/* set obfd */
	csd->obfd     = obfd;

	/* get required size for symbol table and allocate memory */
	csd->symsize  = bfd_get_symtab_upper_bound(ibfd);
	csd->syms     = xmalloc(csd->symsize);

	csd->symcount =  bfd_canonicalize_symtab (ibfd, csd->syms);

	/* copy symbol table to obfd */
	bfd_set_symtab(obfd, csd->syms, csd->symcount);
}

static bool copy_object(bfd * ibfd, bfd * obfd)
{
	struct COPYSECTION_DATA csd = {0};

	/* set output parameters to ibfd settings */
	bfd_set_format(obfd, bfd_get_format(ibfd));
	bfd_set_arch_mach(obfd, bfd_get_arch(ibfd), bfd_get_mach(ibfd));
	bfd_set_file_flags(obfd, bfd_get_file_flags(ibfd) &
			bfd_applicable_file_flags(obfd));

	/* set the entry point of obfd */
	bfd_set_start_address(obfd, bfd_get_start_address(ibfd));

	/* define sections for output file */
	bfd_map_over_sections(ibfd, define_section, obfd);

	/* copy private data into obfd */
	bfd_copy_private_bfd_data(ibfd, obfd);

	copy_symtable(ibfd, obfd, &csd);

	bfd_map_over_sections(ibfd, copy_section, &csd);

	free(csd.syms);
	return TRUE;
}

bfd * bf_create_writable_bfd(bfd * abfd, char * output_path)
{
	bfd * obfd = bfd_openw(output_path, NULL);
	bfd_set_format(obfd, bfd_object);

	if(copy_object(abfd, obfd)) {
		perror("Error copying BFD object.");
		xexit(-1);
	}

	return obfd;
}
