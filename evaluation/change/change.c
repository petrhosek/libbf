#include <stdlib.h>
#include <stdio.h>
#include <libbind/binary_file.h>

int main(void)
{
	struct bin_file * bf  = load_binary_file("target1", NULL);
	struct bin_file * bf2 = load_binary_file("target2", NULL);

	close_binary_file(bf);
	close_binary_file(bf2);
	return EXIT_SUCCESS;
}
