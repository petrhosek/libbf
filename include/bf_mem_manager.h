/**
 * @internal
 * @file bf_mem_manager.h
 * @brief API of bf_mem_manager.
 * @details bf_mem_manager is responsible for mapping sections of the target
 * into the local memory. One important aspect of this is to make sure that
 * no section is mapped more than once. <b>libind</b>'s method of locating
 * sections is borrowed from <b>libopdis</b>.
 *
 * Internally, bf_mem_manager stores bf_mem_block objects within the
 * binary_file.mem_table hashtable. The functions for interacting with this
 * table are not exposed however (they will never be used externally), except
 * for unload_all_sections().
 * @author Mike Kwan <michael.kwan08@imperial.ac.uk>
 */

#ifndef BF_MEM_MANAGER_H
#define BF_MEM_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "binary_file.h"

/**
 * @struct bf_mem_block
 * @brief Each bf_mem_block represents a section mapping.
 */
struct bf_mem_block {
  /**
   * @var entry
   * @brief Entry into the binary_file.mem_table hashtable of
   * binary_file.
   */
  struct htable_entry entry;

  /**
   * @var section
   * @brief The mapped section.
   * @note This is defined in bfd.h in the binutils distribution.
   */
  asection * section;

  /**
   * @var buffer_vma
   * @brief The VMA of the section within the target.
   */
  bfd_vma buffer_vma;

  /**
   * @var buffer_length
   * @brief The size of the section.
   */
  unsigned int buffer_length;

  /**
   * @var buffer
   * @brief The VMA of the mapping within the local memory.
   */
  bfd_byte * buffer;
};

/**
 * @brief Locates the section containing a VMA and loads it.
 * @param bf The binary_file being analysed.
 * @param vma The VMA of the instruction we want to locate the section of.
 * @return A bf_mem_block representing the mapped memory.
 * @details If the section has already been loaded, this function will not
 * reload it.
 */
struct bf_mem_block * load_section_for_vma(struct bin_file * bf, bfd_vma vma);

/**
 * @brief Unloads all sections mapped in by bf_mem_manager.
 * @param bf The binary_file holding the binary_file.mem_table to be purged.
 */
void unload_all_sections(struct bin_file * bf);

#ifdef __cplusplus
}
#endif

#endif
