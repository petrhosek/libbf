#include <stdlib.h>
#include <stdio.h>
#include <libbind/binary_file.h>
#include <libbind/func.h>
#include <libbind/detour.h>

int main(void)
{
	/*
	 * The target is loaded and NULL is specified for the output file.
	 * This means the target is edited as opposed to a new output created.
	 */
	struct bin_file * bf = load_bin_file("rand", NULL);
	struct bf_func *  src_func, * dest_func;

	disasm_all_func_sym(bf);
	src_func  = bf_get_func_from_name(bf, "func1");
	dest_func = bf_get_func_from_name(bf, "log_func");

	if(!bf_trampoline_func(bf, src_func, dest_func)) {
		puts("asdf");
	}

	close_bin_file(bf);
	return EXIT_SUCCESS;
}
