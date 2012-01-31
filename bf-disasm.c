#include "bf-disasm.h"

int binary_file_fprintf(void * stream, const char * format, ...)
{
	char str[512] = {0};
	int rv;

	/* not used right now */
	binary_file * bf   = stream;
	va_list       args;

	va_start(args, format);
	rv = vsnprintf(str, ARRAY_SIZE(str) - 1, format, args);
	va_end(args);

	puts(str);

	return rv;
}
