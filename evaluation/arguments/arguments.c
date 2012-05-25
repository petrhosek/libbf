#include <stdlib.h>
#include <libbind/binary_file.h>
#include <libbind/detour.h>

int main(void)
{
	struct bin_file * bf = load_bin_file("target64", "target64-output");

	bf_detour_basic_blk(bf,
			disasm_bin_file_sym(bf,
			symbol_find(&bf->sym_table, "f1"), TRUE),
			disasm_bin_file_sym(bf,
			symbol_find(&bf->sym_table, "f_detour"), TRUE));
	close_bin_file(bf);
	return EXIT_SUCCESS;
}
