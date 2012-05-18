#include "bf_copy.h"

/* Symbols and Sections */
static asymbol **isympp = NULL;	/* Input symbols.  */
static asymbol **osympp = NULL;	/* Output symbols that survive stripping.  */

void my_debug(char *msg){
  if(FALSE) printf("debug: %s\n", msg);
}

/*
 * Return a pointer to the symbol used as a signature for GROUP.
 */
static asymbol * group_signature (asection *group) {
	bfd *abfd = group->owner;
	Elf_Internal_Shdr *ghdr;

	if (bfd_get_flavour (abfd) != bfd_target_elf_flavour) {
		return NULL;
	}

	ghdr = &elf_section_data (group)->this_hdr;
	if (ghdr->sh_link < elf_numsections (abfd)) {
		const struct elf_backend_data *bed =
				get_elf_backend_data (abfd);
		Elf_Internal_Shdr *symhdr =
				elf_elfsections (abfd) [ghdr->sh_link];

		if (symhdr->sh_type == SHT_SYMTAB &&
				ghdr->sh_info < symhdr->sh_size / bed->s->sizeof_sym) {
			return isympp[ghdr->sh_info - 1];
		}
	}

	return NULL;
}

/*
 * Copy the data of input section ISECTION of IBFD to an output section with
 * the same name in OBFD.
 */
static void copy_section (bfd *ibfd, sec_ptr isection, void *obfdarg)
{
	bfd *obfd = (bfd *) obfdarg;
	arelent **relpp;
	long relcount;
	sec_ptr osection;
	bfd_size_type size;
	long relsize;
	flagword flags;

	flags = bfd_get_section_flags (ibfd, isection);
	if ((flags & SEC_GROUP) != 0) {
		return;
	}

	osection = isection->output_section;
	size = bfd_get_section_size (isection);

	if (size == 0 || osection == 0) {
		return;
	}

	relsize = bfd_get_reloc_upper_bound (ibfd, isection);

	if (relsize < 0) {
		/*
		 * Do not complain if the target does not support relocations.
		 */
		if(relsize == -1 &&
				bfd_get_error() ==
				bfd_error_invalid_operation) {
			relsize = 0;
		} else {
			perror("Problem reading section relocation "\
					"information.");
			return;
		}
	}

	if (relsize == 0) {
		bfd_set_reloc (obfd, osection, NULL, 0);
	} else {
		relpp = (arelent **) xmalloc (relsize);
		relcount = bfd_canonicalize_reloc (ibfd, isection, relpp, isympp);

		if (relcount < 0) {
			perror("Relocation count is negative");
			return;
		}

		bfd_set_reloc (obfd, osection,
				relcount == 0 ? NULL : relpp, relcount);

		if (relcount == 0) {
			free (relpp);
		}
	}

	if (bfd_get_section_flags (ibfd, isection) & SEC_HAS_CONTENTS &&
			bfd_get_section_flags (obfd, osection) &
			SEC_HAS_CONTENTS) {
		bfd_byte *memhunk = NULL;

		if (!bfd_get_full_section_contents(ibfd, isection, &memhunk)) {
			perror("Error reading section contents");
			return;
		}

		if (!bfd_set_section_contents (obfd, osection, memhunk,
				0, size)) {
			perror("Error setting section contents");
			return;
		}

		free (memhunk);
	}
}

/*
 * Create a section in OBFD with the same name and attributes as ISECTION
 * in IBFD.
 */
static void setup_section (bfd *ibfd, sec_ptr isection, void *obfdarg)
{
	bfd *obfd = (bfd *) obfdarg;
	sec_ptr osection;
	bfd_size_type size;
	bfd_vma vma;
	bfd_vma lma;
	flagword flags = bfd_get_section_flags(ibfd, isection);
	const char * name;

	/* Get the name of the output section.  */
	name = bfd_section_name(ibfd, isection);

	osection = bfd_make_section_anyway_with_flags (obfd, name, flags);

	if (osection == NULL) {
		perror("Failed to create output section");
		goto loser;
	}

	size = bfd_section_size (ibfd, isection);

	if (! bfd_set_section_size (obfd, osection, size)) {
		perror("Failed to set size");
		goto loser;
	}

	vma = bfd_section_vma (ibfd, isection);

	if (! bfd_set_section_vma (obfd, osection, vma)) {
		perror("Failed to set vma");
		goto loser;
	}

	lma = isection->lma;

	osection->lma = lma;

	if (!bfd_set_section_alignment (obfd, osection,
			bfd_section_alignment (ibfd, isection))) {
		perror("Failed to set alignment");
		goto loser;
	}

	/* Copy merge entity size.  */
	osection->entsize = isection->entsize;

	/*
	 * This used to be mangle_section; we do here to avoid using
	 * bfd_get_section_by_name since some formats allow multiple
	 * sections with the same name.
	 */
	isection->output_section = osection;
	isection->output_offset = 0;

	if ((isection->flags & SEC_GROUP) != 0) {
		asymbol *gsym = group_signature (isection);

		if (gsym != NULL) {
			gsym->flags |= BSF_KEEP;

			if (ibfd->xvec->flavour == bfd_target_elf_flavour) {
				elf_group_id (isection) = gsym;
			}
		}
	}

	/*
	 * Allow the BFD backend to copy any private data it understands
	 * from the input section to the output section.
	 */
	if (!bfd_copy_private_section_data (ibfd, isection, obfd, osection)) {
		perror("Failed to copy private data");
		goto loser;
	}

	/* All went well. */
	return;

	loser:
		perror("setup_section failed");
}

/*
 * Once each of the sections is copied, we may still need to do some
 * finalization work for private section headers.  Do that here.
 */
static void setup_bfd_headers (bfd *ibfd, bfd *obfd)
{
	/*
	 * Allow the BFD backend to copy any private data it understands
	 * from the input section to the output section.
	 */
	if (! bfd_copy_private_header_data (ibfd, obfd)) {
		perror("Error in private header data.");
	}
}

/*
 * Copy object file IBFD onto OBFD. Returns TRUE upon success, FALSE otherwise.
 */
static bool copy_object(bfd *ibfd, bfd *obfd,
		const bfd_arch_info_type *input_arch)
{
	long symcount;
	long symsize;
	enum bfd_architecture iarch;
	unsigned int imach;

	if(ibfd->xvec->byteorder != obfd->xvec->byteorder
			&& ibfd->xvec->byteorder != BFD_ENDIAN_UNKNOWN
			&& obfd->xvec->byteorder != BFD_ENDIAN_UNKNOWN) {
		perror("Unable to change endianness of input file(s)");
	}

	if(!bfd_set_format(obfd, bfd_get_format (ibfd))) {
		perror("Problem setting format of output object.");
		return FALSE;
	} else {
		bfd_vma  start = bfd_get_start_address(ibfd);
		flagword flags;

		flags = bfd_get_file_flags(ibfd);
		flags &= bfd_applicable_file_flags(obfd);

		if(!bfd_set_start_address(obfd, start) ||
				!bfd_set_file_flags(obfd, flags)) {
			perror("Problem initialising output file with start "\
					"address and file flags.");
			return FALSE;
		}
	}

	/* Copy architecture of input file to output file.  */
	iarch = bfd_get_arch (ibfd);
	imach = bfd_get_mach (ibfd);

	if(input_arch) {
		if(bfd_get_arch_info (ibfd) == NULL ||
				bfd_get_arch_info(ibfd)->arch ==
				bfd_arch_unknown) {
			iarch = input_arch->arch;
			imach = input_arch->mach;
		}
	}

	if(!bfd_set_arch_mach (obfd, iarch, imach) &&
			(ibfd->target_defaulted ||
			bfd_get_arch (ibfd) != bfd_get_arch (obfd))) {
		if (bfd_get_arch (ibfd) == bfd_arch_unknown) {
			perror("Unable to recognise format of input file");
		} else {
			perror("Output file cannot represent architecture");
		}
	
		return FALSE;
	}

	if(!bfd_set_format (obfd, bfd_get_format (ibfd))) {
		perror("Problem setting format of output object.");
		return FALSE;
	}

	isympp = NULL;
	osympp = NULL;

	symsize = bfd_get_symtab_upper_bound (ibfd);
	if (symsize < 0) {
		perror("Failed reading symtab of input file.");
		return FALSE;
	}

	osympp = isympp = (asymbol **) xmalloc (symsize);
	symcount = bfd_canonicalize_symtab (ibfd, isympp);

	if (symcount < 0) {
		perror("Did not find any symbols when attempting to "\
				"canonicalize symtab");
		return FALSE;
	}

	/*
	 * BFD mandates that all output sections be created and sizes set
	 * before any output is done.  Thus, we traverse all sections multiple
	 * times.
	 */
	bfd_map_over_sections (ibfd, setup_section, obfd);

	setup_bfd_headers (ibfd, obfd);

	bfd_set_symtab (obfd, osympp, symcount);

	/* This has to happen after the symbol table has been set.  */
	bfd_map_over_sections (ibfd, copy_section, obfd);

	/*
	 * Allow the BFD backend to copy any private data it understands
	 * from the input BFD to the output BFD.  This is done last to
	 * permit the routine to look at the filtered symbol table, which is
	 * important for the ECOFF code at least.
	 */
	if(! bfd_copy_private_bfd_data (ibfd, obfd)) {
		perror("Error copying private BFD data");
	}

	return TRUE;
}

bfd * create_writable_bfd(bfd * abfd, char * output_path)
{
	isympp = osympp = NULL;

	bfd * obfd = bfd_openw(output_path, NULL);
	copy_object(abfd, obfd, bfd_get_arch_info(abfd));

	return obfd;
}
