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
			!strncmp("iret", str, 4) || !strncmp("sysexit", str, 7) ||
			!strncmp("sysret", str, 6));
}

bfd_vma get_vma_target(char * str)
{
	bfd_vma vma = 0;
	sscanf(str, "0x%lX", &vma);
	return vma;
}

bool is_mnemonic(char * str)
{
	uint64_t insn = 0;
	strncpy((char *)&insn, str, sizeof(uint64_t));

	switch(insn) {
	case aaa_insn:
	case aad_insn:
	case aam_insn:
	case aas_insn:
	case adc_insn:
	case adcl_insn:
	case add_insn:
	case addb_insn:
	case addl_insn:
	case addq_insn:
	case addsd_insn:
	case addss_insn:
	case and_insn:
	case andb_insn:
	case andl_insn:
	case arpl_insn:
	case bound_insn:
	case bsf_insn:
	case bsr_insn:
	case bswap_insn:
	case bt_insn:
	case btc_insn:
	case btr_insn:
	case bts_insn:
	case call_insn:
	case callq_insn:
	case cbw_insn:
	case cdq_insn:
	case clc_insn:
	case cld_insn:
	case cli_insn:
	case cltq_insn:
	case clts_insn:
	case cmc_insn:
	case cmova_insn:
	case cmovae_insn:
	case cmovb_insn:
	case cmovbe_insn:
	case cmovc_insn:
	case cmove_insn:
	case cmovg_insn:
	case cmovge_insn:
	case cmovl_insn:
	case cmovle_insn:
	case cmovna_insn:
	case cmovnae_insn:
	case cmovnb_insn:
	case cmovnbe_insn:
	case cmovnc_insn:
	case cmovne_insn:
	case cmovng_insn:
	case cmovnge_insn:
	case cmovnl_insn:
	case cmovnle_insn:
	case cmovno_insn:
	case cmovnp_insn:
	case cmovns_insn:
	case cmovnz_insn:
	case cmovo_insn:
	case cmovp_insn:
	case cmovpe_insn:
	case cmovpo_insn:
	case cmovs_insn:
	case cmovz_insn:
	case cmp_insn:
	case cmpb_insn:
	case cmpl_insn:
	case cmpq_insn:
	case cmpsb_insn:
	case cmpsd_insn:
	case cmpsw_insn:
	case cmpw_insn:
	case cmpxchg_insn:
	case cpuid_insn:
	case cvtsi2sd_insn:
	case cvtsi2ss_insn:
	case cvttsd2si_insn:
	case cvttss2si_insn:
	case cwd_insn:
	case cwde_insn:
	case cwtl_insn:
	case daa_insn:
	case das_insn:
	case dec_insn:
	case div_insn:
	case divq_insn:
	case divsd_insn:
	case divss_insn:
	case enter_insn:
	case esc_insn:
	case fadd_insn:
	case fadds_insn:
	case fchs_insn:
	case fdivp_insn:
	case fdivrp_insn:
	case fildll_insn:
	case fistpll_insn:
	case fld_insn:
	case fld1_insn:
	case fldcw_insn:
	case flds_insn:
	case fldt_insn:
	case fldz_insn:
	case fmul_insn:
	case fmulp_insn:
	case fnstcw_insn:
	case fnstsw_insn:
	case fstp_insn:
	case fstpt_insn:
	case fsub_insn:
	case fucomi_insn:
	case fucomip_insn:
	case fxam_insn:
	case fxch_insn:
	case hlt_insn:
	case idiv_insn:
	case idivl_insn:
	case imul_insn:
	case in_insn:
	case inc_insn:
	case ins_insn:
	case insb_insn:
	case insd_insn:
	case insw_insn:
	case int_insn:
	case into_insn:
	case invd_insn:
	case invlpg_insn:
	case iret_insn:
	case iretd_insn:
	case ja_insn:
	case jae_insn:
	case jb_insn:
	case jbe_insn:
	case jc_insn:
	case jcxz_insn:
	case je_insn:
	case jecxz_insn:
	case jg_insn:
	case jge_insn:
	case jl_insn:
	case jle_insn:
	case jmp_insn:
	case jmpq_insn:
	case jna_insn:
	case jnae_insn:
	case jnb_insn:
	case jnbe_insn:
	case jnc_insn:
	case jne_insn:
	case jng_insn:
	case jnge_insn:
	case jnl_insn:
	case jnle_insn:
	case jno_insn:
	case jnp_insn:
	case jns_insn:
	case jnz_insn:
	case jo_insn:
	case jp_insn:
	case jpe_insn:
	case jpo_insn:
	case js_insn:
	case jz_insn:
	case lahf_insn:
	case lar_insn:
	case lds_insn:
	case lea_insn:
	case leave_insn:
	case leaveq_insn:
	case les_insn:
	case lfs_insn:
	case lgdt_insn:
	case lgs_insn:
	case lidt_insn:
	case lldt_insn:
	case lmsw_insn:
	case loadall_insn:
	case lock_insn:
	case lodsb_insn:
	case lodsd_insn:
	case lodsw_insn:
	case loop_insn:
	case loopd_insn:
	case loope_insn:
	case looped_insn:
	case loopew_insn:
	case loopne_insn:
	case loopned_insn:
	case loopnew_insn:
	case loopnz_insn:
	case loopnzd_insn:
	case loopnzw_insn:
	case loopw_insn:
	case loopz_insn:
	case loopzd_insn:
	case loopzw_insn:
	case lsl_insn:
	case lss_insn:
	case ltr_insn:
	case maxsd_insn:
	case mov_insn:
	case movabs_insn:
	case movapd_insn:
	case movaps_insn:
	case movb_insn:
	case movl_insn:
	case movq_insn:
	case movsb_insn:
	case movsbl_insn:
	case movsbq_insn:
	case movsbw_insn:
	case movsd_insn:
	case movslq_insn:
	case movss_insn:
	case movsw_insn:
	case movswl_insn:
	case movswq_insn:
	case movsx_insn:
	case movw_insn:
	case movzbl_insn:
	case movzwl_insn:
	case movzx_insn:
	case mul_insn:
	case mulsd_insn:
	case mulss_insn:
	case neg_insn:
	case negq_insn:
	case nop_insn:
	case nopl_insn:
	case nopw_insn:
	case not_insn:
	case or_insn:
	case orb_insn:
	case orl_insn:
	case orw_insn:
	case out_insn:
	case outs_insn:
	case pop_insn:
	case popa_insn:
	case popad_insn:
	case popf_insn:
	case popfd_insn:
	case push_insn:
	case pusha_insn:
	case pushad_insn:
	case pushf_insn:
	case pushfd_insn:
	case rcl_insn:
	case rcr_insn:
	case rdmsr_insn:
	case rdpmc_insn:
	case rdtsc_insn:
	case rep_insn:
	case repe_insn:
	case repne_insn:
	case repnz_insn:
	case repz_insn:
	case ret_insn:
	case retf_insn:
	case retn_insn:
	case retq_insn:
	case rol_insn:
	case roll_insn:
	case ror_insn:
	case rsm_insn:
	case sahf_insn:
	case sal_insn:
	case sar_insn:
	case sbb_insn:
	case scasb_insn:
	case scasd_insn:
	case scasw_insn:
	case seta_insn:
	case setae_insn:
	case setb_insn:
	case setbe_insn:
	case setc_insn:
	case sete_insn:
	case setg_insn:
	case setge_insn:
	case setl_insn:
	case setle_insn:
	case setna_insn:
	case setnae_insn:
	case setnb_insn:
	case setnbe_insn:
	case setnc_insn:
	case setne_insn:
	case setng_insn:
	case setnge_insn:
	case setnl_insn:
	case setnle_insn:
	case setno_insn:
	case setnp_insn:
	case setns_insn:
	case setnz_insn:
	case seto_insn:
	case setp_insn:
	case setpe_insn:
	case setpo_insn:
	case sets_insn:
	case setz_insn:
	case sgdt_insn:
	case shl_insn:
	case shld_insn:
	case shlq_insn:
	case shr_insn:
	case shrd_insn:
	case sidt_insn:
	case sldt_insn:
	case smsw_insn:
	case stc_insn:
	case std_insn:
	case sti_insn:
	case stosb_insn:
	case stosd_insn:
	case stosw_insn:
	case str_insn:
	case sub_insn:
	case subl_insn:
	case subq_insn:
	case subsd_insn:
	case subss_insn:
	case syscall_insn:
	case sysenter_insn:
	case sysret_insn:
	case test_insn:
	case testb_insn:
	case testl_insn:
	case ucomisd_insn:
	case ucomiss_insn:
	case verr_insn:
	case verw_insn:
	case wait_insn:
	case wbinvd_insn:
	case wrmsr_insn:
	case xadd_insn:
	case xchg_insn:
	case xlat_insn:
	case xor_insn:
	case xorb_insn:
	case xorpd_insn:
		return TRUE;
	default:
		return FALSE;
	}
}

static bool is_reg(char * str)
{
	uint64_t reg = 0;
	strncpy((char *)&reg, str, sizeof(uint64_t));

	switch(reg) {
	case eax_reg:
	case ebx_reg:
	case ecx_reg:
	case edx_reg:
	case edi_reg:
	case esi_reg:
	case ebp_reg:
	case esp_reg:
	case eip_reg:
	case rax_reg:
	case rbx_reg:
	case rcx_reg:
	case rdx_reg:
	case rdi_reg:
	case rsi_reg:
	case rdp_reg:
	case rsp_reg:
		return TRUE;
	default:
		return FALSE;
	}
}

bool is_address(char * str)
{
	bfd_vma vma = 0;
	return sscanf(str, "0x%lX", &vma) == 1;
}

bool is_operand(char * str)
{
	return is_address(str) || is_reg(str);
}
