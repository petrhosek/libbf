#include "bf_disasm.h"
#include "bf_insn_decoder.h"
#include "bf_basic_blk.h"

static void update_insn_info(struct binary_file * bf, char * str)
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

	struct binary_file * bf   = stream;
	va_list		     args;

	va_start(args, format);
	rv = vsnprintf(str, ARRAY_SIZE(str) - 1, format, args);
	va_end(args);

	add_insn_part(bf->context.insn, str);
	// puts(str);

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
static bool load_section(struct binary_file * bf, asection * s)
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

	printf("Loaded %d bytes at 0x%lX\n", size,
			bf->disasm_config.buffer_vma);
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
static bool load_section_for_vma(struct binary_file * bf, bfd_vma vma)
{
	BFD_VMA_SECTION req = {vma, NULL};
	bfd_map_over_sections(bf->abfd, vma_in_section, &req);

	if(!req.sec) {
		return FALSE;
	}

	return load_section(bf, req.sec);
}

static unsigned int disasm_single_insn(struct binary_file * bf, bfd_vma vma)
{
	bf->disasm_config.insn_info_valid = 0;
	return bf->disassembler(vma, &bf->disasm_config);
}

/*
 * We need to hoist the memory management to another module which ensures
 * the same section is never mapped twice.
 *
 * Still need to deal with symbol information.
 *
 *
 * Solution is to make some memory manager module which keeps track of
 * what has already been mapped.
 */
static struct bf_basic_blk * disasm_block(struct binary_file * bf, bfd_vma vma)
{
	void * buf;

	if(!load_section_for_vma(bf, vma)) {
		puts("Failed to load section");
		return NULL;
	}

	buf = bf->disasm_config.buffer;

	struct bf_basic_blk * bb = init_bf_basic_blk(vma);
	add_bb(bf, bb);
	bf->disasm_config.insn_type = dis_noninsn;

	while(bf->disasm_config.insn_type != dis_condjsr) {
		int size;

		bf->context.insn = init_bf_insn(vma);
		add_insn(bb, bf->context.insn);

		size = disasm_single_insn(bf, vma);

		if(size == -1 || size == 0) {
			puts("Something went wrong");
			free(buf);
			return NULL;
		}

		// printf("Disassembled %d bytes at 0x%lX\n\n", size, vma);

		if(bf->disasm_config.insn_type == dis_condbranch ||
				bf->disasm_config.insn_type == dis_jsr ||
				bf->disasm_config.insn_type == dis_branch) {
			/*
			 * For dis_jsr, we should consider adding the target to
			 * a function list for function enumeration. But for
			 * now, we treat a CALL the same as a conditional
			 * branch.
			 */
			if(bf->disasm_config.insn_type != dis_branch) {
				struct bf_basic_blk * bb_next =
						disasm_block(bf,
						vma + size);
				bf_add_next_basic_blk(bb, bb_next);
			}

			if(bf->disasm_config.target != 0) {
				struct bf_basic_blk * bb_branch =
						disasm_block(bf,
						bf->disasm_config.target);
				bf_add_next_basic_blk(bb, bb_branch);
			}

			bf->disasm_config.insn_type = dis_condjsr;
		}

		vma += size;
	}

	free(buf);
	return bb;
}

struct bf_basic_blk * disasm_generate_cflow(struct binary_file * bf, bfd_vma vma)
{
	/*if(!load_section_for_vma(bf, vma)) {
		return FALSE;
	}*/

	return disasm_block(bf, vma);

	//free(bf->disasm_config.buffer);
}

struct bf_basic_blk * disasm_from_sym(struct binary_file * bf, asymbol * sym)
{
	symbol_info info;

	bfd_symbol_info(sym, &info);
	return disasm_generate_cflow(bf, info.value);
}
