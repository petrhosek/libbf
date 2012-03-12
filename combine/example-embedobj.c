#include <stdlib.h>
#include <stdio.h>

/*
 * Fake main. Never used, just there so we can perform a full link.
 */
int main(void)
{
	return EXIT_SUCCESS;
}

void func1(void)
{
	puts("asdf");
}
