#include <stdlib.h>
#include <stdio.h>

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
}

int main(void)
{
	func1();
	return EXIT_SUCCESS;
}
