#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <bfd.h>
#include <libiberty.h>
#include "../lib/bfd/elf-bfd.h"

/* What kind of change to perform.  */
enum change_action
{
  CHANGE_IGNORE,
  CHANGE_MODIFY,
  CHANGE_SET
};

/* Structure used to hold lists of sections and actions to take.  */
struct section_list
{
  struct section_list * next;	   /* Next section to change.  */
  const char *		name;	   /* Section name.  */
  bfd_boolean		used;	   /* Whether this entry was used.  */
  bfd_boolean		remove;	   /* Whether to remove this section.  */
  bfd_boolean		copy;	   /* Whether to copy this section.  */
  enum change_action	change_vma;/* Whether to change or set VMA.  */
  bfd_vma		vma_val;   /* Amount to change by or set to.  */
  enum change_action	change_lma;/* Whether to change or set LMA.  */
  bfd_vma		lma_val;   /* Amount to change by or set to.  */
  bfd_boolean		set_flags; /* Whether to set the section flags.	 */
  flagword		flags;	   /* What to set the section flags to.	 */
};

/*
 * Globals required by objcopy.
 */
struct section_list *change_sections;
asymbol **isympp = NULL;	/* Input symbols.  */
asymbol **osympp = NULL;	/* Output symbols that survive stripping.  */
int status = 0;		/* Exit status.  */

/* Return a pointer to the symbol used as a signature for GROUP.  */

asymbol * group_signature (asection *group)
{
  bfd *abfd = group->owner;
  Elf_Internal_Shdr *ghdr;

  if (bfd_get_flavour (abfd) != bfd_target_elf_flavour)
    return NULL;

  ghdr = &elf_section_data (group)->this_hdr;
  if (ghdr->sh_link < elf_numsections (abfd))
    {
      const struct elf_backend_data *bed = get_elf_backend_data (abfd);
      Elf_Internal_Shdr *symhdr = elf_elfsections (abfd) [ghdr->sh_link];

      if (symhdr->sh_type == SHT_SYMTAB
	  && ghdr->sh_info < symhdr->sh_size / bed->s->sizeof_sym)
	return isympp[ghdr->sh_info - 1];
    }
  return NULL;
}

/* Find and optionally add an entry in the change_sections list.  */

struct section_list * find_section_list (const char *name, bfd_boolean add)
{
  struct section_list *p;

  for (p = change_sections; p != NULL; p = p->next)
    if (strcmp (p->name, name) == 0)
      return p;

  if (! add)
    return NULL;

  p = (struct section_list *) xmalloc (sizeof (struct section_list));
  p->name = name;
  p->used = FALSE;
  p->remove = FALSE;
  p->copy = FALSE;
  p->change_vma = CHANGE_IGNORE;
  p->change_lma = CHANGE_IGNORE;
  p->vma_val = 0;
  p->lma_val = 0;
  p->set_flags = FALSE;
  p->flags = 0;

  p->next = change_sections;
  change_sections = p;

  return p;
}

/* Check the section rename list for a new name of the input section
   ISECTION.  Return the new name if one is found.
   Also set RETURNED_FLAGS to the flags to be used for this section.  */

const char * find_section_rename (bfd * ibfd ATTRIBUTE_UNUSED,
		sec_ptr isection,
		flagword * returned_flags)
{
  const char * old_name = bfd_section_name (ibfd, isection);
  /* Default to using the flags of the input section.  */
  * returned_flags = bfd_get_section_flags (ibfd, isection);
  return old_name;
}

/* Create a section in OBFD with the same
   name and attributes as ISECTION in IBFD.  */
void setup_section (bfd *ibfd, sec_ptr isection, void *obfdarg)
{
  bfd *obfd = (bfd *) obfdarg;
  struct section_list *p;
  sec_ptr osection;
  bfd_size_type size;
  bfd_vma vma;
  bfd_vma lma;
  flagword flags;
  const char * name;

  p = find_section_list (bfd_section_name (ibfd, isection), FALSE);
  if (p != NULL)
    p->used = TRUE;

  /* Get the, possibly new, name of the output section.  */
  name = find_section_rename (ibfd, isection, & flags);

  if (p != NULL && p->set_flags)
    flags = p->flags | (flags & (SEC_HAS_CONTENTS | SEC_RELOC));

  osection = bfd_make_section_anyway_with_flags (obfd, name, flags);

  if (osection == NULL)
    {
      puts("Failed to create output section");
      goto loser;
    }

  size = bfd_section_size (ibfd, isection);

  if (! bfd_set_section_size (obfd, osection, size))
    {
      puts("Failed to set size");
      goto loser;
    }

  vma = bfd_section_vma (ibfd, isection);
  if (p != NULL && p->change_vma == CHANGE_MODIFY)
    vma += p->vma_val;
  else if (p != NULL && p->change_vma == CHANGE_SET)
    vma = p->vma_val;

  if (! bfd_set_section_vma (obfd, osection, vma))
    {
      puts("Failed to set vma");
      goto loser;
    }

  lma = isection->lma;
  if ((p != NULL) && p->change_lma != CHANGE_IGNORE)
    {
      if (p->change_lma == CHANGE_MODIFY)
	lma += p->lma_val;
      else if (p->change_lma == CHANGE_SET)
	lma = p->lma_val;
      else
	abort ();
    }

  osection->lma = lma;

  /* FIXME: This is probably not enough.  If we change the LMA we
     may have to recompute the header for the file as well.  */
  if (!bfd_set_section_alignment (obfd,
				  osection,
				  bfd_section_alignment (ibfd, isection)))
    {
      puts("Failed to set alignment");
      goto loser;
    }

  /* Copy merge entity size.  */
  osection->entsize = isection->entsize;

  /* This used to be mangle_section; we do here to avoid using
     bfd_get_section_by_name since some formats allow multiple
     sections with the same name.  */
  isection->output_section = osection;
  isection->output_offset = 0;

  if ((isection->flags & SEC_GROUP) != 0)
    {
      asymbol *gsym = group_signature (isection);

      if (gsym != NULL)
	{
	  gsym->flags |= BSF_KEEP;
	  if (ibfd->xvec->flavour == bfd_target_elf_flavour)
	    elf_group_id (isection) = gsym;
	}
    }

  /* Allow the BFD backend to copy any private data it understands
     from the input section to the output section.  */
  if (!bfd_copy_private_section_data (ibfd, isection, obfd, osection))
    {
      puts("Failed to copy private data");
      goto loser;
    }

  /* All went well.  */
  return;

loser:
  status = 1;
  puts("Non fatal: Failed setup_section");
}

/* Once each of the sections is copied, we may still need to do some
   finalization work for private section headers.  Do that here.  */

static void setup_bfd_headers (bfd *ibfd, bfd *obfd)
{
  /* Allow the BFD backend to copy any private data it understands
     from the input section to the output section.  */
  if(!bfd_merge_private_bfd_data(ibfd, obfd)) {
    status = 1;
    puts("Non fatal: Failed merging private BFD data");
  }

  /* All went well.  */
  return;
}

/* Copy the data of input section ISECTION of IBFD
   to an output section with the same name in OBFD.
   If stripping then don't copy any relocation info.  */
void copy_section (bfd *ibfd, sec_ptr isection, void *obfdarg)
{
  bfd *obfd = (bfd *) obfdarg;
  struct section_list *p;
  arelent **relpp;
  long relcount;
  sec_ptr osection;
  bfd_size_type size;
  long relsize;
  flagword flags;

  /* If we have already failed earlier on,
     do not keep on generating complaints now.  */
  if (status != 0)
    return;

  flags = bfd_get_section_flags (ibfd, isection);
  if ((flags & SEC_GROUP) != 0)
    return;

  osection = isection->output_section;
  size = bfd_get_section_size (isection);

  if (size == 0 || osection == 0)
    return;

  p = find_section_list (bfd_get_section_name (ibfd, isection), FALSE);

  /* Core files do not need to be relocated.  */
  if (bfd_get_format (obfd) == bfd_core)
    relsize = 0;
  else
    {
      relsize = bfd_get_reloc_upper_bound (ibfd, isection);

      if (relsize < 0)
	{
	  /* Do not complain if the target does not support relocations.  */
	  if (relsize == -1 && bfd_get_error () == bfd_error_invalid_operation)
	    relsize = 0;
	  else
	    {
	      status = 1;
              puts("Non fatal: Problem getting relocation info");
	      return;
	    }
	}
    }

  if (relsize == 0)
    bfd_set_reloc (obfd, osection, NULL, 0);
  else
    {
      relpp = (arelent **) xmalloc (relsize);
      relcount = bfd_canonicalize_reloc (ibfd, isection, relpp, isympp);
      if (relcount < 0)
	{
	  status = 1;
          puts("Non fatal: Relocation count is negative");
	  return;
	}

      bfd_set_reloc (obfd, osection, relcount == 0 ? NULL : relpp, relcount);
      if (relcount == 0)
	free (relpp);
    }

  if (bfd_get_section_flags (ibfd, isection) & SEC_HAS_CONTENTS
      && bfd_get_section_flags (obfd, osection) & SEC_HAS_CONTENTS)
    {
      bfd_byte *memhunk = NULL;

      if (!bfd_get_full_section_contents (ibfd, isection, &memhunk))
	{
	  status = 1;
          puts("Non fatal: Failed getting full section contents");
	  return;
	}

      if (!bfd_set_section_contents (obfd, osection, memhunk, 0, size))
	{
	  status = 1;
          puts("Non fatal: Failed setting section contents");
	  return;
	}
      free (memhunk);
    }
  else if (p != NULL && p->set_flags && (p->flags & SEC_HAS_CONTENTS) != 0)
    {
      void *memhunk = xmalloc (size);

      /* We don't permit the user to turn off the SEC_HAS_CONTENTS
	 flag--they can just remove the section entirely and add it
	 back again.  However, we do permit them to turn on the
	 SEC_HAS_CONTENTS flag, and take it to mean that the section
	 contents should be zeroed out.  */

      memset (memhunk, 0, size);
      if (! bfd_set_section_contents (obfd, osection, memhunk, 0, size))
	{
	  status = 1;
          puts("Non fatal: Failed setting section contents");
	  return;
	}
      free (memhunk);
    }
}

bfd_boolean copy_object (bfd *ibfd, bfd *objbfd, bfd *obfd,
		const bfd_arch_info_type *input_arch)
{
  bfd_vma start;
  long symcount;
  long symsize;
  enum bfd_architecture iarch;
  unsigned int imach;

  if (ibfd->xvec->byteorder != obfd->xvec->byteorder
      && ibfd->xvec->byteorder != BFD_ENDIAN_UNKNOWN
      && obfd->xvec->byteorder != BFD_ENDIAN_UNKNOWN)
    puts("Fatal: Unable to change endianness of input file(s)");

  start = bfd_get_start_address (ibfd);

  if (!bfd_set_format (obfd, bfd_get_format (ibfd)))
    {
      puts("Non fatal: Unable to set format of output");
      return FALSE;
    }

  /* Neither the start address nor the flags
     need to be set for a core file.  */
  if (bfd_get_format (obfd) != bfd_core)
    {
      flagword flags;

      flags = bfd_get_file_flags (ibfd);
      flags &= bfd_applicable_file_flags (obfd);

      if (!bfd_set_start_address (obfd, start)
	  || !bfd_set_file_flags (obfd, flags))
	{
          puts("Non fatal: Unable to set output start address or file flags.");
	  return FALSE;
	}
    }

  /* Copy architecture of input file to output file.  */
  iarch = bfd_get_arch (ibfd);
  imach = bfd_get_mach (ibfd);
  if (input_arch)
    {
      if (bfd_get_arch_info (ibfd) == NULL
	  || bfd_get_arch_info (ibfd)->arch == bfd_arch_unknown)
	{
	  iarch = input_arch->arch;
	  imach = input_arch->mach;
	}
    }
  if (!bfd_set_arch_mach (obfd, iarch, imach)
      && (ibfd->target_defaulted
	  || bfd_get_arch (ibfd) != bfd_get_arch (obfd)))
    {
      if (bfd_get_arch (ibfd) == bfd_arch_unknown)
        puts("Non fatal: Unable to recognise the format of the input file");
      else
        puts("Non fatal: Output file cannot represent architecture");
      return FALSE;
    }

  if (!bfd_set_format (obfd, bfd_get_format (ibfd)))
    {
      puts("Non fatal: Unable to set format of output");
      return FALSE;
    }

  symsize = bfd_get_symtab_upper_bound (ibfd) +
		bfd_get_symtab_upper_bound (objbfd);
  if (symsize < 0)
    {
      puts("Non fatal: symtabs of ibfd and objbfd were 0 bytes");
      return FALSE;
    }

  osympp = isympp = (asymbol **) xmalloc (symsize);
  symcount = bfd_canonicalize_symtab (ibfd, isympp);
  symcount += bfd_canonicalize_symtab (objbfd, isympp + symcount);
  if (symcount < 0)
    {
      puts("Unable to canonicalize symtab of input");
      return FALSE;
    }

  /* BFD mandates that all output sections be created and sizes set before
     any output is done.  Thus, we traverse all sections multiple times.  */
  bfd_map_over_sections (ibfd, setup_section, obfd);
  bfd_map_over_sections (objbfd, setup_section, obfd);

  setup_bfd_headers (ibfd, obfd);
  setup_bfd_headers (objbfd, obfd);

  bfd_set_symtab (obfd, osympp, symcount);

  /* This has to happen after the symbol table has been set.  */
  bfd_map_over_sections (ibfd, copy_section, obfd);
  bfd_map_over_sections (objbfd, copy_section, obfd);


  /* Allow the BFD backend to copy any private data it understands
     from the input BFD to the output BFD.  This is done last to
     permit the routine to look at the filtered symbol table, which is
     important for the ECOFF code at least.  */
  if (! bfd_copy_private_bfd_data (ibfd, obfd))
    {
      puts("Non fatal: Error copying private BFD data");
      return FALSE;
    }

  if (! bfd_copy_private_bfd_data (objbfd, obfd))
    {
      puts("Non fatal: Error copying private BFD data");
      return FALSE;
    }

  return TRUE;
}

bfd_vma get_sym_vma(bfd * abfd, char * sym)
{
	long    storage_needed = bfd_get_symtab_upper_bound(abfd);
	bfd_vma vma	       = 0;

	if(storage_needed < 0) {
		return 0;
	} else if(storage_needed == 0) {
		return 0;
	} else {
		asymbol **symbol_table    = xmalloc(storage_needed);
		long    number_of_symbols = bfd_canonicalize_symtab(abfd, symbol_table);

		if(number_of_symbols < 0) {
			free(symbol_table);
			return 0;
		} else {
			for(long i = 0; i < number_of_symbols; i++) {
				if(strcmp(symbol_table[i]->name, sym) == 0) {
					vma = symbol_table[i]->udata.i;
					break;
				}
			}
		}

		free(symbol_table);
		return vma;
	}
}

int main(int argc, char **argv)
{
	bfd *			   ibfd;
	bfd *			   objbfd;
	bfd *			   obfd;
	asection *		   asec;
	const bfd_arch_info_type * iarch;

	bfd_init();
	ibfd   = bfd_openr(argv[1], NULL);
	objbfd = bfd_openr(argv[2], NULL);

	if(!bfd_check_format(ibfd, bfd_object) ||
			!bfd_check_format(objbfd, bfd_object)) {
		perror("File format error");
		xexit(-1);
	}

	obfd = bfd_openw(argv[3], NULL);
	bfd_set_format(obfd, bfd_object);

	iarch = bfd_get_arch_info(ibfd);
	if(iarch == NULL) {
		perror("Error getting architecture of ibfd");
		xexit(-1);
	}

	if(!(copy_object(ibfd, objbfd, obfd, iarch))) {
		perror("Error merging input/obj");
		xexit(-1);
	}

	asec = bfd_get_section_by_name(obfd, ".text");
	if(asec == NULL) {
		perror("Unable to find .text section in output");
		xexit(-1);
	} else {
		bfd_byte buf[asec->size];
		bfd_vma  vma = get_sym_vma(obfd, "func1");

		/*if(vma == 0) {
			perror("Unable to find func1");
			xexit(-1);
		}*/

		bfd_get_section_contents(obfd, asec, buf, 0, asec->size);
		buf[0xF2]		  = 0xE8;
		*(uint32_t *)(buf + 0xF3) = vma - 0xF2 - 5;

		/*if(!bfd_set_section_contents(obfd, asec, buf, 0, asec->size)) {
			perror("Unable to update .text section of obfd");
			xexit(-1);
		}*/
	}

	bfd_close(ibfd);
	bfd_close(objbfd);
	bfd_close(obfd);
	return EXIT_SUCCESS;
}
