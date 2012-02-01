#include "binary_file.h"
#include "bf-disasm.h"

/*
 * Initialising the opcodes disassembler. Instead of providing the real
 * fprintf, we redirect to our own version which writes to our bf object.
 * (Concept borrowed from opdis).
 */
void init_bf_disassembler(binary_file * bf)
{
	init_disassemble_info(&bf->disasm_config, bf, binary_file_fprintf);
	bf->disasm_config.flavour = bfd_get_flavour(bf->abfd);
	bf->disasm_config.arch	  = bfd_get_arch(bf->abfd);
	bf->disasm_config.mach	  = bfd_get_mach(bf->abfd);
	bf->disasm_config.endian  = bf->abfd->xvec->byteorder;
	disassemble_init_for_target(&bf->disasm_config);

	bf->disassembler = disassembler(bf->abfd);
}

binary_file * load_binary_file(char * target_path)
{
	binary_file * bf   = xmalloc(sizeof(binary_file));
	bfd *	      abfd = NULL;

	memset(&bf->disasm_config, 0, sizeof(bf->disasm_config));
	bfd_init();

	bf->abfd = abfd = bfd_openr(target_path, NULL);

	if(abfd) {
		if(!bfd_check_format(abfd, bfd_object)) {
			bfd_close(abfd);
			free(bf);
			bf = NULL;
		}

		init_bf_disassembler(bf);
	}

	return bf;
}

bool close_binary_file(binary_file * bf)
{
	bool success = bfd_close(bf->abfd);
	free(bf);
	return success;
}

bool binary_file_for_each_symbol(binary_file * bf, void (*handler)(asymbol *))
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
			long i;

			for(i = 0; i < number_of_symbols; i++) {
				handler(symbol_table[i]);
			}
		}

		free(symbol_table);
		return TRUE;
	}
}

bool disassemble_binary_file_entry(binary_file * bf)
{
	bfd_vma vma = bfd_get_start_address(bf->abfd);
	return disassemble_binary_file_cflow(bf, vma);
}

bool disassemble_binary_file_symbol(binary_file * bf, asymbol * sym)
{
	return TRUE;
}
