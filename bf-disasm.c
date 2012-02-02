#include "bf-disasm.h"

int binary_file_fprintf(void * stream, const char * format, ...)
{
	char str[512] = {0};
	int rv;

	binary_file * bf   = stream;
	va_list       args;

	va_start(args, format);
	rv = vsnprintf(str, ARRAY_SIZE(str) - 1, format, args);
	va_end(args);

	puts(str);

	/*
	 * Just some test code for the time being.
	 */
	if(strstr(str, "retq") != 0) {
		puts("Reached end of function");
		bf->is_end_block = TRUE;
	}

	return rv;
}

/*
 * Method of locating section from VMA taken from opdis.
 */
typedef struct {
	bfd_vma    vma;
	asection * sec;
} BFD_VMA_SECTION;

/*
 * It should be noted that any calls to load_section should eventually free
 * bf->disasm_config.buffer.
 */
static bool load_section(binary_file * bf, asection * s)
{
	int		size = bfd_section_size(s->owner, s);
	unsigned char * buf  = xmalloc(size);

	if(!bfd_get_section_contents(s->owner, s, buf, 0, size)) {
		free(buf);
		return FALSE;
	}

	bf->disasm_config.section	= s;
	bf->disasm_config.buffer	= buf;
	bf->disasm_config.buffer_length = size;
	bf->disasm_config.buffer_vma	= bfd_get_section_vma(s->owner, s);
	return TRUE;
}

static void vma_in_section(bfd * abfd, asection * s, void * data)
{
	BFD_VMA_SECTION * req = data;

	if(req && req->vma >= s->vma &&
	req->vma < (s->vma + bfd_section_size(abfd, s)) ) {
		req->sec = s;
	}
}

/*
 * Locates section containing a VMA and loads it.
 */
static bool load_section_for_vma(binary_file * bf, bfd_vma vma)
{
	BFD_VMA_SECTION req = {vma, NULL};
	bfd_map_over_sections(bf->abfd, vma_in_section, &req);

	if(!req.sec) {
		return FALSE;
	}

	return load_section(bf, req.sec);
}

static unsigned int disasm_single_insn(binary_file * bf, bfd_vma vma)
{
	unsigned int size;

	bf->disasm_config.insn_info_valid = 0;
	bf->is_end_block		  = FALSE;
	size = bf->disassembler(vma, &bf->disasm_config);
	return size;
}

bool disasm_generate_cflow(binary_file * bf, bfd_vma vma)
{
	bool disasm_success = TRUE;

	if(!load_section_for_vma(bf, vma)) {
		return FALSE;
	}

	while(true) {
		int size = disasm_single_insn(bf, vma);
		printf("Disassembled %d bytes at 0x%lX\n\n", size, vma);

		if(size == 0 || size == -1 || bf->is_end_block) {
			break;
		}

		vma += size;
	}

	free(bf->disasm_config.buffer);
	return disasm_success;
}

bool disasm_from_sym(binary_file * bf, asymbol * sym)
{
	symbol_info info;

	bfd_symbol_info(sym, &info);
	return disasm_generate_cflow(bf, info.value);
}
