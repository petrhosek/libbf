#include "binary_file.h"

binary_file * load_binary_file(char * target_path)
{
	binary_file * bf   = xmalloc(sizeof(binary_file));
	bfd *	      abfd = NULL;

	bfd_init();	
	/* not sure if this is needed */
	bfd_set_default_target("i686-pc-linux-gnu");

	bf->abfd = abfd = bfd_openr(target_path, NULL);

	if(abfd != NULL) {
		if(!bfd_check_format(abfd, bfd_object)) {
			bfd_close(abfd);
			free(bf);
			bf = NULL;
		}
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
