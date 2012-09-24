/*
 * This file is part of libbf.
 *
 * libbf is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * libbf is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with libbf.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SECTION_H_
#define SECTION_H_

#include <elf.h>
#include <libelf.h>

#include <libkern/hlist.h>
#include <libkern/jhash.h>
#include <libkern/list.h>

#if defined(__x86_64__)
typedef Elf64_Ehdr Elf_Ehdr;
typedef Elf64_Shdr Elf_Shdr;
typedef Elf64_Sym  Elf_Sym;
typedef Elf64_Addr Elf_Addr;
#elif defined(__i386__)
typedef Elf32_Ehdr Elf_Ehdr;
typedef Elf32_Shdr Elf_Shdr;
typedef Elf32_Sym  Elf_Sym;
typedef Elf32_Addr Elf_Addr;
#else
#error Unsupported target platform
#endif

/* Internal data structure for sections. */
struct section {
  struct segment *seg;  /* containing segment */
  const char *name; /* section name */
  int idx; /* secton index */
  Elf_Shdr shdr; /* section header */
  Elf_Scn *is;  /* input scn */
  Elf_Scn *os;  /* output scn */
  void *buf;  /* section content */
  uint8_t *pad; /* section padding */
  uint64_t off; /* section offset */
  uint64_t sz;  /* section size */
  uint64_t cap; /* section capacity */
  uint64_t align; /* section alignment */
  uint64_t type;  /* section type */
  uint64_t vma; /* section virtual addr */
  uint64_t lma; /* section load addr */
  uint64_t pad_sz;/* section padding size */
  int loadable; /* whether loadable */
  int pseudo;
  int nocopy;

  struct list_head sections; /* list of all sections */
  struct list_head segment_entry; /* list of sections in a segment */
  struct hlist_node section_hash;
};

struct section_table {
  struct hlist_head *section_hash;
};

#define section_hashfn(n) jhash(n, strlen(n), 0) & (sectionhash_size - 1)
#define sectionhash_size 16
#define sectionhash_shift 4

#define sectionhash_entry(node) hlist_entry((node), struct section, section_hash)

extern void section_init(struct section *s, const char *name, int idx, Elf_Shdr shdr);
extern struct section *section_find(struct section_table *table, const char *name);
extern void section_add(struct section_table *table, struct section *scn);

extern void section_table_init(struct section_table *table);
extern void section_table_destroy(struct section_table *table);

/**
 * Iterate over all sections.
 * @param scn The section pointer to use as a loop cursor
 * @param table The secion table
 */
#define for_each_section(scn, table) \
    for (int i = 0; i < symbolhash_size; ++i) \
        for (scn = sectionhash_entry(h(table)->symbol_hash[i].first); \
             &scn->section_hash; \
             scn = sectionhash_entry(hash.next))

#endif // SECTION_H_
