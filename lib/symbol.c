#include "symbol.h"
#include "binary_file.h"

struct symbol *symbol_find(struct symbol_table *table, const char *name) {
  struct hlist_head *head;
  struct hlist_node *node;
  struct symbol *s;

  head = &table->symbol_hash[symbol_hashfn(name)];
  hlist_for_each_entry(s, node, head, symbol_hash) {
    if (strcmp(s->name, name) == 0)
      return s;
  }

  return NULL;
}

void symbol_add(struct symbol_table *table, struct symbol *sym) {
  struct hlist_head *head;
  struct hlist_node *node;
  struct symbol *s;

  head = &table->symbol_hash[symbol_hashfn(sym->name)];
  hlist_for_each_entry(s, node, head, symbol_hash) {
    if (strcmp(s->name, sym->name) == 0)
      return;
  }

  hlist_add_head(&sym->symbol_hash, head);
}

#define rb_entry_symbol(node) rb_entry((node), struct symbol, rb_symbol)

/**
 * @brief Gets the symbol for address.
 * @param table The symbol table to be searched.
 * @param addr The address of the symbol being searched for.
 * @return The symbol starting at @p addr or NULL if no symbol has been
 * discovered at that address.
 */
struct symbol *rb_search_symbol(struct symbol_table *table, void *addr) {
  struct rb_node *n = table->rb_symbol.rb_node;
  struct symbol *symbol;

  while (n) {
    symbol = rb_entry_symbol(n);

    if (addr < (void *)symbol->address)
      n = n->rb_left;
    else if (addr > (void *)symbol->address)
      n = n->rb_right;
    else
      return symbol;
  }
  return NULL;
}

/**
 * Returns a pointer pointing to the first target whose address does not compare less than @p addr
 */
struct symbol *rb_lower_bound_symbol(struct symbol_table *table, void *addr) {
  struct rb_node *n = table->rb_symbol.rb_node;
  struct rb_node *parent = NULL;
  struct symbol *symbol;

  while (n) {
    symbol = rb_entry_symbol(n);

    if (!((void *)symbol->address < addr)) {
      parent = n;
      n = n->rb_left;
    } else
      n = n->rb_right;
  }
  return parent ? rb_entry_symbol(parent) : NULL;
}

/**
 * Returns an iterator pointing to the first target whose address compares greater than @p addr
 */
struct symbol *rb_upper_bound_symbol(struct symbol_table *table, void *addr) {
  struct rb_node *n = table->rb_symbol.rb_node;
  struct rb_node *parent = NULL;
  struct symbol *symbol;

  while (n) {
    symbol = rb_entry_symbol(n);

    if ((void *)symbol->address > addr) {
      parent = n;
      n = n->rb_left;
    } else
      n = n->rb_right;
  }
  return parent ? rb_entry_symbol(parent) : NULL;
}

struct symbol *__rb_insert_target(struct symbol_table *table, void *addr, struct rb_node *node) {
  struct rb_node **p = &table->rb_symbol.rb_node;
  struct rb_node *parent = NULL;
  struct symbol *symbol;

  while (*p) {
    parent = *p;
    symbol = rb_entry(parent, struct symbol, rb_symbol);

    if (addr < (void *)symbol->address)
      p = &(*p)->rb_left;
    else if (addr > (void *)symbol->address)
      p = &(*p)->rb_right;
    else
      return symbol;
  }

  rb_link_node(node, parent, p);

  return NULL;
}

struct symbol *rb_insert_symbol(struct symbol_table *table, void *addr, struct rb_node *node) {
  struct symbol *ret;
  if ((ret = __rb_insert_target(table, addr, node)))
    goto out;
  rb_insert_color(node, &table->rb_symbol);
out:
  return ret;
}

void symbol_table_init(struct symbol_table *table) {
  table->rb_symbol = RB_ROOT;
  table->symbol_hash = malloc(sizeof(struct hlist_head) * symbolhash_size);
  for (int i = 0; i < symbolhash_size; i++)
    INIT_HLIST_HEAD(&table->symbol_hash[i]);
}

void symbol_table_destroy(struct symbol_table *table) {
  if (!table)
    return;

  free(table->symbol_hash);
}

struct bfd_context {
  asymbol **syms; /** The symbol table */
  long symcount; /** Number of symbols in @p syms */
  asymbol **dynsyms; /** The dynamic symbol table */
  long dynsymcount; /** Number of symbols in @p syms */
  asymbol *synthsyms; /** The synthetic symbol table */
  long synthcount; /** Number of symbols in @p synthsyms */
  struct symbol_table *table; /**  */
};

enum target_type {
  TARGET_STATIC,
  TARGET_DYNAMIC
};

static const int adjust_section_vma = 0;

static int slurp_symtab(bfd *abfd, struct bfd_context *symbols) {
  if (!(bfd_get_file_flags(abfd) & HAS_SYMS)) {
    return 0;
  }

  long storage = bfd_get_symtab_upper_bound(abfd);
  if (storage < 0) {
    return -1;
  }
  symbols->syms = malloc(storage);
  symbols->symcount = bfd_canonicalize_symtab(abfd, symbols->syms);
  if (symbols->symcount < 0) {
    return -1;
  }

  return 0;
}

static int slurp_dynamic_symtab(bfd *abfd, struct bfd_context *symbols) {
  long storage = bfd_get_dynamic_symtab_upper_bound(abfd);
  if (storage < 0) {
    if (!(bfd_get_file_flags(abfd) & DYNAMIC)) {
      symbols->dynsymcount = 0;
      return 0;
    }
  }

  if (storage < 0) {
    return -1;
  }
  symbols->dynsyms = malloc(storage);
  symbols->dynsymcount = bfd_canonicalize_dynamic_symtab(abfd, symbols->dynsyms);
  if (symbols->dynsymcount < 0) {
    return -1;
  }

  return 0;
}

static void dump_symbols(bfd *abfd, struct bfd_context *ctx, struct symbol_table *table, enum target_type type) {
  asymbol **asym, **syms;
  long count;

  switch (type) {
  case TARGET_STATIC:
    asym = ctx->syms;
    count = ctx->symcount;
    break;
  case TARGET_DYNAMIC:
    asym = ctx->dynsyms;
    count = ctx->dynsymcount;
    break;
  }

  syms = malloc((count + ctx->synthcount) * sizeof(asymbol *));
  memcpy(syms, asym, count * sizeof (asymbol *));
  for (int i = 0; i < ctx->synthcount; ++i) {
    syms[count] = ctx->synthsyms + i;
    ++count;
  }
  asym = syms;

  // TODO(petr): currently the allocated memory is not released as we would
  // loose access to symbol->asymbol fields; ideally we should avoid storing
  // asymbol structures and copy necessary data into our own symbol structure

  symbol_info info;
  for (long i = 0; i < count; i++) {
    if (*asym != NULL) {
      bfd *abfd = bfd_asymbol_bfd(*asym);
      if (abfd != NULL && !bfd_is_target_special_symbol(abfd, *asym)) {
        enum symbol_type type = SYMBOL_UNDEFINED;
        if ((*asym)->flags & BSF_LOCAL)
          type |= SYMBOL_LOCAL;
        if ((*asym)->flags & BSF_GLOBAL)
          type |= SYMBOL_GLOBAL;
        if ((*asym)->flags & BSF_FUNCTION)
          type |= SYMBOL_FUNCTION;
        if ((*asym)->flags & BSF_OBJECT)
          type |= SYMBOL_OBJECT;
        if ((*asym)->flags & BSF_DYNAMIC)
          type |= SYMBOL_DYNAMIC;
        if ((*asym)->flags & BSF_WEAK)
          type |= SYMBOL_WEAK;
        if ((*asym)->flags & BSF_DEBUGGING)
          type |= SYMBOL_DEBUGGING;
        if ((*asym)->flags & ((!BSF_KEEP) | (1 << 4) | BSF_DEBUGGING))
          type |= SYMBOL_COMMON;

        if (!type && !bfd_asymbol_value(*asym))
          goto next;

        bfd_symbol_info(*asym, &info);

        struct symbol *symbol = malloc(sizeof(struct symbol));
#ifdef HAVE_DEMANGLE_H
        symbol->name = bfd_demangle(abfd, bfd_asymbol_name(*asym), DMGL_ANSI | DMGL_PARAMS);
#else
        symbol->name = (char *)bfd_asymbol_name(*asym);
#endif
        if (!symbol->name) {
          symbol->name = strdup(info.name);
        } else {
          symbol->name = strdup(symbol->name);
	}
        symbol->address = bfd_asymbol_value(*asym);
        symbol->type = type;
        symbol->asymbol = *asym; // TODO: leaky abstraction

        INIT_HLIST_NODE(&symbol->symbol_hash);
        rb_init_node(&symbol->rb_symbol);

        symbol_add(table, symbol);
        rb_insert_symbol(table, (void *)symbol->address, &symbol->rb_symbol);
      }
    }
next:
    asym++;
  }
}

static void dump_reloc_set(struct symbol_table *table, bfd *abfd, asection *sec, arelent **relpp, long relcount) {
  for (arelent **p = relpp; relcount && *p != NULL; p++, relcount--) {
    arelent *rel = *p;

    if (rel->sym_ptr_ptr && *rel->sym_ptr_ptr) {
      struct symbol *symbol = malloc(sizeof(struct symbol));

      symbol->name = strdup((*(rel->sym_ptr_ptr))->name);
      symbol->address = rel->address;
      symbol->section = (*(rel->sym_ptr_ptr))->section->name;
      symbol->type = SYMBOL_DYNAMIC;

      symbol_add(table, symbol);
      rb_insert_symbol(table, (void *)symbol->address, &symbol->rb_symbol);
    }
  }
}

static void dump_relocs_in_section(bfd *abfd, asection *section, void *obj) {
  if (bfd_is_abs_section(section) || bfd_is_und_section(section)
      || bfd_is_com_section(section) || (section->flags & SEC_RELOC) == 0)
    return;

  struct bfd_context *symbols = (struct bfd_context *)obj;

  long relsize = bfd_get_reloc_upper_bound (abfd, section);
  if (relsize > 0) {
    arelent **relpp = malloc(relsize);
    long relcount = bfd_canonicalize_reloc(abfd, section, relpp, symbols->syms);
    if (relcount > 0)
      dump_reloc_set(symbols->table, abfd, section, relpp, relcount);

    free(relpp);
  }
}

static void dump_relocs(bfd *abfd, struct bfd_context *symbols, enum target_type type) {
  if (type & TARGET_STATIC)
    bfd_map_over_sections(abfd, dump_relocs_in_section, symbols);
  if (type & TARGET_DYNAMIC) {
    long relsize = bfd_get_dynamic_reloc_upper_bound(abfd);
    if (relsize > 0) {
      arelent **relpp = malloc(relsize);

      long relcount = bfd_canonicalize_dynamic_reloc(abfd, relpp, symbols->dynsyms);
      if (relcount > 0)
        dump_reloc_set(symbols->table, abfd, NULL, relpp, relcount);

      free(relpp);
    }
  }
}

static void adjust_addresses(bfd *abfd, asection *section, void *arg) {
  if ((section->flags & SEC_DEBUGGING) == 0) {
    bfd_boolean *has_reloc_p = (bfd_boolean *) arg;
    section->vma += adjust_section_vma;
    if (*has_reloc_p)
      section->lma += adjust_section_vma;
  }
}

static int dump_bfd(bfd *abfd, struct symbol_table *table, enum target_type type) {
  struct bfd_context ctx = {
    .syms = NULL,
    .symcount = 0,
    .dynsyms = NULL,
    .dynsymcount = 0,
    .table = table
  };

  if (adjust_section_vma != 0) {
    bfd_boolean has_reloc = (abfd->flags & HAS_RELOC);
    bfd_map_over_sections(abfd, adjust_addresses, &has_reloc);
  }

  slurp_symtab(abfd, &ctx);
  if (bfd_get_dynamic_symtab_upper_bound(abfd) > 0) {
    slurp_dynamic_symtab(abfd, &ctx);
  }

  ctx.synthcount = bfd_get_synthetic_symtab(abfd, ctx.symcount, ctx.syms, ctx.dynsymcount, ctx.dynsyms, &ctx.synthsyms);
  if (ctx.synthcount < 0) {
    ctx.synthcount = 0;
  }

  dump_symbols(abfd, &ctx, table, type);
  dump_relocs(abfd, &ctx, type);

  if (ctx.syms)
    free(ctx.syms);
  if (ctx.dynsyms)
    free(ctx.dynsyms);
  if (ctx.synthsyms)
    free(ctx.synthsyms);

  return 0;
}

static int display_bfd(bfd *abfd, struct symbol_table *table, enum target_type type) {
  if (bfd_check_format(abfd, bfd_object)) {
    dump_bfd(abfd, table, type);
    return 0;
  }

  if (bfd_get_error() == bfd_error_file_ambiguously_recognized)
    return 1;
  if (bfd_get_error() != bfd_error_file_not_recognized)
    return 1;

  if (bfd_check_format(abfd, bfd_core)) {
    dump_bfd(abfd, table, type);
    return 0;
  }

  if (bfd_get_error() == bfd_error_file_ambiguously_recognized)
    return 1;

  return 0;
}

/**
 * @internal
 * @brief Load the symbol table in bin_file.
 * @param bf The bin_file to load symbols for.
 * @details This takes care of the initial load of symbols and the copying of
 * them into our own structures.
 */
int load_sym_table(struct bin_file *file) {
  if (!file)
    return -1;

  /* Decompress sections unless dumping the section contents.  */
  //if (!dump_section_contents)
  //  file->flags |= BFD_DECOMPRESS;

  /* if the file is an archive, process all of its elements */
  if (bfd_check_format(file->abfd, bfd_archive)) {
    bfd *last_arfile = NULL;
    bfd *arfile      = NULL;

    while(1) {
      bfd_set_error(bfd_error_no_error);

      arfile = bfd_openr_next_archived_file(file->abfd, arfile);
      if (arfile == NULL) {
        if (bfd_get_error() != bfd_error_no_more_archived_files)
          return -1;
        break;
      }
      display_bfd(arfile, &file->sym_table, TARGET_STATIC); // TODO: pass as parameter

      if (last_arfile != NULL)
        bfd_close(last_arfile);
      last_arfile = arfile;
    }

    if (last_arfile != NULL)
      bfd_close(last_arfile);
  } else {
    display_bfd(file->abfd, &file->sym_table, TARGET_STATIC); // TODO: pass as parameter
  }

  return 0;
}

/**
 * @internal
 * @brief Releases memory for all currently discovered symbols.
 * @param bf The bin_file holding the bin_file.sym_table to be purged.
 */
void close_sym_table(struct bin_file * bf)
{
  struct symbol *sym;

  for_each_symbol(sym, &bf->sym_table) {
    hlist_del(&sym->symbol_hash);
    rb_erase(&sym->rb_symbol, &bf->sym_table.rb_symbol);
    free(sym->name);
    free(sym);
  }
}
