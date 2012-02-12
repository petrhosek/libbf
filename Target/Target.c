#include <stdio.h>

void func1(int i)
{
	if(i==7) {
		puts("Example puts output");
	} else {
		puts("Some other output");
	}
}

int main(void)
{
	func1(7);
	return 0;
}
