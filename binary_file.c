#include "binary_file.h"
#include "bf_disasm.h"
#include "bf_func.h"
#include "bf_basic_blk.h"
#include "bf_mem_manager.h"

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
 * Initialises the internal hashtables of binary_file.
 */
static void init_bf(struct binary_file * bf)
{
	htable_init(&bf->func_table);
	htable_init(&bf->bb_table);
	htable_init(&bf->insn_table);
	htable_init(&bf->sym_table);
	htable_init(&bf->mem_table);

	bf->bitiness = bfd_arch_bits_per_address(bf->abfd) == 64 ?
			arch_64 : arch_32;
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

			printf("%s\n", target_path);
		} else {
			init_bf(bf);
			init_bf_disassembler(bf);
			load_sym_table(bf);
		}
	}

	return bf;
}

bool close_binary_file(struct binary_file * bf)
{
	bool success;

	bf_close_sym_table(bf);
	bf_close_func_table(bf);
	bf_close_bb_table(bf);
	bf_close_insn_table(bf);
	unload_all_sections(bf);

	htable_finit(&bf->func_table);
	htable_finit(&bf->bb_table);
	htable_finit(&bf->insn_table);
	htable_finit(&bf->sym_table);
	htable_finit(&bf->mem_table);
	success = bfd_close(bf->abfd);
	free(bf);
	return success;
}

struct bf_basic_blk * disassemble_binary_file_entry(struct binary_file * bf)
{
	bfd_vma vma = bfd_get_start_address(bf->abfd);
	return disasm_generate_cflow(bf, vma, TRUE);
}

struct bf_basic_blk * disassemble_binary_file_symbol(struct binary_file * bf,
		asymbol * sym, bool is_func)
{
	return disasm_from_sym(bf, sym, is_func);
}
