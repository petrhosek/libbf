#include "bf_insn_decoder.h"

bool breaks_flow(char * str)
{
	return (!strncmp("jmp", str, 3) || !strncmp("ljmp", str, 4));
}

bool branches_flow(char * str)
{
	return (!strncmp("loop", str, 4) || str[0] == 'j');
}

bool calls_subroutine(char * str)
{
	return (!strncmp("call", str, 4) || !strncmp("lcall", str, 5));
}

bool ends_flow(char * str)
{
	return (!strncmp("ret", str, 3) || !strncmp("lret", str, 4) ||
			!strncmp("iret", str, 4) || !strcmp("sysexit", str) ||
			!strcmp("sysret", str));
}

bfd_vma get_vma_target(char * str)
{
	bfd_vma vma = 0;

	if(sscanf(str, "0x%lX", &vma) != 1) {
		puts(str);
	} else {
		printf("parsed 0x%lX", vma);
	}

	return vma;
}
