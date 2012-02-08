#include "bf-disasm.h"

/*
 * libopcodes appends spaces on the end of some instructions so for
 * comparisons, we want to strip those first.
 */
static void strip_tail(char * str, unsigned int size)
{
	int i;
	for(i = 0; i < size; i++) {
		if(!isgraph(str[i])) {
			str[i] = '\0';
			break;
		}
	}
}

/*
 * Checks whether the current instruction will cause the control flow to not
 * proceed to the linearly subsequent instruction (e.g. ret, jmp, etc.)
 */
static bool breaks_control_flow(binary_file * bf, char * str)
{
	if(ARCH_64(bf)) {
		if(strcmp(str, "retq") == 0) {
			return TRUE;
		}
	} else {
		if(strcmp(str, "ret") == 0) {
			return TRUE;
		}
	}
	return FALSE;
}

int binary_file_fprintf(void * stream, const char * format, ...)
{
	char str[256] = {0};
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
	strip_tail(str, ARRAY_SIZE(str));

	bf->disasm_config.insn_info_valid = TRUE;

	/* Treating returns, etc. as jump to subroutine */
	if(breaks_control_flow(bf, str)) {
		bf->disasm_config.insn_type = dis_jsr;
	} else {
		bf->disasm_config.insn_type = dis_nonbranch;
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
	bf->disasm_config.insn_info_valid = 0;
	// bf->is_end_block		  = FALSE;
	return bf->disassembler(vma, &bf->disasm_config);
}

bool is_end_basic_block(binary_file * bf)
{
	if(!bf->disasm_config.insn_info_valid) {
		puts("insn_info was invalid!");
		return FALSE;
	}

	switch(bf->disasm_config.insn_type) {
		case dis_jsr:
		case dis_noninsn:
			return TRUE;
		default:
			return FALSE;
	}
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

		if(is_end_basic_block(bf)) {
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
