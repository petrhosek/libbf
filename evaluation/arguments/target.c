#include <stdlib.h>
#include <stdio.h>

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

void f_trampoline(void)
{
	#ifdef __x86_64__
		void * rsp;
		asm ("movq %%rsp, %0" : "=g" (rsp));

		printf("%p\n", rsp);
	#else
		void * i;
		asm("movl %%esp, %0" : "=g" (i));

		printf("%p\n", i);
	#endif

	TRAMPOLINE_BLOCK
}

void f1(int i, double j, char * k)
{
//	f_trampoline();
	printf("%p\n", &i);
/*	printf("i = %d\n", i);
	printf("j = %g\n", j);
	printf("k = %s\n", k);*/
}

int main(void)
{
	f1(1, 2.0, "3");
	return EXIT_SUCCESS;
}
