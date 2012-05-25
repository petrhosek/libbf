#include <stdlib.h>
#include <libbind/binary_file.h>
#include <libbind/detour.h>

void trampoline_target(char * target, char * output)
{
	struct bin_file * bf = load_bin_file(target, output);

	bf_trampoline_basic_blk(bf,
			disasm_bin_file_sym(bf,
			symbol_find(&bf->sym_table, "f1"), TRUE),
			disasm_bin_file_sym(bf,
			symbol_find(&bf->sym_table, "f_trampoline"), TRUE));
	close_bin_file(bf);
}

int main(void)
{
	trampoline_target("target64", "target64-output");
	trampoline_target("target32", "target32-output");
	return EXIT_SUCCESS;
}
