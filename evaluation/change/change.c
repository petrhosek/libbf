#include <stdlib.h>
#include <stdio.h>
#include <libbind/binary_file.h>
#include <libbind/symbol.h>

int main(void)
{
	struct bin_file * bf  = load_binary_file("target1", NULL);
	struct bin_file * bf2 = load_binary_file("target2", NULL);

	struct symbol * sym;
	for_each_symbol(sym, &bf->sym_table) {
		if(sym->type | SYMBOL_FUNCTION) {
			printf("%p, sym->name = %s\n", sym->address, sym->name);
		}
	}

	close_binary_file(bf);
	close_binary_file(bf2);
	return EXIT_SUCCESS;
}
