#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#ifdef __x86_64__
	/* Should find a smarter way of doing this expansion */
	#define TRAMPOLINE_LENGTH 43
	#define TRAMPOLINE_BLOCK \
		({ \
		    /*
		     * Our detour takes up 14 bytes. Worst case is if there is
		     * an instruction on the 14th byte. The maximum length of
		     * an instruction is 15 bytes so we have to pad 28 bytes to
		     * be safe. Then we add another 14 bytes for the actual
		     * trampoline. One extra byte is needed to clean up the
		     * stack. This means the total padding required is 43
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
		    asm volatile ("nop\n");\
		});
#else
	#define TRAMPOLINE_LENGTH 25
	#define TRAMPOLINE_BLOCK \
		({ \
		    /*
		     * Our detour takes up 5 bytes. Worst case is if there is
		     * an instruction on the 5th byte. The maximum length of
		     * an instruction is 15 bytes so we have to pad 19 bytes to
		     * be safe. Then we add another 5 bytes for the actual
		     * trampoline. One extra byte is needed to clean up the
		     * stack. This means the total padding required is 25
		     * bytes.
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
		});
#endif

void f_trampoline(const int i, const double j, char const * const k)
{
	#ifdef __x86_64__
		uint32_t edi;
		__asm ("movl %%edi, %0;" : "=r" ( edi ));
	#endif

	printf("i = %d (detour)\n", i);
	printf("j = %g (detour)\n", j);
	printf("k = %s (detour)\n", k);

	#ifdef __x86_64__
		__asm ("movl %0, %%edi;" : "=d"( edi ));
	#endif

	TRAMPOLINE_BLOCK
}

void f1(int i, double j, char * k)
{
	printf("i = %d\n", i);
	printf("j = %g\n", j);
	printf("k = %s\n", k);
}

int main(void)
{
	f1(1, 2.0, "3");
	return EXIT_SUCCESS;
}
