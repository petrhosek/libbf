#include <bfd.h>
#include <dis-asm.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <libiberty.h>

#ifndef BINARY_FILE_H
#define BINARY_FILE_H

/*
 * Our wrapper around BFD. Eventually more members will be added to this.
 * Currently, we are exposing the definition so users can directly access
 * members but we can change this later.
 */
typedef struct binary_file {
	bfd *		   abfd;
	disassembler_ftype disassembler;
	disassemble_info   disasm_config;
} binary_file;

/*
 * Returns a binary_file object for the target passed in. NULL if a matching
 * BFD backend could not be found (because the BFD structure is useless to us
 * in that case).
 * 
 * close_binary_file must be called to allow the object to properly clean up.
 */
extern binary_file * load_binary_file(char *);

/*
 * Closes a lsd object obtained from calling
 * load_binary_file.
 */
extern bool close_binary_file(binary_file *);


/*
 * Specify a callback which is invoked for each discovered symbol
 */
extern bool binary_file_for_each_symbol(binary_file *, void (*)(binary_file *,
		asymbol *));

/*
 * At the moment disassembles the entry point instruction. But eventually we
 * want to fill some internal structure of binary_file with the CFG.
 */
extern bool disassemble_binary_file_entry(binary_file *);

/*
 * Disassemble from the symbol up to...
 */
extern bool disassemble_binary_file_symbol(binary_file *, asymbol *);

#endif
