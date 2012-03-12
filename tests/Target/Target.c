#include <stdio.h>

void func1(int i)
{
	i += 7;

	do {
		i++;
	} while(i < 20);
}

int main(void)
{
	func1(7);
	return 0;
}
