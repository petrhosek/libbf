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

/**
 * @file bf_sym.h
 * @brief Definition and API of bf_sym.
 * @details bf_sym is responsible for storing the symbol table of a BFD in a
 * persistent way. The raison d'etre for this module is because although BFD
 * allows us to fetch the symbol table, we would have to free it in its own
 * specific way. By storing our own copy, we are able to customise the
 * interactions to be consistent with the rest of our API.
 *
 * Another reason for not using BFD directly for symbol access is that BFD
 * provides symbols as a list. Given that we need to constantly access symbols
 * whenever bf_basic_blk and bf_func objects are created, it is more efficient
 * to store the symbols in a hashtable.
 *
 * Internally, the bf_sym module interacts with the bin_file.sym_table.
 * The functions for interacting with this table are not exposed however
 * (they will never be used externally), except for bf_close_sym_table().
 * @author Mike Kwan <michael.kwan08@imperial.ac.uk>
 */

#ifndef SYMBOL_H_
#define SYMBOL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/cdefs.h>
#include <sys/param.h>
#include <err.h>
#include <fnmatch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <bfd.h>
#include <libelf.h>

#include <libkern/jhash.h>
#include <libkern/hlist.h>
#include <libkern/list.h>
#include <libkern/rbtree.h>

struct bin_file;

#if defined(__x86_64__)
typedef Elf64_Ehdr Elf_Ehdr;
typedef Elf64_Shdr Elf_Shdr;
typedef Elf64_Sym  Elf_Sym;
typedef Elf64_Addr Elf_Addr;
typedef Elf64_Word Elf_Word;
typedef Elf64_Half Elf_Half;
#elif defined(__i386__)
typedef Elf32_Ehdr Elf_Ehdr;
typedef Elf32_Shdr Elf_Shdr;
typedef Elf32_Sym  Elf_Sym;
typedef Elf32_Addr Elf_Addr;
typedef Elf32_Word Elf_Word;
typedef Elf32_Half Elf_Half;
#else
#error Unsupported target platform
#endif

enum symbol_type {
  SYMBOL_UNDEFINED  = 0,    /* the symbol is undefined */
  SYMBOL_LOCAL      = 0x1,  /* the symbol has local scope */
  SYMBOL_GLOBAL     = 0x2,  /* the symbol has global scope */
  SYMBOL_FUNCTION   = 0x4,
  SYMBOL_OBJECT     = 0x8,
  SYMBOL_DYNAMIC    = 0x10,
  SYMBOL_WEAK       = 0x20,
  SYMBOL_DEBUGGING  = 0x40,
  SYMBOL_COMMON     = 0x80,
};

enum plt_type {
  PLT_NONE = 0,   /* PLT not used for this symbol. */
  PLT_EXEC,       /* PLT for this symbol is executable. */
  PLT_POINT       /* PLT for this symbol is a non-executable. */
};

/**
 * @struct bf_sym
 * @brief <b>libbf</b>'s abstraction of a symbol.
 * @details Currently a symbol consists of an address and a name. This can be
 * easily extended if we need more of the information from the original
 * asymbol structure.
 */
struct symbol {
  /** The symbol name */
  char *name;
  /** The symbol address */
  bfd_vma address;
  /** The symbol size */
  size_t size;
  /** The encompassing section name */
  const char *section;
  /** The symbol type */
  enum symbol_type type;
  /** BFD for symbol, if applicable */
  asymbol *asymbol;
  /** The symbol PLT type */
  enum plt_type plt_type;
  /** Symbol hash */
  struct hlist_node symbol_hash;
  /** Symbol tree */
  struct rb_node rb_symbol;
};

struct call_site {
  struct list_head caller, callee;
  struct symbol *from, *to;
  void *addr, *ret_addr;
  size_t size;
  int index;
};

struct symbol_table {
  struct hlist_head *symbol_hash;
  struct rb_root rb_symbol;
};

#define symbol_hashfn(n) jhash(n, strlen(n), 0) & (symbolhash_size - 1)
#define symbolhash_size 16
#define symbolhash_shift 4

#define symbolhash_entry(node) hlist_entry((node), struct symbol, symbol_hash)

extern struct symbol *symbol_find(struct symbol_table *table, const char *name);
extern void symbol_add(struct symbol_table *table, struct symbol *sym);

/**
 * Iterate over all symbols.
 * @param sym The symbol pointer to use as a loop cursor
 * @param hash The symbol table
 */
#define for_each_symbol(sym, table) \
    for (int i = 0; i < symbolhash_size; ++i) \
        for (sym = symbolhash_entry((table)->symbol_hash[i].first); \
             &sym->symbol_hash; \
             sym = symbolhash_entry(sym->symbol_hash.next))

extern struct symbol *rb_search_symbol(struct symbol_table *table, void *addr);
extern struct symbol *rb_insert_symbol(struct symbol_table *table, void *addr, struct rb_node *node);

extern void symbol_table_init(struct symbol_table *table);
extern void symbol_table_destroy(struct symbol_table *table);

extern int load_sym_table(struct bin_file * bf);
extern void close_sym_table(struct bin_file * bf);

#ifdef __cplusplus
}
#endif

#endif // SYMBOL_H_
