#include "bf-disasm.h"
#include "bf_insn_decoder.h"

static disassemble_info * save_disasm_context(binary_file * bf)
{
	disassemble_info * context = xmalloc(sizeof(disassemble_info));
	memcpy(context, &bf->disasm_config, sizeof(disassemble_info));
	return context;
}

static void restore_disasm_context(binary_file * bf,
		disassemble_info * context)
{
	memcpy(&bf->disasm_config, context, sizeof(disassemble_info));
	free(context);
}

static void update_insn_info(binary_file * bf, char * str)
{
	if(!bf->disasm_config.insn_info_valid) {
		/*
		 * We come in here if we are analysing the mnemonic part.
		 * In x86 there are never two targets for branching so we
		 * use target2 to keep track of whether the branch target
		 * has been checked yet.
		 */

		bf->disasm_config.insn_info_valid = TRUE;
		bf->disasm_config.target2	  = 0;

		/*
		 * We use dis_condjsr to represent instructions which end flow
		 * even though it is not quite appropriate. It is the best fit.
		 */

		if(breaks_flow(str)) {
			bf->disasm_config.insn_type = dis_branch;
		} else if(branches_flow(str)) {
			bf->disasm_config.insn_type = dis_condbranch;
		} else if(calls_subroutine(str)) {
			bf->disasm_config.insn_type = dis_jsr;
		} else if(ends_flow(str)) {
			bf->disasm_config.insn_type = dis_condjsr;
		} else {
			bf->disasm_config.insn_type = dis_nonbranch;
		}
	} else {
		if(bf->disasm_config.target2 == 0) {
			bf->disasm_config.target2 = 1;

			switch(bf->disasm_config.insn_type) {
			case dis_branch:
			case dis_condbranch:
			case dis_jsr:
				bf->disasm_config.target = get_vma_target(str);
				/* Just fall through */
			default:
				break;
			}
		}
	}
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

	update_insn_info(bf, str);
	return rv;
}

/*
 * Method of locating section from VMA borrowed from opdis.
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

	printf("Loaded %d bytes at 0x%lX\n", size, bf->disasm_config.buffer_vma);
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
	return bf->disassembler(vma, &bf->disasm_config);
}

static void disasm_function(binary_file * bf, bfd_vma vma)
{
	bf->disasm_config.insn_type = dis_noninsn;

	/* A function starts here */
	/* A basic block starts here */
	while(bf->disasm_config.insn_type != dis_condjsr) {
		int size = disasm_single_insn(bf, vma);
		if(size == -1 || size == 0) {
			puts("Something went wrong");
			return;
		}

		printf("Disassembled %d bytes at 0x%lX\n\n", size, vma);

		switch(bf->disasm_config.insn_type) {
		case dis_branch:
			// End basic block
			// Start new basic block
			break;
		case dis_condbranch:
			// End basic block
			// ...
			// Start new basic block
			break;
		case dis_jsr: {
			// End basic block

			/* We really want to check whether the section is already mapped */
			disassemble_info * context = save_disasm_context(bf);
			disasm_generate_cflow(bf, bf->disasm_config.target);
			restore_disasm_context(bf, context);
			// Start new basic block
			break;
		}
		case dis_condjsr:
			// Returned...
			break;
		case dis_nonbranch:
			// Building basic block
			break;
		default:
			break;
		}

		vma += size;
	}
	/* A function ends here */
}

bool disasm_generate_cflow(binary_file * bf, bfd_vma vma)
{
	if(!load_section_for_vma(bf, vma)) {
		return FALSE;
	}

	disasm_function(bf, vma);

	free(bf->disasm_config.buffer);
	return TRUE;
}

bool disasm_from_sym(binary_file * bf, asymbol * sym)
{
	symbol_info info;

	bfd_symbol_info(sym, &info);
	return disasm_generate_cflow(bf, info.value);
}
