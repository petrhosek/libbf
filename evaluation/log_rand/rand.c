#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#ifdef __i386__
	#define TRAMPOLINE_BLOCK \
	({ \
	    asm volatile ("nop\n");\
	    asm volatile ("nop\n");\
	    asm volatile ("nop\n");\
	    asm volatile ("nop\n");\
	    asm volatile ("nop\n");\
	    asm volatile ("nop\n");\
	    asm volatile ("nop\n");\
	    asm volatile ("nop\n");\
	    asm volatile ("nop\n");\
	    asm volatile ("nop\n");\
	    asm volatile ("nop\n");\
	    asm volatile ("nop\n");\
	    asm volatile ("nop\n");\
	    asm volatile ("nop\n");\
	    asm volatile ("nop\n");\
	    asm volatile ("nop\n");\
	    asm volatile ("nop\n");\
	    asm volatile ("nop\n");\
	    asm volatile ("nop\n");\
	    asm volatile ("nop\n");\
	    asm volatile ("nop\n");\
	    asm volatile ("nop\n");\
	    asm volatile ("nop\n");\
	    asm volatile ("nop\n");\
	});
#endif

int new_func1(int num)
{
	return num == 0 ? num : 10/num;
}

void log_func(int num)
{
	printf("func1 was invoked with %d\n", num);
	TRAMPOLINE_BLOCK
}

int func1(int num)
{
	return 10/num;
}

int main(void)
{
	srand(time(NULL));

	for(int i = 0; i < 5; i++) {
		func1(rand() % 5);
	}

	return EXIT_SUCCESS;
}
