#ifndef BF_MEM_MANAGER_H
#define BF_MEM_MANAGER_H

#include "binary_file.h"

/*
 * This module is responsible for mapping sections of the target into the
 * local memory.
 */

struct bf_mem_block {
	/*
	 * Entry into the hashtable of binary_file.
	 */
	struct htable_entry entry;

	/*
	 * Section.
	 */
	asection *	    section;

	/*
	 * VMA of section.
	 */
	bfd_vma		    buffer_vma;

	/*
	 * Size of section.
	 */
	unsigned int	    buffer_length;

	/*
	 * Local memory mapping of section.
	 */
	bfd_byte *	    buffer;
};

/*
 * Locates section containing a VMA and loads it. If the section has already
 * been loaded, then this function simply returns TRUE.
 */
struct bf_mem_block * load_section_for_vma(struct binary_file *, bfd_vma);

/*
 * Unloads all sections mapped in by bf_mem_manager.
 */
void unload_all_sections(struct binary_file *);

#endif
