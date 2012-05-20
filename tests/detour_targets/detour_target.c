#include <stdlib.h>
#include <stdio.h>

#ifdef __x86_64__
	/* Should find a smarter way of doing this expansion */
	#define TRAMPOLINE_LENGTH 42
	#define TRAMPOLINE_BLOCK \
		({ \
		    /*
		     * Our detour takes up 14 bytes. Worst case is if there is
		     * an instruction on the 14th byte. The maximum length of
		     * an instruction is 15 bytes so we have to pad 28 bytes to
		     * be safe. Then we add another 14 bytes for the actual
		     * trampoline. This means the total padding required is 42
		     * bytes.\
		     */ \
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
#else
	#define TRAMPOLINE_LENGTH 24
	#define TRAMPOLINE_BLOCK \
		({ \
		    /*
		     * Our detour takes up 5 bytes. Worst case is if there is
		     * an instruction on the 5th byte. The maximum length of
		     * an instruction is 15 bytes so we have to pad 19 bytes to
		     * be safe. Then we add another 5 bytes for the actual
		     * trampoline. This means the total padding required is 24
		     * bytes.\
		     */ \
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

/*
 * func1 is invoked by the regular execution of detour_test.
 */
void func1(void)
{
	puts("func1 was invoked");
}

/*
 * func2 is not invoked by the regular execution of detour_test. The aim is to
 * detour execution to this function.
 */
void func2(void)
{
	puts("func2 was invoked");

	TRAMPOLINE_BLOCK
}

int main(void)
{
	func1();
	return EXIT_SUCCESS;
}
