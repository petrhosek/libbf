#include "symbol.h"

#include <sys/cdefs.h>
#include <sys/param.h>
#include <err.h>
#include <fnmatch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libkern/hlist.h>
#include <libkern/rbtree.h>

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

    if (addr < symbol->address)
      n = n->rb_left;
    else if (addr > symbol->address)
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

    if (!(symbol->address < addr)) {
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

    if (symbol->address > addr) {
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

    if (addr < symbol->address)
      p = &(*p)->rb_left;
    else if (addr > symbol->address)
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
  symbols->syms = (asymbol **)malloc(storage);
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
  symbols->dynsyms = (asymbol **)malloc(storage);
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

  syms = (asymbol **)malloc((count + ctx->synthcount) * sizeof(asymbol *));
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
  struct symbol symbol;
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
        if ((*asym)->flags & (!BSF_KEEP | (1 << 4) | BSF_DEBUGGING))
          type |= SYMBOL_COMMON;

        if (!type && !bfd_asymbol_value(*asym))
          goto next;

        bfd_symbol_info(*asym, &info);

        struct symbol *symbol = (struct symbol *)malloc(sizeof(struct symbol));
#ifdef HAVE_DEMANGLE_H
        symbol->name = bfd_demangle(abfd, bfd_asymbol_name(*asym), DMGL_ANSI | DMGL_PARAMS);
#else
        symbol->name = (char *)bfd_asymbol_name(*asym);
#endif
        if (!symbol->name) {
          symbol->name = strdup(info.name);
        }
        symbol->address = bfd_asymbol_value(*asym);
        symbol->type = type;
        symbol->asymbol = *asym; // TODO: leaky abstraction

        INIT_HLIST_NODE(&symbol->symbol_hash);
        rb_init_node(&symbol->rb_symbol);

        symbol_add(table, symbol);
        rb_insert_symbol(table, symbol->address, &symbol->rb_symbol);
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
      struct symbol *symbol = (struct symbol *)malloc(sizeof(struct symbol));

      symbol->name = strdup((*(rel->sym_ptr_ptr))->name);
      symbol->address = (void *)rel->address;
      symbol->section = (*(rel->sym_ptr_ptr))->section->name;
      symbol->type = SYMBOL_DYNAMIC;

      symbol_add(table, symbol);
      rb_insert_symbol(table, symbol->address, &symbol->rb_symbol);
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
    arelent **relpp = (arelent **)malloc(relsize);
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
      arelent **relpp = (arelent **)malloc(relsize);

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
 * @brief Load the symbol table in binary_file.
 * @param bf The binary_file to load symbols for.
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
    while(1) {
      bfd_set_error(bfd_error_no_error);

      bfd *arfile = bfd_openr_next_archived_file(file->abfd, arfile);
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
 * @param bf The binary_file holding the binary_file.sym_table to be purged.
 */
void close_sym_table(struct bin_file * bf)
{
  struct htable_entry * cur_entry;
  struct htable_entry * n;
  struct symbol *sym;

  for_each_symbol(sym, &bf->sym_table) {
    hlist_del(&sym->symbol_hash);
    rb_erase(&sym->rb_symbol, &bf->sym_table.rb_symbol);
    free(sym->name);
    free(sym);
  }
}


//////////////////////////////////////////////////////////////////////////////////////////


/* Symbol table buffer structure. */
struct symbuf {
  Elf32_Sym *l32;   /* 32bit local symbol */
  Elf32_Sym *g32;   /* 32bit global symbol */
  Elf64_Sym *l64;   /* 64bit local symbol */
  Elf64_Sym *g64;   /* 64bit global symbol */
  size_t ngs, nls;  /* number of each kind */
  size_t gcap, lcap;  /* buffer capacities. */
};

/* String table buffer structure. */
struct strbuf {
  char *l;    /* local symbol string table */
  char *g;    /* global symbol string table */
  size_t lsz, gsz;  /* size of each kind */
  size_t gcap, lcap;  /* buffer capacities. */
};

static bool is_debug_symbol(unsigned char st_info);
static bool is_global_symbol(unsigned char st_info);
static bool is_local_symbol(unsigned char st_info);
static bool is_needed_symbol(struct elfcopy *ecp, int i, GElf_Sym *s);
static bool is_remove_symbol(struct elfcopy *ecp, size_t sc, int i, GElf_Sym *s, const char *name);
static bool is_weak_symbol(unsigned char st_info);
static int  lookup_exact_string(const char *buf, size_t sz, const char *s);
static int  generate_symbols(struct elfcopy *ecp);
static void mark_symbols(struct elfcopy *ecp, size_t sc);
static int  match_wildcard(const char *name, const char *pattern);

/* Convenient bit vector operation macros. */
#define BIT_SET(v, n) (v[(n)>>3] |= 1U << ((n) & 7))
#define BIT_CLR(v, n) (v[(n)>>3] &= ~(1U << ((n) & 7)))
#define BIT_ISSET(v, n) (v[(n)>>3] & (1U << ((n) & 7)))

static inline bool is_debug_symbol(unsigned char st_info) {
  if (GELF_ST_TYPE(st_info) == STT_SECTION ||
      GELF_ST_TYPE(st_info) == STT_FILE)
    return true;

  return false;
}

static inline bool is_global_symbol(unsigned char st_info) {
  if (GELF_ST_BIND(st_info) == STB_GLOBAL)
    return true;

  return false;
}

static inline bool is_weak_symbol(unsigned char st_info) {
  if (GELF_ST_BIND(st_info) == STB_WEAK)
    return true;

  return false;
}

static inline bool is_local_symbol(unsigned char st_info) {
  if (GELF_ST_BIND(st_info) == STB_LOCAL)
    return true;

  return false;
}

/*
 * Symbols related to relocation are needed.
 */
static inline bool is_needed_symbol(struct elfcopy *ecp, int i, GElf_Sym *s) {
  // If symbol involves relocation, it is needed.
  if (BIT_ISSET(ecp->v_rel, i))
    return true;

  // For relocatable files (.o files), global and weak symbols are needed.
  if (ecp->flags & RELOCATABLE) {
    if (is_global_symbol(s->st_info) || is_weak_symbol(s->st_info))
      return true;
  }

  return false;
}

static inline bool is_remove_symbol(struct elfcopy *ecp, size_t sc, int i, GElf_Sym *s, const char *name) {
  GElf_Sym sym0 = {
    0,    /* st_name */
    0,    /* st_value */
    0,    /* st_size */
    0,    /* st_info */
    0,    /* st_other */
    SHN_UNDEF,  /* st_shndx */
  };

  if (lookup_symop_list(ecp, name, SYMOP_KEEP) != NULL)
    return false;

  if (lookup_symop_list(ecp, name, SYMOP_STRIP) != NULL)
    return true;

  /*
   * Keep the first symbol if it is the special reserved symbol.
   * XXX Should we generate one if it's missing?
   */
  if (i == 0 && !memcmp(s, &sym0, sizeof(GElf_Sym)))
    return false;

  /* Remove the symbol if the section it refers to was removed. */
  if (s->st_shndx != SHN_UNDEF && s->st_shndx < SHN_LORESERVE &&
      ecp->secndx[s->st_shndx] == 0)
    return true;

  if (ecp->strip == STRIP_ALL)
    return true;

  if (ecp->v_rel == NULL)
    mark_symbols(ecp, sc);

  if (is_needed_symbol(ecp, i, s))
    return false;

  if (ecp->strip == STRIP_UNNEEDED)
    return true;

  if ((ecp->flags & DISCARD_LOCAL) && is_local_symbol(s->st_info))
    return true;

  if (ecp->strip == STRIP_DEBUG && is_debug_symbol(s->st_info))
    return true;

  return false;
}

/*
 * Mark symbols refered by relocation entries.
 */
static void mark_symbols(struct elfcopy *ecp, size_t sc) {
  const char  *name;
  Elf_Data  *d;
  Elf_Scn   *s;
  GElf_Rel   r;
  GElf_Rela  ra;
  GElf_Shdr  sh;
  size_t     n, indx;
  int    elferr, i, len;

  ecp->v_rel = calloc((sc + 7) / 8, 1);
  if (ecp->v_rel == NULL)
    err(EXIT_FAILURE, "calloc failed");

  if (elf_getshdrstrndx(ecp->ein, &indx) == -1)
    errx(EXIT_FAILURE, "elf_getshdrstrndx failed: %s",
        elf_errmsg(-1));

  s = NULL;
  while ((s = elf_nextscn(ecp->ein, s)) != NULL) {
    if (gelf_getshdr(s, &sh) != &sh)
      errx(EXIT_FAILURE, "elf_getshdr failed: %s",
          elf_errmsg(-1));

    if (sh.sh_type != SHT_REL && sh.sh_type != SHT_RELA)
      continue;

    /*
     * Skip if this reloc section won't appear in the
     * output object.
     */
    if ((name = elf_strptr(ecp->ein, indx, sh.sh_name)) == NULL)
      errx(EXIT_FAILURE, "elf_strptr failed: %s",
          elf_errmsg(-1));
    if (is_remove_section(ecp, name) ||
        is_remove_reloc_sec(ecp, sh.sh_info))
      continue;

    /* Skip if it's not for .symtab */
    if (sh.sh_link != elf_ndxscn(ecp->symtab->is))
      continue;

    d = NULL;
    n = 0;
    while (n < sh.sh_size && (d = elf_getdata(s, d)) != NULL) {
      len = d->d_size / sh.sh_entsize;
      for (i = 0; i < len; i++) {
        if (sh.sh_type == SHT_REL) {
          if (gelf_getrel(d, i, &r) != &r)
            errx(EXIT_FAILURE,
                "elf_getrel failed: %s",
                 elf_errmsg(-1));
          n = GELF_R_SYM(r.r_info);
        } else {
          if (gelf_getrela(d, i, &ra) != &ra)
            errx(EXIT_FAILURE,
                "elf_getrela failed: %s",
                 elf_errmsg(-1));
          n = GELF_R_SYM(ra.r_info);
        }
        if (n > 0 && n < sc)
          BIT_SET(ecp->v_rel, n);
        else if (n != 0)
          warnx("invalid symbox index");
      }
    }
    elferr = elf_errno();
    if (elferr != 0)
      errx(EXIT_FAILURE, "elf_getdata failed: %s",
          elf_errmsg(elferr));
  }
  elferr = elf_errno();
  if (elferr != 0)
    errx(EXIT_FAILURE, "elf_nextscn failed: %s",
        elf_errmsg(elferr));
}

static int generate_symbols(struct elfcopy *ecp) {
  struct section  *s;
  struct symop  *sp;
  struct symbuf *sy_buf;
  struct strbuf *st_buf;
  const char  *name;
  char    *newname;
  unsigned char *gsym;
  GElf_Shdr  ish;
  GElf_Sym   sym;
  Elf_Data*  id;
  Elf_Scn   *is;
  size_t     ishstrndx, namelen, ndx, nsyms, sc, symndx;
  int    ec, elferr, i;

  if (elf_getshdrstrndx(ecp->ein, &ishstrndx) == -1)
    errx(EXIT_FAILURE, "elf_getshdrstrndx failed: %s",
        elf_errmsg(-1));
  if ((ec = gelf_getclass(ecp->eout)) == ELFCLASSNONE)
    errx(EXIT_FAILURE, "gelf_getclass failed: %s",
        elf_errmsg(-1));

  /* Create buffers for .symtab and .strtab. */
  if ((sy_buf = calloc(1, sizeof(*sy_buf))) == NULL)
    err(EXIT_FAILURE, "calloc failed");
  if ((st_buf = calloc(1, sizeof(*st_buf))) == NULL)
    err(EXIT_FAILURE, "calloc failed");
  sy_buf->gcap = sy_buf->lcap = 64;
  st_buf->gcap = 256;
  st_buf->lcap = 64;
  st_buf->lsz = 1;  /* '\0' at start. */
  st_buf->gsz = 0;
  nsyms = 0;

  ecp->symtab->sz = 0;
  ecp->strtab->sz = 0;
  ecp->symtab->buf = sy_buf;
  ecp->strtab->buf = st_buf;

  /*
   * Create bit vector v_secsym, which is used to mark sections
   * that already have corresponding STT_SECTION symbols.
   */
  ecp->v_secsym = calloc((ecp->nos + 7) / 8, 1);
  if (ecp->v_secsym == NULL)
    err(EXIT_FAILURE, "calloc failed");

  /* Locate .strtab of input object. */
  symndx = 0;
  name = NULL;
  is = NULL;
  while ((is = elf_nextscn(ecp->ein, is)) != NULL) {
    if (gelf_getshdr(is, &ish) != &ish)
      errx(EXIT_FAILURE, "elf_getshdr failed: %s",
          elf_errmsg(-1));
    if ((name = elf_strptr(ecp->ein, ishstrndx, ish.sh_name)) ==
        NULL)
      errx(EXIT_FAILURE, "elf_strptr failed: %s",
          elf_errmsg(-1));
    if (strcmp(name, ".strtab") == 0) {
      symndx = elf_ndxscn(is);
      break;
    }
  }
  elferr = elf_errno();
  if (elferr != 0)
    errx(EXIT_FAILURE, "elf_nextscn failed: %s",
        elf_errmsg(elferr));
  /* FIXME don't panic if can't find .strtab */
  if (symndx == 0)
    errx(EXIT_FAILURE, "can't find .strtab section");

  /* Locate .symtab of input object. */
  is = NULL;
  while ((is = elf_nextscn(ecp->ein, is)) != NULL) {
    if (gelf_getshdr(is, &ish) != &ish)
      errx(EXIT_FAILURE, "elf_getshdr failed: %s",
          elf_errmsg(-1));
    if ((name = elf_strptr(ecp->ein, ishstrndx, ish.sh_name)) ==
        NULL)
      errx(EXIT_FAILURE, "elf_strptr failed: %s",
          elf_errmsg(-1));
    if (strcmp(name, ".symtab") == 0)
      break;
  }
  elferr = elf_errno();
  if (elferr != 0)
    errx(EXIT_FAILURE, "elf_nextscn failed: %s",
        elf_errmsg(elferr));
  if (is == NULL)
    errx(EXIT_FAILURE, "can't find .strtab section");

  /*
   * Create bit vector gsym to mark global symbols, and symndx
   * to keep track of symbol index changes from input object to
   * output object, it is used by update_reloc() later to update
   * relocation information.
   */
  gsym = NULL;
  sc = ish.sh_size / ish.sh_entsize;
  if (sc > 0) {
    ecp->symndx = calloc(sc, sizeof(*ecp->symndx));
    if (ecp->symndx == NULL)
      err(EXIT_FAILURE, "calloc failed");
    gsym = calloc((sc + 7) / 8, sizeof(*gsym));
    if (gsym == NULL)
      err(EXIT_FAILURE, "calloc failed");
    if ((id = elf_getdata(is, NULL)) == NULL) {
      elferr = elf_errno();
      if (elferr != 0)
        errx(EXIT_FAILURE, "elf_getdata failed: %s",
            elf_errmsg(elferr));
      return (0);
    }
  } else
    return (0);

  /* Copy/Filter each symbol. */
  for (i = 0; (size_t)i < sc; i++) {
    if (gelf_getsym(id, i, &sym) != &sym)
      errx(EXIT_FAILURE, "gelf_getsym failed: %s",
          elf_errmsg(-1));
    if ((name = elf_strptr(ecp->ein, symndx, sym.st_name)) == NULL)
      errx(EXIT_FAILURE, "elf_strptr failed: %s",
          elf_errmsg(-1));

    /* Symbol filtering. */
    if (is_remove_symbol(ecp, sc, i, &sym, name) != 0)
      continue;

    /* Check if we need to change the binding of this symbol. */
    if (is_global_symbol(sym.st_info) ||
        is_weak_symbol(sym.st_info)) {
      /*
       * XXX Binutils objcopy does not weaken certain
       * symbols.
       */
      if (ecp->flags & WEAKEN_ALL ||
          lookup_symop_list(ecp, name, SYMOP_WEAKEN) != NULL)
        sym.st_info = GELF_ST_INFO(STB_WEAK,
            GELF_ST_TYPE(sym.st_info));
      /* Do not localize undefined symbols. */
      if (sym.st_shndx != SHN_UNDEF &&
          lookup_symop_list(ecp, name, SYMOP_LOCALIZE) !=
          NULL)
        sym.st_info = GELF_ST_INFO(STB_LOCAL,
            GELF_ST_TYPE(sym.st_info));
      if (ecp->flags & KEEP_GLOBAL &&
          sym.st_shndx != SHN_UNDEF &&
          lookup_symop_list(ecp, name, SYMOP_KEEPG) == NULL)
        sym.st_info = GELF_ST_INFO(STB_LOCAL,
            GELF_ST_TYPE(sym.st_info));
    } else {
      /* STB_LOCAL binding. */
      if (lookup_symop_list(ecp, name, SYMOP_GLOBALIZE) !=
          NULL)
        sym.st_info = GELF_ST_INFO(STB_GLOBAL,
            GELF_ST_TYPE(sym.st_info));
      /* XXX We should globalize weak symbol? */
    }

    /* Check if we need to rename this symbol. */
    if ((sp = lookup_symop_list(ecp, name, SYMOP_REDEF)) != NULL)
      name = sp->newname;

    /* Check if we need to prefix the symbols. */
    newname = NULL;
    if (ecp->prefix_sym != NULL && name != NULL && *name != '\0') {
      namelen = strlen(name) + strlen(ecp->prefix_sym) + 1;
      if ((newname = malloc(namelen)) == NULL)
        err(EXIT_FAILURE, "malloc failed");
      snprintf(newname, namelen, "%s%s", ecp->prefix_sym,
          name);
      name = newname;
    }

    /* Copy symbol, mark global/weak symbol and add to index map. */
    if (is_global_symbol(sym.st_info) ||
        is_weak_symbol(sym.st_info)) {
      BIT_SET(gsym, i);
      ecp->symndx[i] = sy_buf->ngs;
    } else
      ecp->symndx[i] = sy_buf->nls;
    add_to_symtab(ecp, name, sym.st_value, sym.st_size,
        sym.st_shndx, sym.st_info, sym.st_other, 0);

    if (newname != NULL)
      free(newname);

    /*
     * If the symbol is a STT_SECTION symbol, mark the section
     * it points to.
     */
    if (GELF_ST_TYPE(sym.st_info) == STT_SECTION)
      BIT_SET(ecp->v_secsym, ecp->secndx[sym.st_shndx]);
  }

  /*
   * Give up if there is no real symbols inside the table.
   * XXX The logic here needs to be improved. We need to
   * check if that only local symbol is the reserved symbol.
   */
  if (sy_buf->nls <= 1 && sy_buf->ngs == 0)
    return (0);

  /*
   * Create STT_SECTION symbols for sections that do not already
   * got one. However, we do not create STT_SECTION symbol for
   * .symtab, .strtab, .shstrtab and reloc sec of relocatables.
   */
  TAILQ_FOREACH(s, &ecp->v_sec, sec_list) {
    if (s->pseudo)
      continue;
    if (strcmp(s->name, ".symtab") == 0 ||
        strcmp(s->name, ".strtab") == 0 ||
        strcmp(s->name, ".shstrtab") == 0)
      continue;
    if ((ecp->flags & RELOCATABLE) != 0 &&
        ((s->type == SHT_REL) || (s->type == SHT_RELA)))
      continue;

    if ((ndx = elf_ndxscn(s->os)) == SHN_UNDEF)
      errx(EXIT_FAILURE, "elf_ndxscn failed: %s",
          elf_errmsg(-1));

    if (!BIT_ISSET(ecp->v_secsym, ndx)) {
      sym.st_name  = 0;
      sym.st_value = s->vma;
      sym.st_size  = 0;
      sym.st_info  = GELF_ST_INFO(STB_LOCAL, STT_SECTION);
      /*
       * Don't let add_to_symtab() touch sym.st_shndx.
       * In this case, we know the index already.
       */
      add_to_symtab(ecp, NULL, sym.st_value, sym.st_size,
          ndx, sym.st_info, sym.st_other, 1);
    }
  }

  /*
   * Update st_name and index map for global/weak symbols. Note that
   * global/weak symbols are put after local symbols.
   */
  if (gsym != NULL) {
    for(i = 0; (size_t) i < sc; i++) {
      if (!BIT_ISSET(gsym, i))
        continue;

      /* Update st_name. */
      if (ec == ELFCLASS32)
        sy_buf->g32[ecp->symndx[i]].st_name +=
            st_buf->lsz;
      else
        sy_buf->g64[ecp->symndx[i]].st_name +=
            st_buf->lsz;

      /* Update index map. */
      ecp->symndx[i] += sy_buf->nls;
    }
    free(gsym);
  }

  return (1);
}

void create_symtab(struct elfcopy *ecp) {
  struct section  *s, *sy, *st;
  size_t     maxndx, ndx;

  sy = ecp->symtab;
  st = ecp->strtab;

  /*
   * Set section index map for .symtab and .strtab. We need to set
   * these map because otherwise symbols which refer to .symtab and
   * .strtab will be removed by symbol filtering unconditionally.
   * And we have to figure out scn index this way (instead of calling
   * elf_ndxscn) because we can not create Elf_Scn before we're certain
   * that .symtab and .strtab will exist in the output object.
   */
  maxndx = 0;
  TAILQ_FOREACH(s, &ecp->v_sec, sec_list) {
    if (s->os == NULL)
      continue;
    if ((ndx = elf_ndxscn(s->os)) == SHN_UNDEF)
      errx(EXIT_FAILURE, "elf_ndxscn failed: %s",
          elf_errmsg(-1));
    if (ndx > maxndx)
      maxndx = ndx;
  }
  ecp->secndx[elf_ndxscn(sy->is)] = maxndx + 1;
  ecp->secndx[elf_ndxscn(st->is)] = maxndx + 2;

  /*
   * Generate symbols for output object if SYMTAB_INTACT is not set.
   * If there is no symbol in the input object or all the symbols are
   * stripped, then free all the resouces allotted for symbol table,
   * and clear SYMTAB_EXIST flag.
   */
  if (((ecp->flags & SYMTAB_INTACT) == 0) && !generate_symbols(ecp)) {
    TAILQ_REMOVE(&ecp->v_sec, ecp->symtab, sec_list);
    TAILQ_REMOVE(&ecp->v_sec, ecp->strtab, sec_list);
    free(ecp->symtab);
    free(ecp->strtab);
    ecp->symtab = NULL;
    ecp->strtab = NULL;
    ecp->flags &= ~SYMTAB_EXIST;
    return;
  }

  /* Create output Elf_Scn for .symtab and .strtab. */
  if ((sy->os = elf_newscn(ecp->eout)) == NULL ||
      (st->os = elf_newscn(ecp->eout)) == NULL)
    errx(EXIT_FAILURE, "elf_newscn failed: %s",
        elf_errmsg(-1));
  /* Update secndx anyway. */
  ecp->secndx[elf_ndxscn(sy->is)] = elf_ndxscn(sy->os);
  ecp->secndx[elf_ndxscn(st->is)] = elf_ndxscn(st->os);

  /*
   * Copy .symtab and .strtab section headers from input to output
   * object to start with, these will be overridden later if need.
   */
  copy_shdr(ecp, sy, ".symtab", 1, 0);
  copy_shdr(ecp, st, ".strtab", 1, 0);

  /* Copy verbatim if symbol table is intact. */
  if (ecp->flags & SYMTAB_INTACT) {
    copy_data(sy);
    copy_data(st);
    return;
  }

  create_symtab_data(ecp);
}

void free_symtab(struct elfcopy *ecp) {
  struct symbuf *sy_buf;
  struct strbuf *st_buf;

  if (ecp->symtab != NULL && ecp->symtab->buf != NULL) {
    sy_buf = ecp->symtab->buf;
    if (sy_buf->l32 != NULL)
      free(sy_buf->l32);
    if (sy_buf->g32 != NULL)
      free(sy_buf->g32);
    if (sy_buf->l64 != NULL)
      free(sy_buf->l64);
    if (sy_buf->g64 != NULL)
      free(sy_buf->g64);
  }

  if (ecp->strtab != NULL && ecp->strtab->buf != NULL) {
    st_buf = ecp->strtab->buf;
    if (st_buf->l != NULL)
      free(st_buf->l);
    if (st_buf->g != NULL)
      free(st_buf->g);
  }
}

void create_external_symtab(struct elfcopy *ecp) {
  struct section *s;
  struct symbuf *sy_buf;
  struct strbuf *st_buf;
  GElf_Shdr sh;
  size_t ndx;

  if (ecp->oec == ELFCLASS32)
    ecp->symtab = create_external_section(ecp, ".symtab", NULL,
        NULL, 0, 0, SHT_SYMTAB, ELF_T_SYM, 0, 4, 0, 0);
  else
    ecp->symtab = create_external_section(ecp, ".symtab", NULL,
        NULL, 0, 0, SHT_SYMTAB, ELF_T_SYM, 0, 8, 0, 0);

  ecp->strtab = create_external_section(ecp, ".strtab", NULL, NULL, 0, 0,
      SHT_STRTAB, ELF_T_BYTE, 0, 1, 0, 0);

  /* Let sh_link field of .symtab section point to .strtab section. */
  if (gelf_getshdr(ecp->symtab->os, &sh) == NULL)
    errx(EXIT_FAILURE, "gelf_getshdr() failed: %s",
        elf_errmsg(-1));
  sh.sh_link = elf_ndxscn(ecp->strtab->os);
  if (!gelf_update_shdr(ecp->symtab->os, &sh))
    errx(EXIT_FAILURE, "gelf_update_shdr() failed: %s",
        elf_errmsg(-1));

  /* Create buffers for .symtab and .strtab. */
  if ((sy_buf = calloc(1, sizeof(*sy_buf))) == NULL)
    err(EXIT_FAILURE, "calloc failed");
  if ((st_buf = calloc(1, sizeof(*st_buf))) == NULL)
    err(EXIT_FAILURE, "calloc failed");
  sy_buf->gcap = sy_buf->lcap = 64;
  st_buf->gcap = 256;
  st_buf->lcap = 64;
  st_buf->lsz = 1;  /* '\0' at start. */
  st_buf->gsz = 0;

  ecp->symtab->sz = 0;
  ecp->strtab->sz = 0;
  ecp->symtab->buf = sy_buf;
  ecp->strtab->buf = st_buf;

  /* Always create the special symbol at the symtab beginning. */
  add_to_symtab(ecp, NULL, 0, 0, SHN_UNDEF,
      ELF32_ST_INFO(STB_LOCAL, STT_NOTYPE), 0, 1);

  /* Create STT_SECTION symbols. */
  TAILQ_FOREACH(s, &ecp->v_sec, sec_list) {
    if (s->pseudo)
      continue;
    if (strcmp(s->name, ".symtab") == 0 ||
        strcmp(s->name, ".strtab") == 0 ||
        strcmp(s->name, ".shstrtab") == 0)
      continue;
    (void) elf_errno();
    if ((ndx = elf_ndxscn(s->os)) == SHN_UNDEF) {
      warnx("elf_ndxscn failed: %s",
          elf_errmsg(-1));
      continue;
    }
    add_to_symtab(ecp, NULL, 0, 0, ndx,
        GELF_ST_INFO(STB_LOCAL, STT_SECTION), 0, 1);
  }
}

void add_to_symtab(struct elfcopy *ecp, const char *name, uint64_t st_value,
    uint64_t st_size, uint16_t st_shndx, unsigned char st_info,
    unsigned char st_other, int ndx_known) {
  struct symbuf *sy_buf;
  struct strbuf *st_buf;
  int pos;

  /*
   * Convenient macro for copying global/local 32/64 bit symbols
   * from input object to the buffer created for output object.
   * It handles buffer growing, st_name calculating and st_shndx
   * updating for symbols with non-special section index.
   */
#define _ADDSYM(B, SZ) do {           \
  if (sy_buf->B##SZ == NULL) {          \
    sy_buf->B##SZ = malloc(sy_buf->B##cap *     \
        sizeof(Elf##SZ##_Sym));       \
    if (sy_buf->B##SZ == NULL)        \
      err(EXIT_FAILURE, "malloc failed");   \
  } else if (sy_buf->n##B##s >= sy_buf->B##cap) {     \
    sy_buf->B##cap *= 2;          \
    sy_buf->B##SZ = realloc(sy_buf->B##SZ, sy_buf->B##cap * \
        sizeof(Elf##SZ##_Sym));       \
    if (sy_buf->B##SZ == NULL)        \
      err(EXIT_FAILURE, "realloc failed");    \
  }               \
  sy_buf->B##SZ[sy_buf->n##B##s].st_info  = st_info;    \
  sy_buf->B##SZ[sy_buf->n##B##s].st_other = st_other;   \
  sy_buf->B##SZ[sy_buf->n##B##s].st_value = st_value;   \
  sy_buf->B##SZ[sy_buf->n##B##s].st_size  = st_size;    \
  if (ndx_known)              \
    sy_buf->B##SZ[sy_buf->n##B##s].st_shndx = st_shndx; \
  else if (st_shndx == SHN_UNDEF || st_shndx >= SHN_LORESERVE)  \
    sy_buf->B##SZ[sy_buf->n##B##s].st_shndx = st_shndx; \
  else                \
    sy_buf->B##SZ[sy_buf->n##B##s].st_shndx =   \
      ecp->secndx[st_shndx];        \
  if (st_buf->B == NULL) {          \
    st_buf->B = calloc(st_buf->B##cap, sizeof(*st_buf->B)); \
    if (st_buf->B == NULL)          \
      err(EXIT_FAILURE, "malloc failed");   \
  }               \
  if (name != NULL && *name != '\0') {        \
    pos = lookup_exact_string(st_buf->B,      \
        st_buf->B##sz, name);       \
    if (pos != -1)            \
      sy_buf->B##SZ[sy_buf->n##B##s].st_name = pos; \
    else {              \
      sy_buf->B##SZ[sy_buf->n##B##s].st_name =  \
          st_buf->B##sz;        \
      while (st_buf->B##sz + strlen(name) >=    \
          st_buf->B##cap - 1) {     \
        st_buf->B##cap *= 2;      \
        st_buf->B = realloc(st_buf->B,    \
            st_buf->B##cap);      \
        if (st_buf->B == NULL)      \
          err(EXIT_FAILURE,   \
              "realloc failed");    \
      }           \
      strncpy(&st_buf->B[st_buf->B##sz], name,  \
          strlen(name));        \
      st_buf->B[st_buf->B##sz + strlen(name)] = '\0'; \
      st_buf->B##sz += strlen(name) + 1;    \
    }             \
  } else                \
    sy_buf->B##SZ[sy_buf->n##B##s].st_name = 0;   \
  sy_buf->n##B##s++;            \
} while (0)

  sy_buf = ecp->symtab->buf;
  st_buf = ecp->strtab->buf;

  if (ecp->oec == ELFCLASS32) {
    if (is_local_symbol(st_info))
      _ADDSYM(l, 32);
    else
      _ADDSYM(g, 32);
  } else {
    if (is_local_symbol(st_info))
      _ADDSYM(l, 64);
    else
      _ADDSYM(g, 64);
  }

  /* Update section size. */
  ecp->symtab->sz = (sy_buf->nls + sy_buf->ngs) *
      (ecp->oec == ELFCLASS32 ? sizeof(Elf32_Sym) : sizeof(Elf64_Sym));
  ecp->strtab->sz = st_buf->lsz + st_buf->gsz;

#undef  _ADDSYM
}

void finalize_external_symtab(struct elfcopy *ecp) {
  struct symbuf *sy_buf;
  struct strbuf *st_buf;
  int i;

  /*
   * Update st_name for global/weak symbols. (global/weak symbols
   * are put after local symbols)
   */
  sy_buf = ecp->symtab->buf;
  st_buf = ecp->strtab->buf;
  for (i = 0; (size_t) i < sy_buf->ngs; i++) {
    if (ecp->oec == ELFCLASS32)
      sy_buf->g32[i].st_name += st_buf->lsz;
    else
      sy_buf->g64[i].st_name += st_buf->lsz;
  }
}

void create_symtab_data(struct elfcopy *ecp) {
  struct section  *sy, *st;
  struct symbuf *sy_buf;
  struct strbuf *st_buf;
  Elf_Data  *gsydata, *lsydata, *gstdata, *lstdata;
  GElf_Shdr  shy, sht;

  sy = ecp->symtab;
  st = ecp->strtab;

  if (gelf_getshdr(sy->os, &shy) == NULL)
    errx(EXIT_FAILURE, "gelf_getshdr() failed: %s",
        elf_errmsg(-1));
  if (gelf_getshdr(st->os, &sht) == NULL)
    errx(EXIT_FAILURE, "gelf_getshdr() failed: %s",
        elf_errmsg(-1));

  /*
   * Create two Elf_Data for .symtab section of output object, one
   * for local symbols and another for global symbols. Note that
   * local symbols appear first in the .symtab.
   */
  sy_buf = sy->buf;
  if (sy_buf->nls > 0) {
    if ((lsydata = elf_newdata(sy->os)) == NULL)
      errx(EXIT_FAILURE, "elf_newdata() failed: %s.",
           elf_errmsg(-1));
    if (ecp->oec == ELFCLASS32) {
      lsydata->d_align  = 4;
      lsydata->d_off    = 0;
      lsydata->d_buf    = sy_buf->l32;
      lsydata->d_size   = sy_buf->nls *
        sizeof(Elf32_Sym);
      lsydata->d_type   = ELF_T_SYM;
      lsydata->d_version  = EV_CURRENT;
    } else {
      lsydata->d_align  = 8;
      lsydata->d_off    = 0;
      lsydata->d_buf    = sy_buf->l64;
      lsydata->d_size   = sy_buf->nls *
        sizeof(Elf64_Sym);
      lsydata->d_type   = ELF_T_SYM;
      lsydata->d_version  = EV_CURRENT;
    }
  }
  if (sy_buf->ngs > 0) {
    if ((gsydata = elf_newdata(sy->os)) == NULL)
      errx(EXIT_FAILURE, "elf_newdata() failed: %s.",
           elf_errmsg(-1));
    if (ecp->oec == ELFCLASS32) {
      gsydata->d_align  = 4;
      gsydata->d_off    = sy_buf->nls *
        sizeof(Elf32_Sym);
      gsydata->d_buf    = sy_buf->g32;
      gsydata->d_size   = sy_buf->ngs *
        sizeof(Elf32_Sym);
      gsydata->d_type   = ELF_T_SYM;
      gsydata->d_version  = EV_CURRENT;
    } else {
      gsydata->d_align  = 8;
      gsydata->d_off    = sy_buf->nls *
        sizeof(Elf64_Sym);
      gsydata->d_buf    = sy_buf->g64;
      gsydata->d_size   = sy_buf->ngs *
        sizeof(Elf64_Sym);
      gsydata->d_type   = ELF_T_SYM;
      gsydata->d_version  = EV_CURRENT;
    }
  }

  /*
   * Create two Elf_Data for .strtab, one for local symbol name
   * and another for globals. Same as .symtab, local symbol names
   * appear first.
   */
  st_buf = st->buf;
  if ((lstdata = elf_newdata(st->os)) == NULL)
    errx(EXIT_FAILURE, "elf_newdata() failed: %s.",
        elf_errmsg(-1));
  lstdata->d_align  = 1;
  lstdata->d_off    = 0;
  lstdata->d_buf    = st_buf->l;
  lstdata->d_size   = st_buf->lsz;
  lstdata->d_type   = ELF_T_BYTE;
  lstdata->d_version  = EV_CURRENT;

  if (st_buf->gsz > 0) {
    if ((gstdata = elf_newdata(st->os)) == NULL)
      errx(EXIT_FAILURE, "elf_newdata() failed: %s.",
          elf_errmsg(-1));
    gstdata->d_align  = 1;
    gstdata->d_off    = lstdata->d_size;
    gstdata->d_buf    = st_buf->g;
    gstdata->d_size   = st_buf->gsz;
    gstdata->d_type   = ELF_T_BYTE;
    gstdata->d_version  = EV_CURRENT;
  }

  shy.sh_addr   = 0;
  shy.sh_addralign  = (ecp->oec == ELFCLASS32 ? 4 : 8);
  shy.sh_size   = sy->sz;
  shy.sh_type   = SHT_SYMTAB;
  shy.sh_flags    = 0;
  shy.sh_entsize    = gelf_fsize(ecp->eout, ELF_T_SYM, 1,
      EV_CURRENT);
  /*
   * According to SYSV abi, here sh_info is one greater than
   * the symbol table index of the last local symbol(binding
   * STB_LOCAL).
   */
  shy.sh_info   = sy_buf->nls;

  sht.sh_addr   = 0;
  sht.sh_addralign  = 1;
  sht.sh_size   = st->sz;
  sht.sh_type   = SHT_STRTAB;
  sht.sh_flags    = 0;
  sht.sh_entsize    = 0;
  sht.sh_info   = 0;
  sht.sh_link   = 0;

  if (!gelf_update_shdr(sy->os, &shy))
    errx(EXIT_FAILURE, "gelf_update_shdr() failed: %s",
        elf_errmsg(-1));
  if (!gelf_update_shdr(st->os, &sht))
    errx(EXIT_FAILURE, "gelf_update_shdr() failed: %s",
        elf_errmsg(-1));
}

void add_to_symop_list(struct elfcopy *ecp, const char *name, const char *newname,
    unsigned int op) {
  struct symop *s;

  if ((s = lookup_symop_list(ecp, name, ~0U)) == NULL) {
    if ((s = calloc(1, sizeof(*s))) == NULL)
      errx(EXIT_FAILURE, "not enough memory");
    s->name = name;
    if (op == SYMOP_REDEF)
      s->newname = newname;
  }

  s->op |= op;
  STAILQ_INSERT_TAIL(&ecp->v_symop, s, symop_list);
}

static int match_wildcard(const char *name, const char *pattern) {
  int reverse, match;

  reverse = 0;
  if (*pattern == '!') {
    reverse = 1;
    pattern++;
  }

  match = 0;
  if (!fnmatch(pattern, name, 0)) {
    match = 1;
    printf("string '%s' match to pattern '%s'\n", name, pattern);
  }

  return (reverse ? !match : match);
}

struct symop *lookup_symop_list(struct elfcopy *ecp, const char *name, unsigned int op) {
  struct symop *s;

  STAILQ_FOREACH(s, &ecp->v_symop, symop_list) {
    if (name == NULL || !strcmp(name, s->name) ||
        ((ecp->flags & WILDCARD) && match_wildcard(name, s->name)))
      if ((s->op & op) != 0)
        return (s);
  }

  return (NULL);
}

static int lookup_exact_string(const char *buf, size_t sz, const char *s) {
  size_t slen = strlen(s);
  for (const char *b = buf; b < buf + sz; b += strlen(b) + 1) {
    if (strlen(b) != slen)
      continue;
    if (!strcmp(b, s))
      return b - buf;
  }

  return -1;
}

/////////////////////////////////////////////////////////////////////////

void byte_put(unsigned char * field, elf_vma value, int size) {
  switch (size) {
  case 8:
    field[7] = (((value >> 24) >> 24) >> 8) & 0xff;
    field[6] = ((value >> 24) >> 24) & 0xff;
    field[5] = ((value >> 24) >> 16) & 0xff;
    field[4] = ((value >> 24) >> 8) & 0xff;
    /* Fall through.  */
  case 4:
    field[3] = (value >> 24) & 0xff;
    /* Fall through.  */
  case 3:
    field[2] = (value >> 16) & 0xff;
    /* Fall through.  */
  case 2:
    field[1] = (value >> 8) & 0xff;
    /* Fall through.  */
  case 1:
    field[0] = value & 0xff;
    break;

  default:
    error(_("Unhandled data length: %d\n"), size);
    abort();
  }
}

elf_vma byte_get(unsigned char *field, int size) {
  switch (size) {
  case 1:
    return *field;

  case 2:
    return ((unsigned int) (field[0]))
        | (((unsigned int) (field[1])) << 8);

  case 3:
    return ((unsigned long) (field[0]))
        | (((unsigned long) (field[1])) << 8)
        | (((unsigned long) (field[2])) << 16);

  case 4:
    return ((unsigned long) (field[0]))
        | (((unsigned long) (field[1])) << 8)
        | (((unsigned long) (field[2])) << 16)
        | (((unsigned long) (field[3])) << 24);

  case 8:
    if (sizeof(elf_vma) == 8)
      return ((elf_vma)(field[0]))
          | (((elf_vma)(field[1])) << 8)
          | (((elf_vma)(field[2])) << 16)
          | (((elf_vma)(field[3])) << 24)
          | (((elf_vma)(field[4])) << 32)
          | (((elf_vma)(field[5])) << 40)
          | (((elf_vma)(field[6])) << 48)
          | (((elf_vma)(field[7])) << 56);
    else if (sizeof(elf_vma) == 4)
      /* We want to extract data from an 8 byte wide field and
       place it into a 4 byte wide field.  Since this is a little
       endian source we can just use the 4 byte extraction code.  */
      return ((unsigned long) (field[0]))
          | (((unsigned long) (field[1])) << 8)
          | (((unsigned long) (field[2])) << 16)
          | (((unsigned long) (field[3])) << 24);

  default:
    error(_("Unhandled data length: %d\n"), size);
    abort();
  }
}

elf_vma byte_get_signed(unsigned char *field, int size) {
  elf_vma x = byte_get(field, size);

  switch (size) {
  case 1:
    return (x ^ 0x80) - 0x80;
  case 2:
    return (x ^ 0x8000) - 0x8000;
  case 4:
    return (x ^ 0x80000000) - 0x80000000;
  case 8:
    return x;
  default:
    abort();
  }
}

#define BYTE_PUT(field, val) byte_put(field, val, sizeof (field))
#define BYTE_GET(field) byte_get(field, sizeof (field))
#define BYTE_GET_SIGNED(field) byte_get_signed(field, sizeof (field))

static void *get_data(void * var, FILE * file, long offset, size_t size, size_t nmemb, const char * reason) {
  void * mvar;

  if (size == 0 || nmemb == 0)
    return NULL;

  if (fseek(file, archive_file_offset + offset, SEEK_SET)) {
    error(_("Unable to seek to 0x%lx for %s\n"), (unsigned long)archive_file_offset + offset, reason);
    return NULL;
  }

  mvar = var;
  if (mvar == NULL) {
    /* Check for overflow.  */
    if (nmemb < (~(size_t) 0 - 1) / size)
      /* + 1 so that we can '\0' terminate invalid string table sections.  */
      mvar = malloc(size * nmemb + 1);

    if (mvar == NULL) {
      error(_("Out of memory allocating 0x%lx bytes for %s\n"), (unsigned long) (size * nmemb), reason);
      return NULL;
    }

    ((char *) mvar)[size * nmemb] = '\0';
  }

  if (fread(mvar, size, nmemb, file) != nmemb) {
    error(_("Unable to read in 0x%lx bytes of %s\n"),
        (unsigned long) (size * nmemb), reason);
    if (mvar != var)
      free(mvar);
    return NULL;
  }

  return mvar;
}

/* Return a pointer to section NAME, or NULL if no such section exists.  */
static Elf_Internal_Shdr *find_section(const char * name) {
  unsigned int i;

  for (i = 0; i < elf_header.e_shnum; i++)
    if (streq (SECTION_NAME (section_headers + i), name))
      return section_headers + i;

  return NULL;
}

/* Return a pointer to a section containing ADDR, or NULL if no such
   section exists.  */
static Elf_Internal_Shdr *find_section_by_address(bfd_vma addr) {
  unsigned int i;

  for (i = 0; i < elf_header.e_shnum; i++) {
    Elf_Internal_Shdr *sec = section_headers + i;
    if (addr >= sec->sh_addr && addr < sec->sh_addr + sec->sh_size)
      return sec;
  }

  return NULL;
}

static Elf_Internal_Sym *get_32bit_elf_symbols(FILE * file, Elf_Internal_Shdr * section) {
  unsigned long number;
  Elf32_External_Sym * esyms = NULL;
  Elf_External_Sym_Shndx * shndx;
  Elf_Internal_Sym * isyms = NULL;
  Elf_Internal_Sym * psym;
  unsigned int j;

  /* Run some sanity checks first.  */
  if (section->sh_entsize == 0) {
    error(_("sh_entsize is zero\n"));
    return NULL;
  }

  number = section->sh_size / section->sh_entsize;

  if (number * sizeof(Elf32_External_Sym) > section->sh_size + 1) {
    error(_("Invalid sh_entsize\n"));
    return NULL;
  }

  esyms = (Elf32_External_Sym *) get_data(NULL, file, section->sh_offset, 1,
      section->sh_size, _("symbols"));
  if (esyms == NULL)
    return NULL;

  shndx = NULL;
  if (symtab_shndx_hdr != NULL && (symtab_shndx_hdr->sh_link == (unsigned long) (section - section_headers))) {
    shndx = (Elf_External_Sym_Shndx *)get_data(NULL, file, symtab_shndx_hdr->sh_offset, 1, symtab_shndx_hdr->sh_size, _("symtab shndx"));
    if (shndx == NULL)
      goto exit_point;
  }

  isyms = (Elf_Internal_Sym *)cmalloc(number, sizeof(Elf_Internal_Sym));

  if (isyms == NULL) {
    error(_("Out of memory\n"));
    goto exit_point;
  }

  for (j = 0, psym = isyms; j < number; j++, psym++) {
    psym->st_name = BYTE_GET(esyms[j].st_name);
    psym->st_value = BYTE_GET(esyms[j].st_value);
    psym->st_size = BYTE_GET(esyms[j].st_size);
    psym->st_shndx = BYTE_GET(esyms[j].st_shndx);
    if (psym->st_shndx == (SHN_XINDEX & 0xffff) && shndx != NULL)
      psym->st_shndx = byte_get((unsigned char *) &shndx[j], sizeof(shndx[j]));
    else if (psym->st_shndx >= (SHN_LORESERVE & 0xffff))
      psym->st_shndx += SHN_LORESERVE - (SHN_LORESERVE & 0xffff);
    psym->st_info = BYTE_GET(esyms[j].st_info);
    psym->st_other = BYTE_GET(esyms[j].st_other);
  }

exit_point:
  if (shndx)
    free(shndx);
  if (esyms)
    free(esyms);

  return isyms;
}

static Elf_Internal_Sym *get_64bit_elf_symbols(FILE * file, Elf_Internal_Shdr * section) {
  unsigned long number;
  Elf64_External_Sym * esyms;
  Elf_External_Sym_Shndx * shndx;
  Elf_Internal_Sym * isyms;
  Elf_Internal_Sym * psym;
  unsigned int j;

  /* Run some sanity checks first.  */
  if (section->sh_entsize == 0) {
    error(_("sh_entsize is zero\n"));
    return NULL;
  }

  number = section->sh_size / section->sh_entsize;

  if (number * sizeof(Elf64_External_Sym) > section->sh_size + 1) {
    error(_("Invalid sh_entsize\n"));
    return NULL;
  }

  esyms = (Elf64_External_Sym *) get_data(NULL, file, section->sh_offset, 1,
      section->sh_size, _("symbols"));
  if (!esyms)
    return NULL;

  shndx = NULL;
  if (symtab_shndx_hdr != NULL && (symtab_shndx_hdr->sh_link == (unsigned long) (section - section_headers))) {
    shndx = (Elf_External_Sym_Shndx *) get_data(NULL, file, symtab_shndx_hdr->sh_offset, 1, symtab_shndx_hdr->sh_size, _("symtab shndx"));
    if (!shndx) {
      free(esyms);
      return NULL;
    }
  }

  isyms = (Elf_Internal_Sym *)cmalloc(number, sizeof(Elf_Internal_Sym));

  if (isyms == NULL) {
    error(_("Out of memory\n"));
    if (shndx)
      free(shndx);
    free(esyms);
    return NULL;
  }

  for (j = 0, psym = isyms; j < number; j++, psym++) {
    psym->st_name = BYTE_GET(esyms[j].st_name);
    psym->st_info = BYTE_GET(esyms[j].st_info);
    psym->st_other = BYTE_GET(esyms[j].st_other);
    psym->st_shndx = BYTE_GET(esyms[j].st_shndx);
    if (psym->st_shndx == (SHN_XINDEX & 0xffff) && shndx != NULL)
      psym->st_shndx = byte_get((unsigned char *) &shndx[j], sizeof(shndx[j]));
    else if (psym->st_shndx >= (SHN_LORESERVE & 0xffff))
      psym->st_shndx += SHN_LORESERVE - (SHN_LORESERVE & 0xffff);
    psym->st_value = BYTE_GET(esyms[j].st_value);
    psym->st_size = BYTE_GET(esyms[j].st_size);
  }

  if (shndx)
    free(shndx);
  free(esyms);

  return isyms;
}

//static int process_section_headers(FILE * file) {
//  Elf_Internal_Shdr * section;
//  unsigned int i;
//
//  section_headers = NULL;
//
//  if (elf_header.e_shnum == 0) {
//    /* PR binutils/12467.  */
//    if (elf_header.e_shoff != 0)
//      warn(_("possibly corrupt ELF file header - it has a non-zero"
//          " section header offset, but no section headers\n"));
//    else if (do_sections)
//      printf(_("\nThere are no sections in this file.\n"));
//
//    return 1;
//  }
//
//  if (do_sections && !do_header)
//    printf(_("There are %d section headers, starting at offset 0x%lx:\n"),
//        elf_header.e_shnum, (unsigned long) elf_header.e_shoff);
//
//  if (is_32bit_elf) {
//    if (!get_32bit_section_headers(file, elf_header.e_shnum))
//      return 0;
//  } else if (!get_64bit_section_headers(file, elf_header.e_shnum))
//    return 0;
//
//  /* Read in the string table, so that we have names to display.  */
//  if (elf_header.e_shstrndx != SHN_UNDEF
//      && elf_header.e_shstrndx < elf_header.e_shnum) {
//    section = section_headers + elf_header.e_shstrndx;
//
//    if (section->sh_size != 0) {
//      string_table = (char *) get_data(NULL, file, section->sh_offset, 1,
//          section->sh_size, _("string table"));
//
//      string_table_length = string_table != NULL ? section->sh_size : 0;
//    }
//  }
//
//  /* Scan the sections for the dynamic symbol table
//   and dynamic string table and debug sections.  */
//  dynamic_symbols = NULL;
//  dynamic_strings = NULL;
//  dynamic_syminfo = NULL;
//  symtab_shndx_hdr = NULL;
//
//  eh_addr_size = is_32bit_elf ? 4 : 8;
//  switch (elf_header.e_machine) {
//  case EM_MIPS:
//  case EM_MIPS_RS3_LE:
//    /* The 64-bit MIPS EABI uses a combination of 32-bit ELF and 64-bit
//     FDE addresses.  However, the ABI also has a semi-official ILP32
//     variant for which the normal FDE address size rules apply.
//
//     GCC 4.0 marks EABI64 objects with a dummy .gcc_compiled_longXX
//     section, where XX is the size of longs in bits.  Unfortunately,
//     earlier compilers provided no way of distinguishing ILP32 objects
//     from LP64 objects, so if there's any doubt, we should assume that
//     the official LP64 form is being used.  */
//    if ((elf_header.e_flags & EF_MIPS_ABI)
//        == E_MIPS_ABI_EABI64 && find_section (".gcc_compiled_long32") == NULL)
//      eh_addr_size = 8;
//    break;
//
//  case EM_H8_300:
//  case EM_H8_300H:
//    switch (elf_header.e_flags & EF_H8_MACH) {
//    case E_H8_MACH_H8300:
//    case E_H8_MACH_H8300HN:
//    case E_H8_MACH_H8300SN:
//    case E_H8_MACH_H8300SXN:
//      eh_addr_size = 2;
//      break;
//    case E_H8_MACH_H8300H:
//    case E_H8_MACH_H8300S:
//    case E_H8_MACH_H8300SX:
//      eh_addr_size = 4;
//      break;
//    }
//    break;
//
//  case EM_M32C_OLD:
//  case EM_M32C:
//    switch (elf_header.e_flags & EF_M32C_CPU_MASK) {
//    case EF_M32C_CPU_M16C:
//      eh_addr_size = 2;
//      break;
//    }
//    break;
//  }
//
//#define CHECK_ENTSIZE_VALUES(section, i, size32, size64) \
//  do                      \
//    {                     \
//      size_t expected_entsize               \
//  = is_32bit_elf ? size32 : size64;           \
//      if (section->sh_entsize != expected_entsize)          \
//  error (_("Section %d has invalid sh_entsize %lx (expected %lx)\n"), \
//         i, (unsigned long int) section->sh_entsize,        \
//         (unsigned long int) expected_entsize);         \
//      section->sh_entsize = expected_entsize;           \
//    }                     \
//  while (0)
//#define CHECK_ENTSIZE(section, i, type) \
//  CHECK_ENTSIZE_VALUES (section, i, sizeof (Elf32_External_##type),     \
//      sizeof (Elf64_External_##type))
//
//  for (i = 0, section = section_headers; i < elf_header.e_shnum;
//      i++, section++) {
//    char * name = SECTION_NAME(section);
//
//    if (section->sh_type == SHT_DYNSYM) {
//      if (dynamic_symbols != NULL) {
//        error(_("File contains multiple dynamic symbol tables\n"));
//        continue;
//      }
//
//      CHECK_ENTSIZE(section, i, Sym);
//      num_dynamic_syms = section->sh_size / section->sh_entsize;
//      dynamic_symbols = GET_ELF_SYMBOLS(file, section);
//    } else if (section->sh_type == SHT_STRTAB && streq(name, ".dynstr")) {
//      if (dynamic_strings != NULL) {
//        error(_("File contains multiple dynamic string tables\n"));
//        continue;
//      }
//
//      dynamic_strings = (char *) get_data(NULL, file, section->sh_offset, 1,
//          section->sh_size, _("dynamic strings"));
//      dynamic_strings_length = section->sh_size;
//    } else if (section->sh_type == SHT_SYMTAB_SHNDX) {
//      if (symtab_shndx_hdr != NULL) {
//        error(_("File contains multiple symtab shndx tables\n"));
//        continue;
//      }
//      symtab_shndx_hdr = section;
//    } else if (section->sh_type == SHT_SYMTAB)
//      CHECK_ENTSIZE(section, i, Sym);
//    else if (section->sh_type == SHT_GROUP)
//      CHECK_ENTSIZE_VALUES(section, i, GRP_ENTRY_SIZE, GRP_ENTRY_SIZE);
//    else if (section->sh_type == SHT_REL)
//      CHECK_ENTSIZE(section, i, Rel);
//    else if (section->sh_type == SHT_RELA)
//      CHECK_ENTSIZE(section, i, Rela);
//    else if ((do_debugging || do_debug_info || do_debug_abbrevs
//        || do_debug_lines || do_debug_pubnames || do_debug_pubtypes
//        || do_debug_aranges || do_debug_frames || do_debug_macinfo
//        || do_debug_str || do_debug_loc || do_debug_ranges)
//        && (const_strneq(name, ".debug_") || const_strneq(name, ".zdebug_"))) {
//      if (name[1] == 'z')
//        name += sizeof(".zdebug_") - 1;
//      else
//        name += sizeof(".debug_") - 1;
//
//      if (do_debugging || (do_debug_info && streq(name, "info"))
//          || (do_debug_info && streq(name, "types"))
//          || (do_debug_abbrevs && streq(name, "abbrev"))
//          || (do_debug_lines && streq(name, "line"))
//          || (do_debug_pubnames && streq(name, "pubnames"))
//          || (do_debug_pubtypes && streq(name, "pubtypes"))
//          || (do_debug_aranges && streq(name, "aranges"))
//          || (do_debug_ranges && streq(name, "ranges"))
//          || (do_debug_frames && streq(name, "frame"))
//          || (do_debug_macinfo && streq(name, "macinfo"))
//          || (do_debug_str && streq(name, "str"))
//          || (do_debug_loc && streq(name, "loc")))
//        request_dump_bynumber(i, DEBUG_DUMP);
//    }
//    /* Linkonce section to be combined with .debug_info at link time.  */
//    else if ((do_debugging || do_debug_info)
//        && const_strneq(name, ".gnu.linkonce.wi."))
//      request_dump_bynumber(i, DEBUG_DUMP);
//    else if (do_debug_frames && streq(name, ".eh_frame"))
//      request_dump_bynumber(i, DEBUG_DUMP);
//    else if (do_gdb_index && streq(name, ".gdb_index"))
//      request_dump_bynumber(i, DEBUG_DUMP);
//    /* Trace sections for Itanium VMS.  */
//    else if ((do_debugging || do_trace_info || do_trace_abbrevs
//        || do_trace_aranges) && const_strneq(name, ".trace_")) {
//      name += sizeof(".trace_") - 1;
//
//      if (do_debugging || (do_trace_info && streq(name, "info"))
//          || (do_trace_abbrevs && streq(name, "abbrev"))
//          || (do_trace_aranges && streq(name, "aranges")))
//        request_dump_bynumber(i, DEBUG_DUMP);
//    }
//
//  }
//
//  if (!do_sections)
//    return 1;
//
//  for (i = 0, section = section_headers; i < elf_header.e_shnum;
//      i++, section++) {
//    if (do_section_details) {
//      printf("  [%2u] %s\n", i, SECTION_NAME(section));
//      if (is_32bit_elf || do_wide)
//        printf("       %-15.15s ", get_section_type_name(section->sh_type));
//    } else
//      printf((do_wide ? "  [%2u] %-17s %-15s " : "  [%2u] %-17.17s %-15.15s "),
//          i, SECTION_NAME(section), get_section_type_name(section->sh_type));
//
//    if (is_32bit_elf) {
//      const char * link_too_big = NULL;
//
//      print_vma(section->sh_addr, LONG_HEX);
//
//      printf(" %6.6lx %6.6lx %2.2lx", (unsigned long) section->sh_offset,
//          (unsigned long) section->sh_size,
//          (unsigned long) section->sh_entsize);
//
//      if (do_section_details)
//        fputs("  ", stdout);
//      else
//        printf(" %3s ", get_elf_section_flags(section->sh_flags));
//
//      if (section->sh_link >= elf_header.e_shnum) {
//        link_too_big = "";
//        /* The sh_link value is out of range.  Normally this indicates
//         an error but it can have special values in Solaris binaries.  */
//        switch (elf_header.e_machine) {
//        case EM_386:
//        case EM_486:
//        case EM_X86_64:
//          if (section->sh_link == (SHN_BEFORE & 0xffff))
//            link_too_big = "BEFORE";
//          else if (section->sh_link == (SHN_AFTER & 0xffff))
//            link_too_big = "AFTER";
//          break;
//        default:
//          break;
//        }
//      }
//
//      if (do_section_details) {
//        if (link_too_big != NULL && *link_too_big)
//          printf("<%s> ", link_too_big);
//        else
//          printf("%2u ", section->sh_link);
//        printf("%3u %2lu\n", section->sh_info,
//            (unsigned long) section->sh_addralign);
//      } else
//        printf("%2u %3u %2lu\n", section->sh_link, section->sh_info,
//            (unsigned long) section->sh_addralign);
//
//      if (link_too_big && !*link_too_big)
//        warn(
//            _(
//                "section %u: sh_link value of %u is larger than the number of sections\n"),
//            i, section->sh_link);
//    } else if (do_wide) {
//      print_vma(section->sh_addr, LONG_HEX);
//
//      if ((long) section->sh_offset == section->sh_offset)
//        printf(" %6.6lx", (unsigned long) section->sh_offset);
//      else {
//        putchar(' ');
//        print_vma(section->sh_offset, LONG_HEX);
//      }
//
//      if ((unsigned long) section->sh_size == section->sh_size)
//        printf(" %6.6lx", (unsigned long) section->sh_size);
//      else {
//        putchar(' ');
//        print_vma(section->sh_size, LONG_HEX);
//      }
//
//      if ((unsigned long) section->sh_entsize == section->sh_entsize)
//        printf(" %2.2lx", (unsigned long) section->sh_entsize);
//      else {
//        putchar(' ');
//        print_vma(section->sh_entsize, LONG_HEX);
//      }
//
//      if (do_section_details)
//        fputs("  ", stdout);
//      else
//        printf(" %3s ", get_elf_section_flags(section->sh_flags));
//
//      printf("%2u %3u ", section->sh_link, section->sh_info);
//
//      if ((unsigned long) section->sh_addralign == section->sh_addralign)
//        printf("%2lu\n", (unsigned long) section->sh_addralign);
//      else {
//        print_vma(section->sh_addralign, DEC);
//        putchar('\n');
//      }
//    } else if (do_section_details) {
//      printf("       %-15.15s  ", get_section_type_name(section->sh_type));
//      print_vma(section->sh_addr, LONG_HEX);
//      if ((long) section->sh_offset == section->sh_offset)
//        printf("  %16.16lx", (unsigned long) section->sh_offset);
//      else {
//        printf("  ");
//        print_vma(section->sh_offset, LONG_HEX);
//      }
//      printf("  %u\n       ", section->sh_link);
//      print_vma(section->sh_size, LONG_HEX);
//      putchar(' ');
//      print_vma(section->sh_entsize, LONG_HEX);
//
//      printf("  %-16u  %lu\n", section->sh_info,
//          (unsigned long) section->sh_addralign);
//    } else {
//      putchar(' ');
//      print_vma(section->sh_addr, LONG_HEX);
//      if ((long) section->sh_offset == section->sh_offset)
//        printf("  %8.8lx", (unsigned long) section->sh_offset);
//      else {
//        printf("  ");
//        print_vma(section->sh_offset, LONG_HEX);
//      }
//      printf("\n       ");
//      print_vma(section->sh_size, LONG_HEX);
//      printf("  ");
//      print_vma(section->sh_entsize, LONG_HEX);
//
//      printf(" %3s ", get_elf_section_flags(section->sh_flags));
//
//      printf("     %2u   %3u     %lu\n", section->sh_link, section->sh_info,
//          (unsigned long) section->sh_addralign);
//    }
//
//    return 1;
//  }
//
//  static int process_dynamic_section(FILE * file) {
//    Elf_Internal_Dyn * entry;
//
//    if (dynamic_size == 0) {
//      if (do_dynamic)
//        printf(_("\nThere is no dynamic section in this file.\n"));
//
//      return 1;
//    }
//
//    if (is_32bit_elf) {
//      if (!get_32bit_dynamic_section(file))
//        return 0;
//    } else if (!get_64bit_dynamic_section(file))
//      return 0;
//
//    /* Find the appropriate symbol table.  */
//    if (dynamic_symbols == NULL) {
//      for (entry = dynamic_section; entry < dynamic_section + dynamic_nent;
//          ++entry) {
//        Elf_Internal_Shdr section;
//
//        if (entry->d_tag != DT_SYMTAB)
//          continue;
//
//        dynamic_info[DT_SYMTAB] = entry->d_un.d_val;
//
//        /* Since we do not know how big the symbol table is,
//         we default to reading in the entire file (!) and
//         processing that.  This is overkill, I know, but it
//         should work.  */
//        section.sh_offset = offset_from_vma(file, entry->d_un.d_val, 0);
//
//        if (archive_file_offset != 0)
//          section.sh_size = archive_file_size - section.sh_offset;
//        else {
//          if (fseek(file, 0, SEEK_END))
//            error(_("Unable to seek to end of file!\n"));
//
//          section.sh_size = ftell(file) - section.sh_offset;
//        }
//
//        if (is_32bit_elf)
//          section.sh_entsize = sizeof(Elf32_External_Sym);
//        else
//          section.sh_entsize = sizeof(Elf64_External_Sym);
//
//        num_dynamic_syms = section.sh_size / section.sh_entsize;
//        if (num_dynamic_syms < 1) {
//          error(_("Unable to determine the number of symbols to load\n"));
//          continue;
//        }
//
//        dynamic_symbols = GET_ELF_SYMBOLS(file, &section);
//      }
//    }
//
//    /* Similarly find a string table.  */
//    if (dynamic_strings == NULL) {
//      for (entry = dynamic_section; entry < dynamic_section + dynamic_nent;
//          ++entry) {
//        unsigned long offset;
//        long str_tab_len;
//
//        if (entry->d_tag != DT_STRTAB)
//          continue;
//
//        dynamic_info[DT_STRTAB] = entry->d_un.d_val;
//
//        /* Since we do not know how big the string table is,
//         we default to reading in the entire file (!) and
//         processing that.  This is overkill, I know, but it
//         should work.  */
//
//        offset = offset_from_vma(file, entry->d_un.d_val, 0);
//
//        if (archive_file_offset != 0)
//          str_tab_len = archive_file_size - offset;
//        else {
//          if (fseek(file, 0, SEEK_END))
//            error(_("Unable to seek to end of file\n"));
//          str_tab_len = ftell(file) - offset;
//        }
//
//        if (str_tab_len < 1) {
//          error(
//              _(
//                  "Unable to determine the length of the dynamic string table\n"));
//          continue;
//        }
//
//        dynamic_strings = (char *) get_data(NULL, file, offset, 1, str_tab_len,
//            _("dynamic string table"));
//        dynamic_strings_length = str_tab_len;
//        break;
//      }
//    }
//
//    /* And find the syminfo section if available.  */
//    if (dynamic_syminfo == NULL) {
//      unsigned long syminsz = 0;
//
//      for (entry = dynamic_section; entry < dynamic_section + dynamic_nent;
//          ++entry) {
//        if (entry->d_tag == DT_SYMINENT) {
//          /* Note: these braces are necessary to avoid a syntax
//           error from the SunOS4 C compiler.  */
//          assert(sizeof (Elf_External_Syminfo) == entry->d_un.d_val);
//        } else if (entry->d_tag == DT_SYMINSZ)
//          syminsz = entry->d_un.d_val;
//        else if (entry->d_tag == DT_SYMINFO)
//          dynamic_syminfo_offset = offset_from_vma(file, entry->d_un.d_val,
//              syminsz);
//      }
//
//      if (dynamic_syminfo_offset != 0 && syminsz != 0) {
//        Elf_External_Syminfo * extsyminfo;
//        Elf_External_Syminfo * extsym;
//        Elf_Internal_Syminfo * syminfo;
//
//        /* There is a syminfo section.  Read the data.  */
//        extsyminfo = (Elf_External_Syminfo *) get_data(NULL, file,
//            dynamic_syminfo_offset, 1, syminsz, _("symbol information"));
//        if (!extsyminfo)
//          return 0;
//
//        dynamic_syminfo = (Elf_Internal_Syminfo *) malloc(syminsz);
//        if (dynamic_syminfo == NULL) {
//          error(_("Out of memory\n"));
//          return 0;
//        }
//
//        dynamic_syminfo_nent = syminsz / sizeof(Elf_External_Syminfo);
//        for (syminfo = dynamic_syminfo, extsym = extsyminfo;
//            syminfo < dynamic_syminfo + dynamic_syminfo_nent;
//            ++syminfo, ++extsym) {
//          syminfo->si_boundto = BYTE_GET(extsym->si_boundto);
//          syminfo->si_flags = BYTE_GET(extsym->si_flags);
//        }
//
//        free(extsyminfo);
//      }
//    }
//
//    if (do_dynamic && dynamic_addr)
//      printf(_("\nDynamic section at offset 0x%lx contains %u entries:\n"),
//          dynamic_addr, dynamic_nent);
//    if (do_dynamic)
//      printf(_("  Tag        Type                         Name/Value\n"));
//
//    for (entry = dynamic_section; entry < dynamic_section + dynamic_nent;
//        entry++) {
//      if (do_dynamic) {
//        const char * dtype;
//
//        putchar(' ');
//        print_vma(entry->d_tag, FULL_HEX);
//        dtype = get_dynamic_type(entry->d_tag);
//        printf(" (%s)%*s", dtype,
//            ((is_32bit_elf ? 27 : 19) - (int) strlen(dtype)), " ");
//      }
//
//      switch (entry->d_tag) {
//      case DT_FLAGS:
//        if (do_dynamic)
//          print_dynamic_flags(entry->d_un.d_val);
//        break;
//
//      case DT_AUXILIARY:
//      case DT_FILTER:
//      case DT_CONFIG:
//      case DT_DEPAUDIT:
//      case DT_AUDIT:
//        if (do_dynamic) {
//          switch (entry->d_tag) {
//          case DT_AUXILIARY:
//            printf(_("Auxiliary library"));
//            break;
//
//          case DT_FILTER:
//            printf(_("Filter library"));
//            break;
//
//          case DT_CONFIG:
//            printf(_("Configuration file"));
//            break;
//
//          case DT_DEPAUDIT:
//            printf(_("Dependency audit library"));
//            break;
//
//          case DT_AUDIT:
//            printf(_("Audit library"));
//            break;
//          }
//
//          if (VALID_DYNAMIC_NAME(entry->d_un.d_val))
//            printf(": [%s]\n", GET_DYNAMIC_NAME(entry->d_un.d_val));
//          else {
//            printf(": ");
//            print_vma(entry->d_un.d_val, PREFIX_HEX);
//            putchar('\n');
//          }
//        }
//        break;
//
//      case DT_FEATURE:
//        if (do_dynamic) {
//          printf(_("Flags:"));
//
//          if (entry->d_un.d_val == 0)
//            printf(_(" None\n"));
//          else {
//            unsigned long int val = entry->d_un.d_val;
//
//            if (val & DTF_1_PARINIT) {
//              printf(" PARINIT");
//              val ^= DTF_1_PARINIT;
//            }
//            if (val & DTF_1_CONFEXP) {
//              printf(" CONFEXP");
//              val ^= DTF_1_CONFEXP;
//            }
//            if (val != 0)
//              printf(" %lx", val);
//            puts("");
//          }
//        }
//        break;
//
//      case DT_POSFLAG_1:
//        if (do_dynamic) {
//          printf(_("Flags:"));
//
//          if (entry->d_un.d_val == 0)
//            printf(_(" None\n"));
//          else {
//            unsigned long int val = entry->d_un.d_val;
//
//            if (val & DF_P1_LAZYLOAD) {
//              printf(" LAZYLOAD");
//              val ^= DF_P1_LAZYLOAD;
//            }
//            if (val & DF_P1_GROUPPERM) {
//              printf(" GROUPPERM");
//              val ^= DF_P1_GROUPPERM;
//            }
//            if (val != 0)
//              printf(" %lx", val);
//            puts("");
//          }
//        }
//        break;
//
//      case DT_FLAGS_1:
//        if (do_dynamic) {
//          printf(_("Flags:"));
//          if (entry->d_un.d_val == 0)
//            printf(_(" None\n"));
//          else {
//            unsigned long int val = entry->d_un.d_val;
//
//            if (val & DF_1_NOW) {
//              printf(" NOW");
//              val ^= DF_1_NOW;
//            }
//            if (val & DF_1_GLOBAL) {
//              printf(" GLOBAL");
//              val ^= DF_1_GLOBAL;
//            }
//            if (val & DF_1_GROUP) {
//              printf(" GROUP");
//              val ^= DF_1_GROUP;
//            }
//            if (val & DF_1_NODELETE) {
//              printf(" NODELETE");
//              val ^= DF_1_NODELETE;
//            }
//            if (val & DF_1_LOADFLTR) {
//              printf(" LOADFLTR");
//              val ^= DF_1_LOADFLTR;
//            }
//            if (val & DF_1_INITFIRST) {
//              printf(" INITFIRST");
//              val ^= DF_1_INITFIRST;
//            }
//            if (val & DF_1_NOOPEN) {
//              printf(" NOOPEN");
//              val ^= DF_1_NOOPEN;
//            }
//            if (val & DF_1_ORIGIN) {
//              printf(" ORIGIN");
//              val ^= DF_1_ORIGIN;
//            }
//            if (val & DF_1_DIRECT) {
//              printf(" DIRECT");
//              val ^= DF_1_DIRECT;
//            }
//            if (val & DF_1_TRANS) {
//              printf(" TRANS");
//              val ^= DF_1_TRANS;
//            }
//            if (val & DF_1_INTERPOSE) {
//              printf(" INTERPOSE");
//              val ^= DF_1_INTERPOSE;
//            }
//            if (val & DF_1_NODEFLIB) {
//              printf(" NODEFLIB");
//              val ^= DF_1_NODEFLIB;
//            }
//            if (val & DF_1_NODUMP) {
//              printf(" NODUMP");
//              val ^= DF_1_NODUMP;
//            }
//            if (val & DF_1_CONLFAT) {
//              printf(" CONLFAT");
//              val ^= DF_1_CONLFAT;
//            }
//            if (val != 0)
//              printf(" %lx", val);
//            puts("");
//          }
//        }
//        break;
//
//      case DT_PLTREL:
//        dynamic_info[entry->d_tag] = entry->d_un.d_val;
//        if (do_dynamic)
//          puts(get_dynamic_type(entry->d_un.d_val));
//        break;
//
//      case DT_NULL:
//      case DT_NEEDED:
//      case DT_PLTGOT:
//      case DT_HASH:
//      case DT_STRTAB:
//      case DT_SYMTAB:
//      case DT_RELA:
//      case DT_INIT:
//      case DT_FINI:
//      case DT_SONAME:
//      case DT_RPATH:
//      case DT_SYMBOLIC:
//      case DT_REL:
//      case DT_DEBUG:
//      case DT_TEXTREL:
//      case DT_JMPREL:
//      case DT_RUNPATH:
//        dynamic_info[entry->d_tag] = entry->d_un.d_val;
//
//        if (do_dynamic) {
//          char * name;
//
//          if (VALID_DYNAMIC_NAME(entry->d_un.d_val))
//            name = GET_DYNAMIC_NAME(entry->d_un.d_val);
//          else
//            name = NULL;
//
//          if (name) {
//            switch (entry->d_tag) {
//            case DT_NEEDED:
//              printf(_("Shared library: [%s]"), name);
//
//              if (streq(name, program_interpreter))
//                printf(_(" program interpreter"));
//              break;
//
//            case DT_SONAME:
//              printf(_("Library soname: [%s]"), name);
//              break;
//
//            case DT_RPATH:
//              printf(_("Library rpath: [%s]"), name);
//              break;
//
//            case DT_RUNPATH:
//              printf(_("Library runpath: [%s]"), name);
//              break;
//
//            default:
//              print_vma(entry->d_un.d_val, PREFIX_HEX);
//              break;
//            }
//          } else
//            print_vma(entry->d_un.d_val, PREFIX_HEX);
//
//          putchar('\n');
//        }
//        break;
//
//      case DT_PLTRELSZ:
//      case DT_RELASZ:
//      case DT_STRSZ:
//      case DT_RELSZ:
//      case DT_RELAENT:
//      case DT_SYMENT:
//      case DT_RELENT:
//        dynamic_info[entry->d_tag] = entry->d_un.d_val;
//      case DT_PLTPADSZ:
//      case DT_MOVEENT:
//      case DT_MOVESZ:
//      case DT_INIT_ARRAYSZ:
//      case DT_FINI_ARRAYSZ:
//      case DT_GNU_CONFLICTSZ:
//      case DT_GNU_LIBLISTSZ:
//        if (do_dynamic) {
//          print_vma(entry->d_un.d_val, UNSIGNED);
//          printf(_(" (bytes)\n"));
//        }
//        break;
//
//      case DT_VERDEFNUM:
//      case DT_VERNEEDNUM:
//      case DT_RELACOUNT:
//      case DT_RELCOUNT:
//        if (do_dynamic) {
//          print_vma(entry->d_un.d_val, UNSIGNED);
//          putchar('\n');
//        }
//        break;
//
//      case DT_SYMINSZ:
//      case DT_SYMINENT:
//      case DT_SYMINFO:
//      case DT_USED:
//      case DT_INIT_ARRAY:
//      case DT_FINI_ARRAY:
//        if (do_dynamic) {
//          if (entry->d_tag == DT_USED
//              && VALID_DYNAMIC_NAME(entry->d_un.d_val)) {
//            char * name = GET_DYNAMIC_NAME(entry->d_un.d_val);
//
//            if (*name) {
//              printf(_("Not needed object: [%s]\n"), name);
//              break;
//            }
//          }
//
//          print_vma(entry->d_un.d_val, PREFIX_HEX);
//          putchar('\n');
//        }
//        break;
//
//      case DT_BIND_NOW:
//        /* The value of this entry is ignored.  */
//        if (do_dynamic)
//          putchar('\n');
//        break;
//
//      case DT_GNU_PRELINKED:
//        if (do_dynamic) {
//          struct tm * tmp;
//          time_t atime = entry->d_un.d_val;
//
//          tmp = gmtime(&atime);
//          printf("%04u-%02u-%02uT%02u:%02u:%02u\n", tmp->tm_year + 1900,
//              tmp->tm_mon + 1, tmp->tm_mday, tmp->tm_hour, tmp->tm_min,
//              tmp->tm_sec);
//
//        }
//        break;
//
//      case DT_GNU_HASH:
//        dynamic_info_DT_GNU_HASH = entry->d_un.d_val;
//        if (do_dynamic) {
//          print_vma(entry->d_un.d_val, PREFIX_HEX);
//          putchar('\n');
//        }
//        break;
//
//      default:
//        if ((entry->d_tag >= DT_VERSYM) && (entry->d_tag <= DT_VERNEEDNUM))
//          version_info[DT_VERSIONTAGIDX (entry->d_tag)] = entry->d_un.d_val;
//
//        if (do_dynamic) {
//          switch (elf_header.e_machine) {
//          case EM_MIPS:
//          case EM_MIPS_RS3_LE:
//            dynamic_section_mips_val(entry);
//            break;
//          case EM_PARISC:
//            dynamic_section_parisc_val(entry);
//            break;
//          case EM_IA_64:
//            dynamic_section_ia64_val(entry);
//            break;
//          default:
//            print_vma(entry->d_un.d_val, PREFIX_HEX);
//            putchar('\n');
//          }
//        }
//        break;
//      }
//    }
//
//    return 1;
//  }
