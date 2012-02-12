#include "binary_file.h"
#include "bf_disasm.h"

/*
 * Initialising the opcodes disassembler. Instead of providing the real
 * fprintf, we redirect to our own version which writes to our bf object.
 * (Concept borrowed from opdis).
 */
static void init_bf_disassembler(struct binary_file * bf)
{
	init_disassemble_info(&bf->disasm_config, bf, binary_file_fprintf);

	bf->disasm_config.flavour = bfd_get_flavour(bf->abfd);
	bf->disasm_config.arch	  = bfd_get_arch(bf->abfd);
	bf->disasm_config.mach	  = bfd_get_mach(bf->abfd);
	bf->disasm_config.endian  = bf->abfd->xvec->byteorder;
	disassemble_init_for_target(&bf->disasm_config);

	bf->disassembler = disassembler(bf->abfd);
}

/*
 * Initialises the basic block hashtable.
 */
static void init_bf(struct binary_file * bf)
{
	htable_init(&bf->bb_table);
	htable_init(&bf->sym_table);
}

struct binary_file * load_binary_file(char * target_path)
{
	struct binary_file * bf   = xmalloc(sizeof(struct  binary_file));
	bfd *		     abfd = NULL;

	memset(&bf->disasm_config, 0, sizeof(bf->disasm_config));
	bfd_init();

	bf->abfd = abfd = bfd_openr(target_path, NULL);

	if(abfd) {
		if(!bfd_check_format(abfd, bfd_object)) {
			bfd_close(abfd);
			free(bf);
			bf = NULL;
		}

		init_bf(bf);
		init_bf_disassembler(bf);
		load_sym_table(bf);
	}

	return bf;
}

bool close_binary_file(struct binary_file * bf)
{
	bool success;

	close_sym_table(bf);
	htable_finit(&bf->bb_table);
	htable_finit(&bf->sym_table);
	success = bfd_close(bf->abfd);
	free(bf);
	return success;
}

bool binary_file_for_each_symbol(struct binary_file * bf,
		void (*handler)(struct binary_file * bf, asymbol *))
{
	bfd * abfd 	     = bf->abfd;
	long  storage_needed = bfd_get_symtab_upper_bound(abfd);

	if(storage_needed < 0) {
		return FALSE;
	} else if(storage_needed == 0) {
		return TRUE;
	} else {
		asymbol **symbol_table    = xmalloc(storage_needed);
		long    number_of_symbols = bfd_canonicalize_symtab(abfd, symbol_table);

		if(number_of_symbols < 0) {
			free(symbol_table);
			return FALSE;
		} else {
			for(long i = 0; i < number_of_symbols; i++) {
				handler(bf, symbol_table[i]);
			}
		}

		free(symbol_table);
		return TRUE;
	}
}

struct bf_basic_blk * disassemble_binary_file_entry(struct binary_file * bf)
{
	bfd_vma vma = bfd_get_start_address(bf->abfd);
	return disasm_generate_cflow(bf, vma);
}

struct bf_basic_blk * disassemble_binary_file_symbol(struct binary_file * bf,
		asymbol * sym)
{
	return disasm_from_sym(bf, sym);
}
