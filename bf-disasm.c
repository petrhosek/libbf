#include "bf-disasm.h"

int binary_file_fprintf(void * stream, const char * format, ...)
{
	char str[512] = {0};
	int rv;

	/* not used right now */
	/* binary_file * bf   = stream; */
	va_list       args;

	va_start(args, format);
	rv = vsnprintf(str, ARRAY_SIZE(str) - 1, format, args);
	va_end(args);

	puts(str);

	return rv;
}

/*
 * Method of locating section from VMA taken from opdis.
 */
typedef struct {
	bfd_vma    vma;
	asection * sec;
} BFD_VMA_SECTION;

bool load_section(binary_file * bf, asection * s)
{
	int		size = bfd_section_size(s->owner, s);
	unsigned char * buf  = xmalloc(size);

	if(!bfd_get_section_contents(s->owner, s, buf, 0, size)) {
		return FALSE;
	}

	bf->disasm_config.section	= s;
	bf->disasm_config.buffer	= buf;
	bf->disasm_config.buffer_length = size;
	return TRUE;
}

void vma_in_section(bfd * abfd, asection * s, void * data)
{
	BFD_VMA_SECTION * req = data;

	if(req && req->vma >= s->vma &&
	req->vma < (s->vma + bfd_section_size(abfd, s)) ) {
		req->sec = s;
	}
}

bool load_section_for_vma(binary_file * bf, bfd_vma vma)
{
	BFD_VMA_SECTION req = {vma, NULL};
	bfd_map_over_sections(bf->abfd, vma_in_section, &req);

	if(!req.sec) {
		return FALSE;
	}

	load_section(bf, req.sec);
	bf->disasm_config.buffer_vma = vma;
	return TRUE;
}
