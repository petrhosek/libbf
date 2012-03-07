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
 * @brief Enumeration of x86-32 and x86-64 mnemonics.
 * @details This enumeration works by assigning the integer value of mnemonics
 * as that mnemonic's enum value. Since we only want to deal with a maximum
 * enum size of uint64_t, this implementation relies on the fact that all
 * mnemonics can be uniquely identified by their first 8 characters. This
 * allows <b>libind</b> to store semantic information about instructions very
 * efficiently. We append _insn to each enum name to avoid clashes with C
 * defined symbols.
 */
enum insn_mnemonic {
	aaa_insn	= INSN_TO_ENUM('a','a','a'),
	aad_insn	= INSN_TO_ENUM('a','a','d'),
	aam_insn	= INSN_TO_ENUM('a','a','m'),
	aas_insn	= INSN_TO_ENUM('a','a','s'),
	adc_insn	= INSN_TO_ENUM('a','d','c'),
	adcl_insn	= INSN_TO_ENUM('a','d','c','l'),
	add_insn	= INSN_TO_ENUM('a','d','d'),
	addb_insn	= INSN_TO_ENUM('a','d','d','b'),
	addl_insn	= INSN_TO_ENUM('a','d','d','l'),
	addq_insn	= INSN_TO_ENUM('a','d','d','q'),
	addsd_insn	= INSN_TO_ENUM('a','d','d','s','d'),
	addss_insn	= INSN_TO_ENUM('a','d','d','s','s'),
	and_insn	= INSN_TO_ENUM('a','n','d'),
	andb_insn	= INSN_TO_ENUM('a','n','d','b'),
	andl_insn	= INSN_TO_ENUM('a','n','d','l'),
	arpl_insn	= INSN_TO_ENUM('a','r','p','l'),
	bound_insn	= INSN_TO_ENUM('b','o','u','n','d'),
	bsf_insn	= INSN_TO_ENUM('b','s','f'),
	bsr_insn	= INSN_TO_ENUM('b','s','r'),
	bswap_insn	= INSN_TO_ENUM('b','s','w','a','p'),
	bt_insn		= INSN_TO_ENUM('b','t'),
	btc_insn	= INSN_TO_ENUM('b','t','c'),
	btr_insn	= INSN_TO_ENUM('b','t','r'),
	bts_insn	= INSN_TO_ENUM('b','t','s'),
	call_insn	= INSN_TO_ENUM('c','a','l','l'),
	callq_insn	= INSN_TO_ENUM('c','a','l','l','q'),
	cbw_insn	= INSN_TO_ENUM('c','b','w'),
	cdq_insn	= INSN_TO_ENUM('c','d','q'),
	clc_insn	= INSN_TO_ENUM('c','l','c'),
	cld_insn	= INSN_TO_ENUM('c','l','d'),
	cli_insn	= INSN_TO_ENUM('c','l','i'),
	cltq_insn	= INSN_TO_ENUM('c','l','t','q'),
	clts_insn	= INSN_TO_ENUM('c','l','t','s'),
	cmc_insn	= INSN_TO_ENUM('c','m','c'),
	cmova_insn	= INSN_TO_ENUM('c','m','o','v','a'),
	cmovae_insn	= INSN_TO_ENUM('c','m','o','v','a','e'),
	cmovb_insn	= INSN_TO_ENUM('c','m','o','v','b'),
	cmovbe_insn	= INSN_TO_ENUM('c','m','o','v','b','e'),
	cmovc_insn	= INSN_TO_ENUM('c','m','o','v','c'),
	cmove_insn	= INSN_TO_ENUM('c','m','o','v','e'),
	cmovg_insn	= INSN_TO_ENUM('c','m','o','v','g'),
	cmovge_insn	= INSN_TO_ENUM('c','m','o','v','g','e'),
	cmovl_insn	= INSN_TO_ENUM('c','m','o','v','l'),
	cmovle_insn	= INSN_TO_ENUM('c','m','o','v','l','e'),
	cmovna_insn	= INSN_TO_ENUM('c','m','o','v','n','a'),
	cmovnae_insn	= INSN_TO_ENUM('c','m','o','v','n','a','e'),
	cmovnb_insn	= INSN_TO_ENUM('c','m','o','v','n','b'),
	cmovnbe_insn	= INSN_TO_ENUM('c','m','o','v','n','b','e'),
	cmovnc_insn	= INSN_TO_ENUM('c','m','o','v','n','c'),
	cmovne_insn	= INSN_TO_ENUM('c','m','o','v','n','e'),
	cmovng_insn	= INSN_TO_ENUM('c','m','o','v','n','g'),
	cmovnge_insn	= INSN_TO_ENUM('c','m','o','v','n','g','e'),
	cmovnl_insn	= INSN_TO_ENUM('c','m','o','v','n','l'),
	cmovnle_insn	= INSN_TO_ENUM('c','m','o','v','n','l','e'),
	cmovno_insn	= INSN_TO_ENUM('c','m','o','v','n','o'),
	cmovnp_insn	= INSN_TO_ENUM('c','m','o','v','n','p'),
	cmovns_insn	= INSN_TO_ENUM('c','m','o','v','n','s'),
	cmovnz_insn	= INSN_TO_ENUM('c','m','o','v','n','z'),
	cmovo_insn	= INSN_TO_ENUM('c','m','o','v','o'),
	cmovp_insn	= INSN_TO_ENUM('c','m','o','v','p'),
	cmovpe_insn	= INSN_TO_ENUM('c','m','o','v','p','e'),
	cmovpo_insn	= INSN_TO_ENUM('c','m','o','v','p','o'),
	cmovs_insn	= INSN_TO_ENUM('c','m','o','v','s'),
	cmovz_insn	= INSN_TO_ENUM('c','m','o','v','z'),
	cmp_insn	= INSN_TO_ENUM('c','m','p'),
	cmpb_insn	= INSN_TO_ENUM('c','m','p','b'),
	cmpl_insn	= INSN_TO_ENUM('c','m','p','l'),
	cmpq_insn	= INSN_TO_ENUM('c','m','p','q'),
	cmpsb_insn	= INSN_TO_ENUM('c','m','p','s','b'),
	cmpsd_insn	= INSN_TO_ENUM('c','m','p','s','d'),
	cmpsw_insn	= INSN_TO_ENUM('c','m','p','s','w'),
	cmpw_insn	= INSN_TO_ENUM('c','m','p','w'),
	cmpxchg_insn	= INSN_TO_ENUM('c','m','p','x','c','h','g'),
	cpuid_insn	= INSN_TO_ENUM('c','p','u','i','d'),
	cvtsi2sd_insn	= INSN_TO_ENUM('c','v','t','s','i','2','s','d'),
	cvtsi2ss_insn	= INSN_TO_ENUM('c','v','t','s','i','2','s','s'),
	cvttsd2si_insn	= INSN_TO_ENUM('c','v','t','t','s','d','2','s',),
	cvttss2si_insn	= INSN_TO_ENUM('c','v','t','t','s','s','2','s',),
	cwd_insn	= INSN_TO_ENUM('c','w','d'),
	cwde_insn	= INSN_TO_ENUM('c','w','d','e'),
	cwtl_insn	= INSN_TO_ENUM('c','w','t','l'),
	daa_insn	= INSN_TO_ENUM('d','a','a'),
	das_insn	= INSN_TO_ENUM('d','a','s'),
	dec_insn	= INSN_TO_ENUM('d','e','c'),
	div_insn	= INSN_TO_ENUM('d','i','v'),
	divq_insn	= INSN_TO_ENUM('d','i','v','q'),
	divsd_insn	= INSN_TO_ENUM('d','i','v','s','d'),
	divss_insn	= INSN_TO_ENUM('d','i','v','s','s'),
	enter_insn	= INSN_TO_ENUM('e','n','t','e','r'),
	esc_insn	= INSN_TO_ENUM('e','s','c'),
	fadd_insn	= INSN_TO_ENUM('f','a','d','d'),
	fadds_insn	= INSN_TO_ENUM('f','a','d','d','s'),
	fchs_insn	= INSN_TO_ENUM('f','c','h','s'),
	fdivp_insn	= INSN_TO_ENUM('f','d','i','v','p'),
	fdivrp_insn	= INSN_TO_ENUM('f','d','i','v','r','p'),
	fildll_insn	= INSN_TO_ENUM('f','i','l','d','l','l'),
	fistpll_insn	= INSN_TO_ENUM('f','i','s','t','p','l','l'),
	fld_insn	= INSN_TO_ENUM('f','l','d'),
	fld1_insn	= INSN_TO_ENUM('f','l','d','1'),
	fldcw_insn	= INSN_TO_ENUM('f','l','d','c','w'),
	flds_insn	= INSN_TO_ENUM('f','l','d','s'),
	fldt_insn	= INSN_TO_ENUM('f','l','d','t'),
	fldz_insn	= INSN_TO_ENUM('f','l','d','z'),
	fmul_insn	= INSN_TO_ENUM('f','m','u','l'),
	fmulp_insn	= INSN_TO_ENUM('f','m','u','l','p'),
	fnstcw_insn	= INSN_TO_ENUM('f','n','s','t','c','w'),
	fnstsw_insn	= INSN_TO_ENUM('f','n','s','t','s','w'),
	fstp_insn	= INSN_TO_ENUM('f','s','t','p'),
	fstpt_insn	= INSN_TO_ENUM('f','s','t','p','t'),
	fsub_insn	= INSN_TO_ENUM('f','s','u','b'),
	fucomi_insn	= INSN_TO_ENUM('f','u','c','o','m','i'),
	fucomip_insn	= INSN_TO_ENUM('f','u','c','o','m','i','p'),
	fxam_insn	= INSN_TO_ENUM('f','x','a','m'),
	fxch_insn	= INSN_TO_ENUM('f','x','c','h'),
	hlt_insn	= INSN_TO_ENUM('h','l','t'),
	idiv_insn	= INSN_TO_ENUM('i','d','i','v'),
	idivl_insn	= INSN_TO_ENUM('i','d','i','v','l'),
	imul_insn	= INSN_TO_ENUM('i','m','u','l'),
	in_insn		= INSN_TO_ENUM('i','n'),
	inc_insn	= INSN_TO_ENUM('i','n','c'),
	ins_insn	= INSN_TO_ENUM('i','n','s'),
	insb_insn	= INSN_TO_ENUM('i','n','s','b'),
	insd_insn	= INSN_TO_ENUM('i','n','s','d'),
	insw_insn	= INSN_TO_ENUM('i','n','s','w'),
	int_insn	= INSN_TO_ENUM('i','n','t'),
	into_insn	= INSN_TO_ENUM('i','n','t','o'),
	invd_insn	= INSN_TO_ENUM('i','n','v','d'),
	invlpg_insn	= INSN_TO_ENUM('i','n','v','l','p','g'),
	iret_insn	= INSN_TO_ENUM('i','r','e','t'),
	iretd_insn	= INSN_TO_ENUM('i','r','e','t','d'),
	ja_insn		= INSN_TO_ENUM('j','a'),
	jae_insn	= INSN_TO_ENUM('j','a','e'),
	jb_insn		= INSN_TO_ENUM('j','b'),
	jbe_insn	= INSN_TO_ENUM('j','b','e'),
	jc_insn		= INSN_TO_ENUM('j','c'),
	jcxz_insn	= INSN_TO_ENUM('j','c','x','z'),
	je_insn		= INSN_TO_ENUM('j','e'),
	jecxz_insn	= INSN_TO_ENUM('j','e','c','x','z'),
	jg_insn		= INSN_TO_ENUM('j','g'),
	jge_insn	= INSN_TO_ENUM('j','g','e'),
	jl_insn		= INSN_TO_ENUM('j','l'),
	jle_insn	= INSN_TO_ENUM('j','l','e'),
	jmp_insn	= INSN_TO_ENUM('j','m','p'),
	jmpq_insn	= INSN_TO_ENUM('j','m','p','q'),
	jna_insn	= INSN_TO_ENUM('j','n','a'),
	jnae_insn	= INSN_TO_ENUM('j','n','a','e'),
	jnb_insn	= INSN_TO_ENUM('j','n','b'),
	jnbe_insn	= INSN_TO_ENUM('j','n','b','e'),
	jnc_insn	= INSN_TO_ENUM('j','n','c'),
	jne_insn	= INSN_TO_ENUM('j','n','e'),
	jng_insn	= INSN_TO_ENUM('j','n','g'),
	jnge_insn	= INSN_TO_ENUM('j','n','g','e'),
	jnl_insn	= INSN_TO_ENUM('j','n','l'),
	jnle_insn	= INSN_TO_ENUM('j','n','l','e'),
	jno_insn	= INSN_TO_ENUM('j','n','o'),
	jnp_insn	= INSN_TO_ENUM('j','n','p'),
	jns_insn	= INSN_TO_ENUM('j','n','s'),
	jnz_insn	= INSN_TO_ENUM('j','n','z'),
	jo_insn		= INSN_TO_ENUM('j','o'),
	jp_insn		= INSN_TO_ENUM('j','p'),
	jpe_insn	= INSN_TO_ENUM('j','p','e'),
	jpo_insn	= INSN_TO_ENUM('j','p','o'),
	js_insn		= INSN_TO_ENUM('j','s'),
	jz_insn		= INSN_TO_ENUM('j','z'),
	lahf_insn	= INSN_TO_ENUM('l','a','h','f'),
	lar_insn	= INSN_TO_ENUM('l','a','r'),
	lds_insn	= INSN_TO_ENUM('l','d','s'),
	lea_insn	= INSN_TO_ENUM('l','e','a'),
	leave_insn	= INSN_TO_ENUM('l','e','a','v','e'),
	leaveq_insn	= INSN_TO_ENUM('l','e','a','v','e','q'),
	les_insn	= INSN_TO_ENUM('l','e','s'),
	lfs_insn	= INSN_TO_ENUM('l','f','s'),
	lgdt_insn	= INSN_TO_ENUM('l','g','d','t'),
	lgs_insn	= INSN_TO_ENUM('l','g','s'),
	lidt_insn	= INSN_TO_ENUM('l','i','d','t'),
	lldt_insn	= INSN_TO_ENUM('l','l','d','t'),
	lmsw_insn	= INSN_TO_ENUM('l','m','s','w'),
	loadall_insn	= INSN_TO_ENUM('l','o','a','d','a','l','l'),
	lock_insn	= INSN_TO_ENUM('l','o','c','k'),
	lodsb_insn	= INSN_TO_ENUM('l','o','d','s','b'),
	lodsd_insn	= INSN_TO_ENUM('l','o','d','s','d'),
	lodsw_insn	= INSN_TO_ENUM('l','o','d','s','w'),
	loop_insn	= INSN_TO_ENUM('l','o','o','p'),
	loopd_insn	= INSN_TO_ENUM('l','o','o','p','d'),
	loope_insn	= INSN_TO_ENUM('l','o','o','p','e'),
	looped_insn	= INSN_TO_ENUM('l','o','o','p','e','d'),
	loopew_insn	= INSN_TO_ENUM('l','o','o','p','e','w'),
	loopne_insn	= INSN_TO_ENUM('l','o','o','p','n','e'),
	loopned_insn	= INSN_TO_ENUM('l','o','o','p','n','e','d'),
	loopnew_insn	= INSN_TO_ENUM('l','o','o','p','n','e','w'),
	loopnz_insn	= INSN_TO_ENUM('l','o','o','p','n','z'),
	loopnzd_insn	= INSN_TO_ENUM('l','o','o','p','n','z','d'),
	loopnzw_insn	= INSN_TO_ENUM('l','o','o','p','n','z','w'),
	loopw_insn	= INSN_TO_ENUM('l','o','o','p','w'),
	loopz_insn	= INSN_TO_ENUM('l','o','o','p','z'),
	loopzd_insn	= INSN_TO_ENUM('l','o','o','p','z','d'),
	loopzw_insn	= INSN_TO_ENUM('l','o','o','p','z','w'),
	lsl_insn	= INSN_TO_ENUM('l','s','l'),
	lss_insn	= INSN_TO_ENUM('l','s','s'),
	ltr_insn	= INSN_TO_ENUM('l','t','r'),
	maxsd_insn	= INSN_TO_ENUM('m','a','x','s','d'),
	mov_insn	= INSN_TO_ENUM('m','o','v'),
	movabs_insn	= INSN_TO_ENUM('m','o','v','a','b','s'),
	movapd_insn	= INSN_TO_ENUM('m','o','v','a','p','d'),
	movaps_insn	= INSN_TO_ENUM('m','o','v','a','p','s'),
	movb_insn	= INSN_TO_ENUM('m','o','v','b'),
	movl_insn	= INSN_TO_ENUM('m','o','v','l'),
	movq_insn	= INSN_TO_ENUM('m','o','v','q'),
	movsb_insn	= INSN_TO_ENUM('m','o','v','s','b'),
	movsbl_insn	= INSN_TO_ENUM('m','o','v','s','b','l'),
	movsbq_insn	= INSN_TO_ENUM('m','o','v','s','b','q'),
	movsbw_insn	= INSN_TO_ENUM('m','o','v','s','b','w'),
	movsd_insn	= INSN_TO_ENUM('m','o','v','s','d'),
	movsl_insn	= INSN_TO_ENUM('m','o','v','s','l'),
	movslq_insn	= INSN_TO_ENUM('m','o','v','s','l','q'),
	movsq_insn	= INSN_TO_ENUM('m','o','v','s','q'),
	movss_insn	= INSN_TO_ENUM('m','o','v','s','s'),
	movsw_insn	= INSN_TO_ENUM('m','o','v','s','w'),
	movswl_insn	= INSN_TO_ENUM('m','o','v','s','w','l'),
	movswq_insn	= INSN_TO_ENUM('m','o','v','s','w','q'),
	movsx_insn	= INSN_TO_ENUM('m','o','v','s','x'),
	movw_insn	= INSN_TO_ENUM('m','o','v','w'),
	movzbl_insn	= INSN_TO_ENUM('m','o','v','z','b','l'),
	movzwl_insn	= INSN_TO_ENUM('m','o','v','z','w','l'),
	movzx_insn	= INSN_TO_ENUM('m','o','v','z','x'),
	mul_insn	= INSN_TO_ENUM('m','u','l'),
	mulsd_insn	= INSN_TO_ENUM('m','u','l','s','d'),
	mulss_insn	= INSN_TO_ENUM('m','u','l','s','s'),
	neg_insn	= INSN_TO_ENUM('n','e','g'),
	negq_insn	= INSN_TO_ENUM('n','e','g','q'),
	nop_insn	= INSN_TO_ENUM('n','o','p'),
	nopl_insn	= INSN_TO_ENUM('n','o','p','l'),
	nopw_insn	= INSN_TO_ENUM('n','o','p','w'),
	not_insn	= INSN_TO_ENUM('n','o','t'),
	or_insn		= INSN_TO_ENUM('o','r'),
	orb_insn	= INSN_TO_ENUM('o','r','b'),
	orl_insn	= INSN_TO_ENUM('o','r','l'),
	orw_insn	= INSN_TO_ENUM('o','r','w'),
	out_insn	= INSN_TO_ENUM('o','u','t'),
	outs_insn	= INSN_TO_ENUM('o','u','t','s'),
	pop_insn	= INSN_TO_ENUM('p','o','p'),
	popa_insn	= INSN_TO_ENUM('p','o','p','a'),
	popad_insn	= INSN_TO_ENUM('p','o','p','a','d'),
	popf_insn	= INSN_TO_ENUM('p','o','p','f'),
	popfd_insn	= INSN_TO_ENUM('p','o','p','f','d'),
	push_insn	= INSN_TO_ENUM('p','u','s','h'),
	pusha_insn	= INSN_TO_ENUM('p','u','s','h','a'),
	pushad_insn	= INSN_TO_ENUM('p','u','s','h','a','d'),
	pushf_insn	= INSN_TO_ENUM('p','u','s','h','f'),
	pushfd_insn	= INSN_TO_ENUM('p','u','s','h','f','d'),
	rcl_insn	= INSN_TO_ENUM('r','c','l'),
	rcr_insn	= INSN_TO_ENUM('r','c','r'),
	rdmsr_insn	= INSN_TO_ENUM('r','d','m','s','r'),
	rdpmc_insn	= INSN_TO_ENUM('r','d','p','m','c'),
	rdtsc_insn	= INSN_TO_ENUM('r','d','t','s','c'),
	rep_insn	= INSN_TO_ENUM('r','e','p'),
	repe_insn	= INSN_TO_ENUM('r','e','p','e'),
	repne_insn	= INSN_TO_ENUM('r','e','p','n','e'),
	repnz_insn	= INSN_TO_ENUM('r','e','p','n','z'),
	repz_insn	= INSN_TO_ENUM('r','e','p','z'),
	ret_insn	= INSN_TO_ENUM('r','e','t'),
	retf_insn	= INSN_TO_ENUM('r','e','t','f'),
	retn_insn	= INSN_TO_ENUM('r','e','t','n'),
	retq_insn	= INSN_TO_ENUM('r','e','t','q'),
	rol_insn	= INSN_TO_ENUM('r','o','l'),
	roll_insn	= INSN_TO_ENUM('r','o','l','l'),
	ror_insn	= INSN_TO_ENUM('r','o','r'),
	rsm_insn	= INSN_TO_ENUM('r','s','m'),
	sahf_insn	= INSN_TO_ENUM('s','a','h','f'),
	sal_insn	= INSN_TO_ENUM('s','a','l'),
	sar_insn	= INSN_TO_ENUM('s','a','r'),
	sbb_insn	= INSN_TO_ENUM('s','b','b'),
	scas_insn	= INSN_TO_ENUM('s','c','a','s'),
	scasb_insn	= INSN_TO_ENUM('s','c','a','s','b'),
	scasd_insn	= INSN_TO_ENUM('s','c','a','s','d'),
	scasw_insn	= INSN_TO_ENUM('s','c','a','s','w'),
	seta_insn	= INSN_TO_ENUM('s','e','t','a'),
	setae_insn	= INSN_TO_ENUM('s','e','t','a','e'),
	setb_insn	= INSN_TO_ENUM('s','e','t','b'),
	setbe_insn	= INSN_TO_ENUM('s','e','t','b','e'),
	setc_insn	= INSN_TO_ENUM('s','e','t','c'),
	sete_insn	= INSN_TO_ENUM('s','e','t','e'),
	setg_insn	= INSN_TO_ENUM('s','e','t','g'),
	setge_insn	= INSN_TO_ENUM('s','e','t','g','e'),
	setl_insn	= INSN_TO_ENUM('s','e','t','l'),
	setle_insn	= INSN_TO_ENUM('s','e','t','l','e'),
	setna_insn	= INSN_TO_ENUM('s','e','t','n','a'),
	setnae_insn	= INSN_TO_ENUM('s','e','t','n','a','e'),
	setnb_insn	= INSN_TO_ENUM('s','e','t','n','b'),
	setnbe_insn	= INSN_TO_ENUM('s','e','t','n','b','e'),
	setnc_insn	= INSN_TO_ENUM('s','e','t','n','c'),
	setne_insn	= INSN_TO_ENUM('s','e','t','n','e'),
	setng_insn	= INSN_TO_ENUM('s','e','t','n','g'),
	setnge_insn	= INSN_TO_ENUM('s','e','t','n','g','e'),
	setnl_insn	= INSN_TO_ENUM('s','e','t','n','l'),
	setnle_insn	= INSN_TO_ENUM('s','e','t','n','l','e'),
	setno_insn	= INSN_TO_ENUM('s','e','t','n','o'),
	setnp_insn	= INSN_TO_ENUM('s','e','t','n','p'),
	setns_insn	= INSN_TO_ENUM('s','e','t','n','s'),
	setnz_insn	= INSN_TO_ENUM('s','e','t','n','z'),
	seto_insn	= INSN_TO_ENUM('s','e','t','o'),
	setp_insn	= INSN_TO_ENUM('s','e','t','p'),
	setpe_insn	= INSN_TO_ENUM('s','e','t','p','e'),
	setpo_insn	= INSN_TO_ENUM('s','e','t','p','o'),
	sets_insn	= INSN_TO_ENUM('s','e','t','s'),
	setz_insn	= INSN_TO_ENUM('s','e','t','z'),
	sgdt_insn	= INSN_TO_ENUM('s','g','d','t'),
	shl_insn	= INSN_TO_ENUM('s','h','l'),
	shld_insn	= INSN_TO_ENUM('s','h','l','d'),
	shlq_insn	= INSN_TO_ENUM('s','h','l','q'),
	shr_insn	= INSN_TO_ENUM('s','h','r'),
	shrd_insn	= INSN_TO_ENUM('s','h','r','d'),
	sidt_insn	= INSN_TO_ENUM('s','i','d','t'),
	sldt_insn	= INSN_TO_ENUM('s','l','d','t'),
	smsw_insn	= INSN_TO_ENUM('s','m','s','w'),
	stc_insn	= INSN_TO_ENUM('s','t','(','c',')'),
	std_insn	= INSN_TO_ENUM('s','t','(','d',')'),
	sti_insn	= INSN_TO_ENUM('s','t','(','i',')'),
	stos_insn	= INSN_TO_ENUM('s','t','o','s'),
	stosb_insn	= INSN_TO_ENUM('s','t','o','s','b'),
	stosd_insn	= INSN_TO_ENUM('s','t','o','s','d'),
	stosw_insn	= INSN_TO_ENUM('s','t','o','s','w'),
	str_insn	= INSN_TO_ENUM('s','t','(','r',')'),
	sub_insn	= INSN_TO_ENUM('s','u','b'),
	subl_insn	= INSN_TO_ENUM('s','u','b','l'),
	subq_insn	= INSN_TO_ENUM('s','u','b','q'),
	subsd_insn	= INSN_TO_ENUM('s','u','b','s','d'),
	subss_insn	= INSN_TO_ENUM('s','u','b','s','s'),
	syscall_insn	= INSN_TO_ENUM('s','y','s','c','a','l','l'),
	sysenter_insn	= INSN_TO_ENUM('s','y','s','e','n','t','e','r'),
	sysret_insn	= INSN_TO_ENUM('s','y','s','r','e','t'),
	test_insn	= INSN_TO_ENUM('t','e','s','t'),
	testb_insn	= INSN_TO_ENUM('t','e','s','t','b'),
	testl_insn	= INSN_TO_ENUM('t','e','s','t','l'),
	ucomisd_insn	= INSN_TO_ENUM('u','c','o','m','i','s','d'),
	ucomiss_insn	= INSN_TO_ENUM('u','c','o','m','i','s','s'),
	verr_insn	= INSN_TO_ENUM('v','e','r','r'),
	verw_insn	= INSN_TO_ENUM('v','e','r','w'),
	wait_insn	= INSN_TO_ENUM('w','a','i','t'),
	wbinvd_insn	= INSN_TO_ENUM('w','b','i','n','v','d'),
	wrmsr_insn	= INSN_TO_ENUM('w','r','m','s','r'),
	xadd_insn	= INSN_TO_ENUM('x','a','d','d'),
	xchg_insn	= INSN_TO_ENUM('x','c','h','g'),
	xlat_insn	= INSN_TO_ENUM('x','l','a','t'),
	xor_insn	= INSN_TO_ENUM('x','o','r'),
	xorb_insn	= INSN_TO_ENUM('x','o','r','b'),
	xorpd_insn	= INSN_TO_ENUM('x','o','r','p','d')
};

/**
 * @enum macro_mnemonic
 * @brief Enumeration of macro mnemonics of x86-32 and x86-64.
 * @details A macro instruction is a mnemonic which is used in an instruction
 * with another mnemonic.
This enumeration works by assigning the integer value of registers
 * as that register's enum value. Since we only want to deal with a maximum
 * enum size of uint64_t, this implementation relies on the fact that all
 * registers can be uniquely identified by their first 8 characters. This
 * allows <b>libind</b> to store semantic information about instructions very
 * efficiently. We append _reg to each enum name to avoid clashes with C
 * defined symbols.
 */
enum insn_macro_mnemonic {
	rep	= rep_insn,
	repe	= repe_insn,
	repne	= repne_insn,
	repnz	= repnz_insn,
	repz	= repz_insn
};

/**
 * @enum insn_reg
 * @brief Enumeration of x86-32 and x86-64 registers.
 * @details This enumeration works by assigning the integer value of registers
 * as that register's enum value. Since we only want to deal with a maximum
 * enum size of uint64_t, this implementation relies on the fact that all
 * registers can be uniquely identified by their first 8 characters. This
 * allows <b>libind</b> to store semantic information about instructions very
 * efficiently. We append _reg to each enum name to avoid clashes with C
 * defined symbols.
 */
enum insn_reg {
	ah_reg		= INSN_TO_ENUM('%','a','h'),
	ah_paren_reg	= INSN_TO_ENUM('(','%','a','h',')'),
	al_reg		= INSN_TO_ENUM('%','a','l'),
	al_paren_reg	= INSN_TO_ENUM('(','%','a','l',')'),
	ax_reg		= INSN_TO_ENUM('%','a','x'),
	ax_paren_reg	= INSN_TO_ENUM('(','%','a','x',')'),
	bh_reg		= INSN_TO_ENUM('%','b','h'),
	bh_paren_reg	= INSN_TO_ENUM('(','%','b','h',')'),
	bl_reg		= INSN_TO_ENUM('%','b','l'),
	bl_paren_reg	= INSN_TO_ENUM('(','%','b','l',')'),
	bp_reg		= INSN_TO_ENUM('%','b','p'),
	bp_paren_reg	= INSN_TO_ENUM('(','%','b','p',')'),
	bpl_reg		= INSN_TO_ENUM('%','b','p','l'),
	bpl_paren_reg	= INSN_TO_ENUM('(','%','b','p','l',')'),
	bx_reg		= INSN_TO_ENUM('%','b','x'),
	bx_paren_reg	= INSN_TO_ENUM('(','%','b','x',')'),
	ch_reg		= INSN_TO_ENUM('%','c','h'),
	ch_paren_reg	= INSN_TO_ENUM('(','%','c','h',')'),
	cl_reg		= INSN_TO_ENUM('%','c','l'),
	cl_paren_reg	= INSN_TO_ENUM('(','%','c','l',')'),
	cx_reg		= INSN_TO_ENUM('%','c','x'),
	cx_paren_reg	= INSN_TO_ENUM('(','%','c','x',')'),
	di_reg		= INSN_TO_ENUM('%','d','i'),
	di_paren_reg	= INSN_TO_ENUM('(','%','d','i',')'),
	dil_reg		= INSN_TO_ENUM('%','d','i','l'),
	dil_paren_reg	= INSN_TO_ENUM('(','%','d','i','l',')'),
	dh_reg		= INSN_TO_ENUM('%','d','h'),
	dh_paren_reg	= INSN_TO_ENUM('(','%','d','h',')'),
	dl_reg		= INSN_TO_ENUM('%','d','l'),
	dl_paren_reg	= INSN_TO_ENUM('(','%','d','l',')'),
	dx_reg		= INSN_TO_ENUM('%','d','x'),
	dx_paren_reg	= INSN_TO_ENUM('(','%','d','x',')'),
	eax_reg		= INSN_TO_ENUM('%','e','a','x'),
	eax_paren_reg	= INSN_TO_ENUM('(','%','e','a','x',')'),
	ebp_reg		= INSN_TO_ENUM('%','e','b','p'),
	ebp_paren_reg	= INSN_TO_ENUM('(','%','e','b','p',')'),
	ebx_reg		= INSN_TO_ENUM('%','e','b','x'),
	ebx_paren_reg	= INSN_TO_ENUM('(','%','e','b','x',')'),
	ecx_reg		= INSN_TO_ENUM('%','e','c','x'),
	ecx_paren_reg	= INSN_TO_ENUM('(','%','e','c','x',')'),
	edi_reg		= INSN_TO_ENUM('%','e','d','i'),
	edi_paren_reg	= INSN_TO_ENUM('(','%','e','d','i',')'),
	edx_reg		= INSN_TO_ENUM('%','e','d','x'),
	edx_paren_reg	= INSN_TO_ENUM('(','%','e','d','x',')'),
	eip_reg		= INSN_TO_ENUM('%','e','i','p'),
	eip_paren_reg	= INSN_TO_ENUM('(','%','e','i','p',')'),
	esi_reg		= INSN_TO_ENUM('%','e','s','i'),
	esi_paren_reg	= INSN_TO_ENUM('(','%','e','s','i',')'),
	esp_reg		= INSN_TO_ENUM('%','e','s','p'),
	esp_paren_reg	= INSN_TO_ENUM('(','%','e','s','p',')'),
	r10_reg		= INSN_TO_ENUM('%','r','1','0'),
	r10_paren_reg	= INSN_TO_ENUM('(','%','r','1','0',')'),
	r10b_reg	= INSN_TO_ENUM('%','r','1','0','b'),
	r10b_paren_reg	= INSN_TO_ENUM('(','%','r','1','0','b',')'),
	r10d_reg	= INSN_TO_ENUM('%','r','1','0','d'),
	r10d_paren_reg	= INSN_TO_ENUM('(','%','r','1','0','d',')'),
	r10w_reg	= INSN_TO_ENUM('%','r','1','0','w'),
	r10w_paren_reg	= INSN_TO_ENUM('(','%','r','1','0','w',')'),
	r11_reg		= INSN_TO_ENUM('%','r','1','1'),
	r11_paren_reg	= INSN_TO_ENUM('(','%','r','1','1',')'),
	r11b_reg	= INSN_TO_ENUM('%','r','1','1','b'),
	r11b_paren_reg	= INSN_TO_ENUM('(','%','r','1','1','b',')'),
	r11d_reg	= INSN_TO_ENUM('%','r','1','1','d'),
	r11d_paren_reg	= INSN_TO_ENUM('(','%','r','1','1','d',')'),
	r11w_reg	= INSN_TO_ENUM('%','r','1','1','w'),
	r11w_paren_reg	= INSN_TO_ENUM('(','%','r','1','1','w',')'),
	r12_reg		= INSN_TO_ENUM('%','r','1','2'),
	r12_paren_reg	= INSN_TO_ENUM('(','%','r','1','2',')'),
	r12b_reg	= INSN_TO_ENUM('%','r','1','2','b'),
	r12b_paren_reg	= INSN_TO_ENUM('(','%','r','1','2','b',')'),
	r12d_reg	= INSN_TO_ENUM('%','r','1','2','d'),
	r12d_paren_reg	= INSN_TO_ENUM('(','%','r','1','2','d',')'),
	r12w_reg	= INSN_TO_ENUM('%','r','1','2','w'),
	r12w_paren_reg	= INSN_TO_ENUM('(','%','r','1','2','w',')'),
	r13_reg		= INSN_TO_ENUM('%','r','1','3'),
	r13_paren_reg	= INSN_TO_ENUM('(','%','r','1','3',')'),
	r13b_reg	= INSN_TO_ENUM('%','r','1','3','b'),
	r13b_paren_reg	= INSN_TO_ENUM('(','%','r','1','3','b',')'),
	r13d_reg	= INSN_TO_ENUM('%','r','1','3','d'),
	r13d_paren_reg	= INSN_TO_ENUM('(','%','r','1','3','d',')'),
	r13w_reg	= INSN_TO_ENUM('%','r','1','3','w'),
	r13w_paren_reg	= INSN_TO_ENUM('(','%','r','1','3','w',')'),
	r14_reg		= INSN_TO_ENUM('%','r','1','4'),
	r14_paren_reg	= INSN_TO_ENUM('(','%','r','1','4',')'),
	r14b_reg	= INSN_TO_ENUM('%','r','1','4','b'),
	r14b_paren_reg	= INSN_TO_ENUM('(','%','r','1','4','b',')'),
	r14d_reg	= INSN_TO_ENUM('%','r','1','4','d'),
	r14d_paren_reg	= INSN_TO_ENUM('(','%','r','1','4','d',')'),
	r14w_reg	= INSN_TO_ENUM('%','r','1','4','w'),
	r14w_paren_reg	= INSN_TO_ENUM('(','%','r','1','4','w',')'),
	r15_reg		= INSN_TO_ENUM('%','r','1','5'),
	r15_paren_reg	= INSN_TO_ENUM('(','%','r','1','5',')'),
	r15b_reg	= INSN_TO_ENUM('%','r','1','5','b'),
	r15b_paren_reg	= INSN_TO_ENUM('(','%','r','1','5','b',')'),
	r15d_reg	= INSN_TO_ENUM('%','r','1','5','d'),
	r15d_paren_reg	= INSN_TO_ENUM('(','%','r','1','5','d',')'),
	r15w_reg	= INSN_TO_ENUM('%','r','1','5','w'),
	r15w_paren_reg	= INSN_TO_ENUM('(','%','r','1','5','w',')'),
	r8_reg		= INSN_TO_ENUM('%','r','8'),
	r8_paren_reg	= INSN_TO_ENUM('(','%','r','8',')'),
	r8b_reg		= INSN_TO_ENUM('%','r','8','b'),
	r8b_paren_reg	= INSN_TO_ENUM('(','%','r','8','b',')'),
	r8d_reg		= INSN_TO_ENUM('%','r','8','d'),
	r8d_paren_reg	= INSN_TO_ENUM('(','%','r','8','d',')'),
	r8w_reg		= INSN_TO_ENUM('%','r','8','w'),
	r8w_paren_reg	= INSN_TO_ENUM('(','%','r','8','w',')'),
	r9_reg		= INSN_TO_ENUM('%','r','9'),
	r9_paren_reg	= INSN_TO_ENUM('(','%','r','9',')'),
	r9b_reg		= INSN_TO_ENUM('%','r','9','b'),
	r9b_paren_reg	= INSN_TO_ENUM('(','%','r','9','b',')'),
	r9d_reg		= INSN_TO_ENUM('%','r','9','d'),
	r9d_paren_reg	= INSN_TO_ENUM('(','%','r','9','d',')'),
	r9w_reg		= INSN_TO_ENUM('%','r','9','w'),
	r9w_paren_reg	= INSN_TO_ENUM('(','%','r','9','w',')'),
	rax_reg		= INSN_TO_ENUM('%','r','a','x'),
	rax_paren_reg	= INSN_TO_ENUM('(','%','r','a','x',')'),
	rbp_reg		= INSN_TO_ENUM('%','r','b','p'),
	rbp_paren_reg	= INSN_TO_ENUM('(','%','r','b','p',')'),
	rbx_reg		= INSN_TO_ENUM('%','r','b','x'),
	rbx_paren_reg	= INSN_TO_ENUM('(','%','r','b','x',')'),
	rcx_reg		= INSN_TO_ENUM('%','r','c','x'),
	rcx_paren_reg	= INSN_TO_ENUM('(','%','r','c','x',')'),
	rdi_reg		= INSN_TO_ENUM('%','r','d','i'),
	rdi_paren_reg	= INSN_TO_ENUM('(','%','r','d','i',')'),
	rdx_reg		= INSN_TO_ENUM('%','r','d','x'),
	rdx_paren_reg	= INSN_TO_ENUM('(','%','r','d','x',')'),
	rip_reg		= INSN_TO_ENUM('%','r','i','p'),
	rip_paren_reg	= INSN_TO_ENUM('(','%','r','i','p',')'),
	rsi_reg		= INSN_TO_ENUM('%','r','s','i'),
	rsi_paren_reg	= INSN_TO_ENUM('(','%','r','s','i',')'),
	rsp_reg		= INSN_TO_ENUM('%','r','s','p'),
	rsp_paren_reg	= INSN_TO_ENUM('(','%','r','s','p',')'),
	rx_reg		= INSN_TO_ENUM('%','r','x'),
	rx_paren_reg	= INSN_TO_ENUM('(','%','r','x',')'),
	si_reg		= INSN_TO_ENUM('%','s','i'),
	si_paren_reg	= INSN_TO_ENUM('(','%','s','i',')'),
	sil_reg		= INSN_TO_ENUM('%','s','i','l'),
	sil_paren_reg	= INSN_TO_ENUM('(','%','s','i','l',')'),
	st_reg		= INSN_TO_ENUM('%','s','t'),
	st_paren_reg	= INSN_TO_ENUM('(','%','s','t',')'),
	st0_reg		= INSN_TO_ENUM('%','s','t','(','0',')'),
	st0_paren_reg	= INSN_TO_ENUM('(','%','s','t','(','0',')',')'),
	st1_reg		= INSN_TO_ENUM('%','s','t','(','1',')'),
	st1_paren_reg	= INSN_TO_ENUM('(','%','s','t','(','1',')',')'),
	st2_reg		= INSN_TO_ENUM('%','s','t','(','2',')'),
	st2_paren_reg	= INSN_TO_ENUM('(','%','s','t','(','2',')',')'),
	st3_reg		= INSN_TO_ENUM('%','s','t','(','3',')'),
	st3_paren_reg	= INSN_TO_ENUM('(','%','s','t','(','3',')',')'),
	st4_reg		= INSN_TO_ENUM('%','s','t','(','4',')'),
	st4_paren_reg	= INSN_TO_ENUM('(','%','s','t','(','4',')',')'),
	st5_reg		= INSN_TO_ENUM('%','s','t','(','5',')'),
	st5_paren_reg	= INSN_TO_ENUM('(','%','s','t','(','5',')',')'),
	st6_reg		= INSN_TO_ENUM('%','s','t','(','6',')'),
	st6_paren_reg	= INSN_TO_ENUM('(','%','s','t','(','6',')',')'),
	st7_reg		= INSN_TO_ENUM('%','s','t','(','7',')'),
	st7_paren_reg	= INSN_TO_ENUM('(','%','s','t','(','7',')',')'),
	xmm0_reg	= INSN_TO_ENUM('%','x','m','m','0'),
	xmm0_paren_reg	= INSN_TO_ENUM('(','%','x','m','m','0',')'),
	xmm1_reg	= INSN_TO_ENUM('%','x','m','m','1'),
	xmm1_paren_reg	= INSN_TO_ENUM('(','%','x','m','m','1',')'),
	xmm2_reg	= INSN_TO_ENUM('%','x','m','m','2'),
	xmm2_paren_reg	= INSN_TO_ENUM('(','%','x','m','m','2',')'),
	xmm3_reg	= INSN_TO_ENUM('%','x','m','m','3'),
	xmm3_paren_reg	= INSN_TO_ENUM('(','%','x','m','m','3',')'),
	xmm4_reg	= INSN_TO_ENUM('%','x','m','m','4'),
	xmm4_paren_reg	= INSN_TO_ENUM('(','%','x','m','m','4',')'),
	xmm5_reg	= INSN_TO_ENUM('%','x','m','m','5'),
	xmm5_paren_reg	= INSN_TO_ENUM('(','%','x','m','m','5',')'),
	xmm6_reg	= INSN_TO_ENUM('%','x','m','m','6'),
	xmm6_paren_reg	= INSN_TO_ENUM('(','%','x','m','m','6',')'),
	xmm7_reg	= INSN_TO_ENUM('%','x','m','m','7'),
	xmm7_paren_reg	= INSN_TO_ENUM('(','%','x','m','m','7',')')
};

enum array_base_type {
	ARR_BASE_REG,
	ARR_BASE_PARTS
};

struct array_parts {
	enum insn_reg base_address;
	enum insn_reg counter;
	int	      array_member_size;
};

struct array_index {
	enum array_base_type tag;
	bool		     is_offset_valid;
	bool		     is_offset_negative;
	bfd_vma		     offset;

	union {
		enum insn_reg	   base_reg;
		struct array_parts parts;
	} arr_info;
};

enum operand_type {
	OP_VAL		 = 1,
	OP_IMM		 = 2,
	OP_REG		 = 3,
	OP_REG_PTR	 = 4,
	OP_INDEX	 = 5,
	OP_INDEX_PTR	 = 6,
	OP_INDEX_INTO_FS = 7,
	OP_INDEX_INTO_CS = 8,
	OP_INDEX_INTO_ES = 9,
	OP_INDEX_INTO_DS = 10
};

struct cs_index {
	uint64_t	   addr;
	struct array_index arr_index;
};

struct insn_operand {
	enum operand_type tag;
	union {
		bfd_vma		    val;
		uint64_t	    imm;
		enum insn_reg	    reg;
		enum insn_reg	    reg_ptr;
		struct array_index  arr_index;
		struct array_index  arr_index_ptr;
		uint64_t	    index_into_fs;
		struct cs_index	    index_into_cs;
		enum insn_reg	    index_into_es;
		enum insn_reg	    index_into_ds;
	} operand_info;
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
 */
extern bool is_mnemonic(char * str);

/**
 * @internal
 * @brief Returns whether the operand is of a type recognised by <b>libind</b>.
 * @param str The operand.
 * @return TRUE if str represents an operand recognised by <b>libind</b>. FALSE
 * otherwise.
 */
extern bool is_operand(char * str);

/**
 * @internal
 * @brief Returns whether the mnemonic is a macro mnemonic.
 * @param The mnemonic.
 * @return TRUE if str represents a mnemonic recognised by <b>libind</b> as a
 * macro mnemonic.
 */
extern bool is_macro_mnemonic(char * str);

/**
 * @internal
 * @brief Parses str as an instruction operand and fills in the insn_operand
 * structure appropriately.
 * @param op The insn_operand structure to be populated.
 * @param str The operand.
 */
extern void set_operand_info(struct insn_operand * op, char * str);

/**
 * @internal
 * @brief Prints an insn_mnemonic to a FILE.
 * @param stream An open FILE to be written to.
 * @param mnemonic The insn_mnemonic to be printed.
 */
extern void print_mnemonic_to_file(FILE * stream, enum insn_mnemonic mnemonic);

/**
 * @internal
 * @brief Prints an insn_operand to a FILE.
 * @param stream An open FILE to be written to.
 * @param op The insn_operand to be printed.
 */
extern void print_operand_to_file(FILE * stream, struct insn_operand * op);

/**
 * @internal
 * @brief Prints extra_info to a FILE.
 * @param stream An open FILE to be written to.
 * @param op The extra_info to be printed.
 */
extern void print_comment_to_file(FILE * stream, bfd_vma extra_info);

#ifdef __cplusplus
}
#endif

#endif
