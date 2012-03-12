#include <stdlib.h>
#include <stdio.h>

int main(void)
{
	puts("1234");
	asm("nop");
	asm("nop");
	asm("nop");
	asm("nop");
	asm("nop");
	return EXIT_SUCCESS;
}
