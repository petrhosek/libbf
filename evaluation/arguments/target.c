#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

void f_detour(void)
{
	#ifdef __x86_64__
		uint8_t * rsp;
		asm ("movq %%rsp, %0" : "=g" (rsp));

		printf("%p\n", rsp);
	#else
	#endif
}

void f1(int i, double j, char * k)
{
//	f_detour();
//	printf("%p\n", &i);
	printf("i = %d\n", i);
	printf("j = %g\n", j);
	printf("k = %s\n", k);
}

int main(void)
{
	f1(1, 2.0, "3");
	return EXIT_SUCCESS;
}
