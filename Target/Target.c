#include <stdio.h>

void func1(int i)
{
	for(int j = 0; j < i; j++) {
		puts("asdf");
	}
}

int main(void)
{
	func1(7);
	return 0;
}
