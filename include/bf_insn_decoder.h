/**
 * @internal
 * @file bf_insn_decoder.h
 * @brief API of bf_insn_decoder.
 * @details This instruction decoder is responsible for analysing the output
 * of libopcodes. It fills in internal libopcodes information about what type
 * an instruction is as well as its branch targets (if any).
 *
 * The decoder is optimised to not repeat checks for instructions. The
 * implementation is written such that an instruction should strictly be
 * checked with breaks_flow() before branches_flow().
 *
 * Overall, we only care to distinguish between five types of instructions:
 *	- The most common type which does not affect control flow
 * (e.g. mov, cmp).
 *	- Instructions that break control flow (e.g. jmp).
 *	- Instructions that branch control flow (e.g. conditional jmps).
 *	- Instructions that call subroutines (e.g. call).
 *	- Instructions that end control flow (e.g. ret, sysret).
 * @author Mike Kwan <michael.kwan08@imperial.ac.uk>
 */

#ifndef BF_INSN_DECODER_H
#define BF_INSN_DECODER_H

#ifdef __cplusplus
extern "C" {
#endif

#define INSN_TO_ENUM(c...) A(0, c, 0, 0, 0, 0, 0, 0, 0)

#include "binary_file.h"
#include <inttypes.h>

/*
 * These macros allow us to use our instruction enumeration in a more readable way.
 */
#define head(h, t...) h
#define tail(h, t...) t

#define A(n, c...) (((uint64_t) (head(c))) << (n)) | B(n + 8, tail(c))
#define B(n, c...) (((uint64_t) (head(c))) << (n)) | C(n + 8, tail(c))
#define C(n, c...) (((uint64_t) (head(c))) << (n)) | D(n + 8, tail(c))
#define D(n, c...) (((uint64_t) (head(c))) << (n)) | E(n + 8, tail(c))
#define E(n, c...) (((uint64_t) (head(c))) << (n)) | F(n + 8, tail(c))
#define F(n, c...) (((uint64_t) (head(c))) << (n)) | G(n + 8, tail(c))
#define G(n, c...) (((uint64_t) (head(c))) << (n)) | H(n + 8, tail(c))
#define H(n, c...) (((uint64_t) (head(c))) << (n))

/**
 * @enum insn_mnemonic
 * @brief Enumeration of x86-32 and x86-64 mnemonics. This enumeration does not
 * include FPU or SIMD instructions such as SSE, MMX, etc.
 * @details This enumeration works by assigning the integer value of mnemonics
 * as that mnemonic's enum value. Since we only want to deal with a maximum
 * enum size of uint64_t, this implementation relies on the fact that all
 * mnemonics can be uniquely identified by their first 8 characters. This
 * allows <b>libind</b> to store semantic information about instructions very
 * efficiently. We append _insn to each enum name to avoid clashes with C
 * defined symbols.
 */
enum insn_mnemonic {
	aaa_insn		= INSN_TO_ENUM('a','a','a'),
	aad_insn		= INSN_TO_ENUM('a','a','d'),
	aam_insn		= INSN_TO_ENUM('a','a','m'),
	aas_insn		= INSN_TO_ENUM('a','a','s'),
	adc_insn		= INSN_TO_ENUM('a','d','c'),
	adcl_insn		= INSN_TO_ENUM('a','d','c','l'),
	add_insn		= INSN_TO_ENUM('a','d','d'),
	addb_insn		= INSN_TO_ENUM('a','d','d','b'),
	addl_insn		= INSN_TO_ENUM('a','d','d','l'),
	addq_insn		= INSN_TO_ENUM('a','d','d','q'),
	addsd_insn		= INSN_TO_ENUM('a','d','d','s','d'),
	addss_insn		= INSN_TO_ENUM('a','d','d','s','s'),
	and_insn		= INSN_TO_ENUM('a','n','d'),
	andb_insn		= INSN_TO_ENUM('a','n','d','b'),
	andl_insn		= INSN_TO_ENUM('a','n','d','l'),
	arpl_insn		= INSN_TO_ENUM('a','r','p','l'),
	bound_insn		= INSN_TO_ENUM('b','o','u','n','d'),
	bsf_insn		= INSN_TO_ENUM('b','s','f'),
	bsr_insn		= INSN_TO_ENUM('b','s','r'),
	bswap_insn		= INSN_TO_ENUM('b','s','w','a','p'),
	bt_insn			= INSN_TO_ENUM('b','t'),
	btc_insn		= INSN_TO_ENUM('b','t','c'),
	btr_insn		= INSN_TO_ENUM('b','t','r'),
	bts_insn		= INSN_TO_ENUM('b','t','s'),
	call_insn		= INSN_TO_ENUM('c','a','l','l'),
	callq_insn		= INSN_TO_ENUM('c','a','l','l','q'),
	cbw_insn		= INSN_TO_ENUM('c','b','w'),
	cdq_insn		= INSN_TO_ENUM('c','d','q'),
	clc_insn		= INSN_TO_ENUM('c','l','c'),
	cld_insn		= INSN_TO_ENUM('c','l','d'),
	cli_insn		= INSN_TO_ENUM('c','l','i'),
	cltq_insn		= INSN_TO_ENUM('c','l','t','q'),
	clts_insn		= INSN_TO_ENUM('c','l','t','s'),
	cmc_insn		= INSN_TO_ENUM('c','m','c'),
	cmova_insn		= INSN_TO_ENUM('c','m','o','v','a'),
	cmovae_insn		= INSN_TO_ENUM('c','m','o','v','a','e'),
	cmovb_insn		= INSN_TO_ENUM('c','m','o','v','b'),
	cmovbe_insn		= INSN_TO_ENUM('c','m','o','v','b','e'),
	cmovc_insn		= INSN_TO_ENUM('c','m','o','v','c'),
	cmove_insn		= INSN_TO_ENUM('c','m','o','v','e'),
	cmovg_insn		= INSN_TO_ENUM('c','m','o','v','g'),
	cmovge_insn		= INSN_TO_ENUM('c','m','o','v','g','e'),
	cmovl_insn		= INSN_TO_ENUM('c','m','o','v','l'),
	cmovle_insn		= INSN_TO_ENUM('c','m','o','v','l','e'),
	cmovna_insn		= INSN_TO_ENUM('c','m','o','v','n','a'),
	cmovnae_insn		= INSN_TO_ENUM('c','m','o','v','n','a','e'),
	cmovnb_insn		= INSN_TO_ENUM('c','m','o','v','n','b'),
	cmovnbe_insn		= INSN_TO_ENUM('c','m','o','v','n','b','e'),
	cmovnc_insn		= INSN_TO_ENUM('c','m','o','v','n','c'),
	cmovne_insn		= INSN_TO_ENUM('c','m','o','v','n','e'),
	cmovng_insn		= INSN_TO_ENUM('c','m','o','v','n','g'),
	cmovnge_insn		= INSN_TO_ENUM('c','m','o','v','n','g','e'),
	cmovnl_insn		= INSN_TO_ENUM('c','m','o','v','n','l'),
	cmovnle_insn		= INSN_TO_ENUM('c','m','o','v','n','l','e'),
	cmovno_insn		= INSN_TO_ENUM('c','m','o','v','n','o'),
	cmovnp_insn		= INSN_TO_ENUM('c','m','o','v','n','p'),
	cmovns_insn		= INSN_TO_ENUM('c','m','o','v','n','s'),
	cmovnz_insn		= INSN_TO_ENUM('c','m','o','v','n','z'),
	cmovo_insn		= INSN_TO_ENUM('c','m','o','v','o'),
	cmovp_insn		= INSN_TO_ENUM('c','m','o','v','p'),
	cmovpe_insn		= INSN_TO_ENUM('c','m','o','v','p','e'),
	cmovpo_insn		= INSN_TO_ENUM('c','m','o','v','p','o'),
	cmovs_insn		= INSN_TO_ENUM('c','m','o','v','s'),
	cmovz_insn		= INSN_TO_ENUM('c','m','o','v','z'),
	cmp_insn		= INSN_TO_ENUM('c','m','p'),
	cmpb_insn		= INSN_TO_ENUM('c','m','p','b'),
	cmpl_insn		= INSN_TO_ENUM('c','m','p','l'),
	cmpq_insn		= INSN_TO_ENUM('c','m','p','q'),
	cmpsb_insn		= INSN_TO_ENUM('c','m','p','s','b'),
	cmpsd_insn		= INSN_TO_ENUM('c','m','p','s','d'),
	cmpsw_insn		= INSN_TO_ENUM('c','m','p','s','w'),
	cmpw_insn		= INSN_TO_ENUM('c','m','p','w'),
	cmpxchg_insn		= INSN_TO_ENUM('c','m','p','x','c','h','g'),
	cpuid_insn		= INSN_TO_ENUM('c','p','u','i','d'),
	cvtsi2sd_insn		= INSN_TO_ENUM('c','v','t','s','i','2','s','d'),
	cvtsi2ss_insn		= INSN_TO_ENUM('c','v','t','s','i','2','s','s'),
	cvttsd2si_insn		= INSN_TO_ENUM('c','v','t','t','s','d','2','s',),
	cvttss2si_insn		= INSN_TO_ENUM('c','v','t','t','s','s','2','s',),
	cwd_insn		= INSN_TO_ENUM('c','w','d'),
	cwde_insn		= INSN_TO_ENUM('c','w','d','e'),
	cwtl_insn		= INSN_TO_ENUM('c','w','t','l'),
	daa_insn		= INSN_TO_ENUM('d','a','a'),
	das_insn		= INSN_TO_ENUM('d','a','s'),
	dec_insn		= INSN_TO_ENUM('d','e','c'),
	div_insn		= INSN_TO_ENUM('d','i','v'),
	divq_insn		= INSN_TO_ENUM('d','i','v','q'),
	divsd_insn		= INSN_TO_ENUM('d','i','v','s','d'),
	divss_insn		= INSN_TO_ENUM('d','i','v','s','s'),
	enter_insn		= INSN_TO_ENUM('e','n','t','e','r'),
	esc_insn		= INSN_TO_ENUM('e','s','c'),
	fadd_insn		= INSN_TO_ENUM('f','a','d','d'),
	fadds_insn		= INSN_TO_ENUM('f','a','d','d','s'),
	fchs_insn		= INSN_TO_ENUM('f','c','h','s'),
	fdivp_insn		= INSN_TO_ENUM('f','d','i','v','p'),
	fdivrp_insn		= INSN_TO_ENUM('f','d','i','v','r','p'),
	fildll_insn		= INSN_TO_ENUM('f','i','l','d','l','l'),
	fistpll_insn		= INSN_TO_ENUM('f','i','s','t','p','l','l'),
	fld_insn		= INSN_TO_ENUM('f','l','d'),
	fld1_insn		= INSN_TO_ENUM('f','l','d','1'),
	fldcw_insn		= INSN_TO_ENUM('f','l','d','c','w'),
	flds_insn		= INSN_TO_ENUM('f','l','d','s'),
	fldt_insn		= INSN_TO_ENUM('f','l','d','t'),
	fldz_insn		= INSN_TO_ENUM('f','l','d','z'),
	fmul_insn		= INSN_TO_ENUM('f','m','u','l'),
	fmulp_insn		= INSN_TO_ENUM('f','m','u','l','p'),
	fnstcw_insn		= INSN_TO_ENUM('f','n','s','t','c','w'),
	fnstsw_insn		= INSN_TO_ENUM('f','n','s','t','s','w'),
	fstp_insn		= INSN_TO_ENUM('f','s','t','p'),
	fstpt_insn		= INSN_TO_ENUM('f','s','t','p','t'),
	fsub_insn		= INSN_TO_ENUM('f','s','u','b'),
	fucomi_insn		= INSN_TO_ENUM('f','u','c','o','m','i'),
	fucomip_insn		= INSN_TO_ENUM('f','u','c','o','m','i','p'),
	fxam_insn		= INSN_TO_ENUM('f','x','a','m'),
	fxch_insn		= INSN_TO_ENUM('f','x','c','h'),
	hlt_insn		= INSN_TO_ENUM('h','l','t'),
	idiv_insn		= INSN_TO_ENUM('i','d','i','v'),
	idivl_insn		= INSN_TO_ENUM('i','d','i','v','l'),
	imul_insn		= INSN_TO_ENUM('i','m','u','l'),
	in_insn			= INSN_TO_ENUM('i','n'),
	inc_insn		= INSN_TO_ENUM('i','n','c'),
	ins_insn		= INSN_TO_ENUM('i','n','s'),
	insb_insn		= INSN_TO_ENUM('i','n','s','b'),
	insd_insn		= INSN_TO_ENUM('i','n','s','d'),
	insw_insn		= INSN_TO_ENUM('i','n','s','w'),
	int_insn		= INSN_TO_ENUM('i','n','t'),
	into_insn		= INSN_TO_ENUM('i','n','t','o'),
	invd_insn		= INSN_TO_ENUM('i','n','v','d'),
	invlpg_insn		= INSN_TO_ENUM('i','n','v','l','p','g'),
	iret_insn		= INSN_TO_ENUM('i','r','e','t'),
	iretd_insn		= INSN_TO_ENUM('i','r','e','t','d'),
	ja_insn			= INSN_TO_ENUM('j','a'),
	jae_insn		= INSN_TO_ENUM('j','a','e'),
	jb_insn			= INSN_TO_ENUM('j','b'),
	jbe_insn		= INSN_TO_ENUM('j','b','e'),
	jc_insn			= INSN_TO_ENUM('j','c'),
	jcxz_insn		= INSN_TO_ENUM('j','c','x','z'),
	je_insn			= INSN_TO_ENUM('j','e'),
	jecxz_insn		= INSN_TO_ENUM('j','e','c','x','z'),
	jg_insn			= INSN_TO_ENUM('j','g'),
	jge_insn		= INSN_TO_ENUM('j','g','e'),
	jl_insn			= INSN_TO_ENUM('j','l'),
	jle_insn		= INSN_TO_ENUM('j','l','e'),
	jmp_insn		= INSN_TO_ENUM('j','m','p'),
	jmpq_insn		= INSN_TO_ENUM('j','m','p','q'),
	jna_insn		= INSN_TO_ENUM('j','n','a'),
	jnae_insn		= INSN_TO_ENUM('j','n','a','e'),
	jnb_insn		= INSN_TO_ENUM('j','n','b'),
	jnbe_insn		= INSN_TO_ENUM('j','n','b','e'),
	jnc_insn		= INSN_TO_ENUM('j','n','c'),
	jne_insn		= INSN_TO_ENUM('j','n','e'),
	jng_insn		= INSN_TO_ENUM('j','n','g'),
	jnge_insn		= INSN_TO_ENUM('j','n','g','e'),
	jnl_insn		= INSN_TO_ENUM('j','n','l'),
	jnle_insn		= INSN_TO_ENUM('j','n','l','e'),
	jno_insn		= INSN_TO_ENUM('j','n','o'),
	jnp_insn		= INSN_TO_ENUM('j','n','p'),
	jns_insn		= INSN_TO_ENUM('j','n','s'),
	jnz_insn		= INSN_TO_ENUM('j','n','z'),
	jo_insn			= INSN_TO_ENUM('j','o'),
	jp_insn			= INSN_TO_ENUM('j','p'),
	jpe_insn		= INSN_TO_ENUM('j','p','e'),
	jpo_insn		= INSN_TO_ENUM('j','p','o'),
	js_insn			= INSN_TO_ENUM('j','s'),
	jz_insn			= INSN_TO_ENUM('j','z'),
	lahf_insn		= INSN_TO_ENUM('l','a','h','f'),
	lar_insn		= INSN_TO_ENUM('l','a','r'),
	lds_insn		= INSN_TO_ENUM('l','d','s'),
	lea_insn		= INSN_TO_ENUM('l','e','a'),
	leave_insn		= INSN_TO_ENUM('l','e','a','v','e'),
	leaveq_insn		= INSN_TO_ENUM('l','e','a','v','e','q'),
	les_insn		= INSN_TO_ENUM('l','e','s'),
	lfs_insn		= INSN_TO_ENUM('l','f','s'),
	lgdt_insn		= INSN_TO_ENUM('l','g','d','t'),
	lgs_insn		= INSN_TO_ENUM('l','g','s'),
	lidt_insn		= INSN_TO_ENUM('l','i','d','t'),
	lldt_insn		= INSN_TO_ENUM('l','l','d','t'),
	lmsw_insn		= INSN_TO_ENUM('l','m','s','w'),
	loadall_insn		= INSN_TO_ENUM('l','o','a','d','a','l','l'),
	lock_insn		= INSN_TO_ENUM('l','o','c','k'),
	lodsb_insn		= INSN_TO_ENUM('l','o','d','s','b'),
	lodsd_insn		= INSN_TO_ENUM('l','o','d','s','d'),
	lodsw_insn		= INSN_TO_ENUM('l','o','d','s','w'),
	loop_insn		= INSN_TO_ENUM('l','o','o','p'),
	loopd_insn		= INSN_TO_ENUM('l','o','o','p','d'),
	loope_insn		= INSN_TO_ENUM('l','o','o','p','e'),
	looped_insn		= INSN_TO_ENUM('l','o','o','p','e','d'),
	loopew_insn		= INSN_TO_ENUM('l','o','o','p','e','w'),
	loopne_insn		= INSN_TO_ENUM('l','o','o','p','n','e'),
	loopned_insn		= INSN_TO_ENUM('l','o','o','p','n','e','d'),
	loopnew_insn		= INSN_TO_ENUM('l','o','o','p','n','e','w'),
	loopnz_insn		= INSN_TO_ENUM('l','o','o','p','n','z'),
	loopnzd_insn		= INSN_TO_ENUM('l','o','o','p','n','z','d'),
	loopnzw_insn		= INSN_TO_ENUM('l','o','o','p','n','z','w'),
	loopw_insn		= INSN_TO_ENUM('l','o','o','p','w'),
	loopz_insn		= INSN_TO_ENUM('l','o','o','p','z'),
	loopzd_insn		= INSN_TO_ENUM('l','o','o','p','z','d'),
	loopzw_insn		= INSN_TO_ENUM('l','o','o','p','z','w'),
	lsl_insn		= INSN_TO_ENUM('l','s','l'),
	lss_insn		= INSN_TO_ENUM('l','s','s'),
	ltr_insn		= INSN_TO_ENUM('l','t','r'),
	maxsd_insn		= INSN_TO_ENUM('m','a','x','s','d'),
	mov_insn		= INSN_TO_ENUM('m','o','v'),
	movabs_insn		= INSN_TO_ENUM('m','o','v','a','b','s'),
	movapd_insn		= INSN_TO_ENUM('m','o','v','a','p','d'),
	movaps_insn		= INSN_TO_ENUM('m','o','v','a','p','s'),
	movb_insn		= INSN_TO_ENUM('m','o','v','b'),
	movl_insn		= INSN_TO_ENUM('m','o','v','l'),
	movq_insn		= INSN_TO_ENUM('m','o','v','q'),
	movsb_insn		= INSN_TO_ENUM('m','o','v','s','b'),
	movsbl_insn		= INSN_TO_ENUM('m','o','v','s','b','l'),
	movsbq_insn		= INSN_TO_ENUM('m','o','v','s','b','q'),
	movsbw_insn		= INSN_TO_ENUM('m','o','v','s','b','w'),
	movsd_insn		= INSN_TO_ENUM('m','o','v','s','d'),
	movslq_insn		= INSN_TO_ENUM('m','o','v','s','l','q'),
	movss_insn		= INSN_TO_ENUM('m','o','v','s','s'),
	movsw_insn		= INSN_TO_ENUM('m','o','v','s','w'),
	movswl_insn		= INSN_TO_ENUM('m','o','v','s','w','l'),
	movswq_insn		= INSN_TO_ENUM('m','o','v','s','w','q'),
	movsx_insn		= INSN_TO_ENUM('m','o','v','s','x'),
	movw_insn		= INSN_TO_ENUM('m','o','v','w'),
	movzbl_insn		= INSN_TO_ENUM('m','o','v','z','b','l'),
	movzwl_insn		= INSN_TO_ENUM('m','o','v','z','w','l'),
	movzx_insn		= INSN_TO_ENUM('m','o','v','z','x'),
	mul_insn		= INSN_TO_ENUM('m','u','l'),
	mulsd_insn		= INSN_TO_ENUM('m','u','l','s','d'),
	mulss_insn		= INSN_TO_ENUM('m','u','l','s','s'),
	neg_insn		= INSN_TO_ENUM('n','e','g'),
	negq_insn		= INSN_TO_ENUM('n','e','g','q'),
	nop_insn		= INSN_TO_ENUM('n','o','p'),
	nopl_insn		= INSN_TO_ENUM('n','o','p','l'),
	nopw_insn		= INSN_TO_ENUM('n','o','p','w'),
	not_insn		= INSN_TO_ENUM('n','o','t'),
	or_insn			= INSN_TO_ENUM('o','r'),
	orb_insn		= INSN_TO_ENUM('o','r','b'),
	orl_insn		= INSN_TO_ENUM('o','r','l'),
	orw_insn		= INSN_TO_ENUM('o','r','w'),
	out_insn		= INSN_TO_ENUM('o','u','t'),
	outs_insn		= INSN_TO_ENUM('o','u','t','s'),
	pop_insn		= INSN_TO_ENUM('p','o','p'),
	popa_insn		= INSN_TO_ENUM('p','o','p','a'),
	popad_insn		= INSN_TO_ENUM('p','o','p','a','d'),
	popf_insn		= INSN_TO_ENUM('p','o','p','f'),
	popfd_insn		= INSN_TO_ENUM('p','o','p','f','d'),
	push_insn		= INSN_TO_ENUM('p','u','s','h'),
	pusha_insn		= INSN_TO_ENUM('p','u','s','h','a'),
	pushad_insn		= INSN_TO_ENUM('p','u','s','h','a','d'),
	pushf_insn		= INSN_TO_ENUM('p','u','s','h','f'),
	pushfd_insn		= INSN_TO_ENUM('p','u','s','h','f','d'),
	rcl_insn		= INSN_TO_ENUM('r','c','l'),
	rcr_insn		= INSN_TO_ENUM('r','c','r'),
	rdmsr_insn		= INSN_TO_ENUM('r','d','m','s','r'),
	rdpmc_insn		= INSN_TO_ENUM('r','d','p','m','c'),
	rdtsc_insn		= INSN_TO_ENUM('r','d','t','s','c'),
	rep_insn		= INSN_TO_ENUM('r','e','p'),
	repe_insn		= INSN_TO_ENUM('r','e','p','e'),
	repne_insn		= INSN_TO_ENUM('r','e','p','n','e'),
	repnz_insn		= INSN_TO_ENUM('r','e','p','n','z'),
	repz_insn		= INSN_TO_ENUM('r','e','p','z'),
	ret_insn		= INSN_TO_ENUM('r','e','t'),
	retf_insn		= INSN_TO_ENUM('r','e','t','f'),
	retn_insn		= INSN_TO_ENUM('r','e','t','n'),
	retq_insn		= INSN_TO_ENUM('r','e','t','q'),
	rol_insn		= INSN_TO_ENUM('r','o','l'),
	roll_insn		= INSN_TO_ENUM('r','o','l','l'),
	ror_insn		= INSN_TO_ENUM('r','o','r'),
	rsm_insn		= INSN_TO_ENUM('r','s','m'),
	sahf_insn		= INSN_TO_ENUM('s','a','h','f'),
	sal_insn		= INSN_TO_ENUM('s','a','l'),
	sar_insn		= INSN_TO_ENUM('s','a','r'),
	sbb_insn		= INSN_TO_ENUM('s','b','b'),
	scasb_insn		= INSN_TO_ENUM('s','c','a','s','b'),
	scasd_insn		= INSN_TO_ENUM('s','c','a','s','d'),
	scasw_insn		= INSN_TO_ENUM('s','c','a','s','w'),
	seta_insn		= INSN_TO_ENUM('s','e','t','a'),
	setae_insn		= INSN_TO_ENUM('s','e','t','a','e'),
	setb_insn		= INSN_TO_ENUM('s','e','t','b'),
	setbe_insn		= INSN_TO_ENUM('s','e','t','b','e'),
	setc_insn		= INSN_TO_ENUM('s','e','t','c'),
	sete_insn		= INSN_TO_ENUM('s','e','t','e'),
	setg_insn		= INSN_TO_ENUM('s','e','t','g'),
	setge_insn		= INSN_TO_ENUM('s','e','t','g','e'),
	setl_insn		= INSN_TO_ENUM('s','e','t','l'),
	setle_insn		= INSN_TO_ENUM('s','e','t','l','e'),
	setna_insn		= INSN_TO_ENUM('s','e','t','n','a'),
	setnae_insn		= INSN_TO_ENUM('s','e','t','n','a','e'),
	setnb_insn		= INSN_TO_ENUM('s','e','t','n','b'),
	setnbe_insn		= INSN_TO_ENUM('s','e','t','n','b','e'),
	setnc_insn		= INSN_TO_ENUM('s','e','t','n','c'),
	setne_insn		= INSN_TO_ENUM('s','e','t','n','e'),
	setng_insn		= INSN_TO_ENUM('s','e','t','n','g'),
	setnge_insn		= INSN_TO_ENUM('s','e','t','n','g','e'),
	setnl_insn		= INSN_TO_ENUM('s','e','t','n','l'),
	setnle_insn		= INSN_TO_ENUM('s','e','t','n','l','e'),
	setno_insn		= INSN_TO_ENUM('s','e','t','n','o'),
	setnp_insn		= INSN_TO_ENUM('s','e','t','n','p'),
	setns_insn		= INSN_TO_ENUM('s','e','t','n','s'),
	setnz_insn		= INSN_TO_ENUM('s','e','t','n','z'),
	seto_insn		= INSN_TO_ENUM('s','e','t','o'),
	setp_insn		= INSN_TO_ENUM('s','e','t','p'),
	setpe_insn		= INSN_TO_ENUM('s','e','t','p','e'),
	setpo_insn		= INSN_TO_ENUM('s','e','t','p','o'),
	sets_insn		= INSN_TO_ENUM('s','e','t','s'),
	setz_insn		= INSN_TO_ENUM('s','e','t','z'),
	sgdt_insn		= INSN_TO_ENUM('s','g','d','t'),
	shl_insn		= INSN_TO_ENUM('s','h','l'),
	shld_insn		= INSN_TO_ENUM('s','h','l','d'),
	shlq_insn		= INSN_TO_ENUM('s','h','l','q'),
	shr_insn		= INSN_TO_ENUM('s','h','r'),
	shrd_insn		= INSN_TO_ENUM('s','h','r','d'),
	sidt_insn		= INSN_TO_ENUM('s','i','d','t'),
	sldt_insn		= INSN_TO_ENUM('s','l','d','t'),
	smsw_insn		= INSN_TO_ENUM('s','m','s','w'),
	stc_insn		= INSN_TO_ENUM('s','t','c'),
	std_insn		= INSN_TO_ENUM('s','t','d'),
	sti_insn		= INSN_TO_ENUM('s','t','i'),
	stosb_insn		= INSN_TO_ENUM('s','t','o','s','b'),
	stosd_insn		= INSN_TO_ENUM('s','t','o','s','d'),
	stosw_insn		= INSN_TO_ENUM('s','t','o','s','w'),
	str_insn		= INSN_TO_ENUM('s','t','r'),
	sub_insn		= INSN_TO_ENUM('s','u','b'),
	subl_insn		= INSN_TO_ENUM('s','u','b','l'),
	subq_insn		= INSN_TO_ENUM('s','u','b','q'),
	subsd_insn		= INSN_TO_ENUM('s','u','b','s','d'),
	subss_insn		= INSN_TO_ENUM('s','u','b','s','s'),
	syscall_insn		= INSN_TO_ENUM('s','y','s','c','a','l','l'),
	sysenter_insn		= INSN_TO_ENUM('s','y','s','e','n','t','e','r'),
	sysret_insn		= INSN_TO_ENUM('s','y','s','r','e','t'),
	test_insn		= INSN_TO_ENUM('t','e','s','t'),
	testb_insn		= INSN_TO_ENUM('t','e','s','t','b'),
	testl_insn		= INSN_TO_ENUM('t','e','s','t','l'),
	ucomisd_insn		= INSN_TO_ENUM('u','c','o','m','i','s','d'),
	ucomiss_insn		= INSN_TO_ENUM('u','c','o','m','i','s','s'),
	verr_insn		= INSN_TO_ENUM('v','e','r','r'),
	verw_insn		= INSN_TO_ENUM('v','e','r','w'),
	wait_insn		= INSN_TO_ENUM('w','a','i','t'),
	wbinvd_insn		= INSN_TO_ENUM('w','b','i','n','v','d'),
	wrmsr_insn		= INSN_TO_ENUM('w','r','m','s','r'),
	xadd_insn		= INSN_TO_ENUM('x','a','d','d'),
	xchg_insn		= INSN_TO_ENUM('x','c','h','g'),
	xlat_insn		= INSN_TO_ENUM('x','l','a','t'),
	xor_insn		= INSN_TO_ENUM('x','o','r'),
	xorb_insn		= INSN_TO_ENUM('x','o','r','b'),
	xorpd_insn		= INSN_TO_ENUM('x','o','r','p','d')
};

/**
 * @brief Returns whether the instruction breaks flow.
 * @param str The instruction being analysed.
 * @return TRUE if str represents an instruction which breaks flow. FALSE
 * otherwise.
 */
extern bool breaks_flow(char * str);

/**
 * @brief Returns whether the instruction branches flow.
 * @param str The instruction being analysed.
 * @return TRUE if str represents an instruction which branches flow. FALSE
 * otherwise.
 */
extern bool branches_flow(char * str);

/**
 * @brief Returns whether the instruction calls a subroutine.
 * @param str The instruction being analysed.
 * @return TRUE if str represents an instruction which calls a subroutine.
 * FALSE otherwise.
 */
extern bool calls_subroutine(char * str);

/**
 * @brief Returns whether the instruction ends flow.
 * @param str The instruction being analysed.
 * @return TRUE if str represents an instruction which ends flow. FALSE
 * otherwise.
 */
extern bool ends_flow(char * str);

/**
 * @brief Returns the VMA parsed from an operand.
 * @param str The operand being analysed.
 * @return The VMA if it could be successfully parsed. If the VMA is unknown
 * because of indirect branching/calling then 0 is returned.
 */
extern bfd_vma get_vma_target(char * str);

/**
 * @internal
 * @brief Returns whether the mnemonic is one enumerated by <b>libind</b>.
 * @param str The mnemonic.
 * @return TRUE if str represents a mnemonic enumerated by <b>libind</b>.
 * FALSE otherwise.
 * @details This is used for debugging. Generally a user would not use this.
 */
extern bool is_mnemonic(char * str);

#ifdef __cplusplus
}
#endif

#endif
