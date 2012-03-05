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
	case movsl_insn:
	case movslq_insn:
	case movsq_insn:
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
	case scas_insn:
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
	case stos_insn:
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
	case ah_reg:
	case ah_paren_reg:
	case al_reg:
	case al_paren_reg:
	case ax_reg:
	case ax_paren_reg:
	case bh_reg:
	case bh_paren_reg:
	case bl_reg:
	case bl_paren_reg:
	case bp_reg:
	case bp_paren_reg:
	case bpl_reg:
	case bpl_paren_reg:
	case bx_reg:
	case bx_paren_reg:
	case ch_reg:
	case ch_paren_reg:
	case cl_reg:
	case cl_paren_reg:
	case cx_reg:
	case cx_paren_reg:
	case di_reg:
	case di_paren_reg:
	case dil_reg:
	case dil_paren_reg:
	case dh_reg:
	case dh_paren_reg:
	case dl_reg:
	case dl_paren_reg:
	case dx_reg:
	case dx_paren_reg:
	case eax_reg:
	case eax_paren_reg:
	case ebp_reg:
	case ebp_paren_reg:
	case ebx_reg:
	case ebx_paren_reg:
	case ecx_reg:
	case ecx_paren_reg:
	case edi_reg:
	case edi_paren_reg:
	case edx_reg:
	case edx_paren_reg:
	case eip_reg:
	case eip_paren_reg:
	case esi_reg:
	case esi_paren_reg:
	case esp_reg:
	case esp_paren_reg:
	case r10_reg:
	case r10_paren_reg:
	case r10b_reg:
	case r10b_paren_reg:
	case r10d_reg:
	case r10d_paren_reg:
	case r10w_reg:
	case r10w_paren_reg:
	case r11_reg:
	case r11_paren_reg:
	case r11b_reg:
	case r11b_paren_reg:
	case r11d_reg:
	case r11d_paren_reg:
	case r11w_reg:
	case r11w_paren_reg:
	case r12_reg:
	case r12_paren_reg:
	case r12b_reg:
	case r12b_paren_reg:
	case r12d_reg:
	case r12d_paren_reg:
	case r12w_reg:
	case r12w_paren_reg:
	case r13_reg:
	case r13_paren_reg:
	case r13b_reg:
	case r13b_paren_reg:
	case r13d_reg:
	case r13d_paren_reg:
	case r13w_reg:
	case r13w_paren_reg:
	case r14_reg:
	case r14_paren_reg:
	case r14b_reg:
	case r14b_paren_reg:
	case r14d_reg:
	case r14d_paren_reg:
	case r14w_reg:
	case r14w_paren_reg:
	case r15_reg:
	case r15_paren_reg:
	case r15b_reg:
	case r15b_paren_reg:
	case r15d_reg:
	case r15d_paren_reg:
	case r15w_reg:
	case r15w_paren_reg:
	case r8_reg:
	case r8_paren_reg:
	case r8b_reg:
	case r8b_paren_reg:
	case r8d_reg:
	case r8d_paren_reg:
	case r8w_reg:
	case r8w_paren_reg:
	case r9_reg:
	case r9_paren_reg:
	case r9b_reg:
	case r9b_paren_reg:
	case r9d_reg:
	case r9d_paren_reg:
	case r9w_reg:
	case r9w_paren_reg:
	case rax_reg:
	case rax_paren_reg:
	case rbp_reg:
	case rbp_paren_reg:
	case rbx_reg:
	case rbx_paren_reg:
	case rcx_reg:
	case rcx_paren_reg:
	case rdi_reg:
	case rdi_paren_reg:
	case rdx_reg:
	case rdx_paren_reg:
	case rsi_reg:
	case rsi_paren_reg:
	case rsp_reg:
	case rsp_paren_reg:
	case rx_reg:
	case rx_paren_reg:
	case si_reg:
	case si_paren_reg:
	case sil_reg:
	case sil_paren_reg:
	case st_reg:
	case st_paren_reg:
	case st0_reg:
	case st0_paren_reg:
	case st1_reg:
	case st1_paren_reg:
	case st2_reg:
	case st2_paren_reg:
	case st3_reg:
	case st3_paren_reg:
	case st4_reg:
	case st4_paren_reg:
	case st5_reg:
	case st5_paren_reg:
	case st6_reg:
	case st6_paren_reg:
	case st7_reg:
	case st7_paren_reg:
	case xmm0_reg:
	case xmm0_paren_reg:
	case xmm1_reg:
	case xmm1_paren_reg:
	case xmm2_reg:
	case xmm2_paren_reg:
	case xmm3_reg:
	case xmm3_paren_reg:
	case xmm4_reg:
	case xmm4_paren_reg:
	case xmm5_reg:
	case xmm5_paren_reg:
	case xmm6_reg:
	case xmm6_paren_reg:
	case xmm7_reg:
	case xmm7_paren_reg:
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

bool is_immediate(char * str)
{
	return str[0] == '$' && is_address(str + 1);
}

bool is_absolute(char * str)
{
	return str[0] == '*' && (is_address(str + 1) || is_reg(str + 1));
}

bool is_val(char * str)
{
	return is_address(str) || (str[0] == '-' && is_address(str + 1));
}

bool is_index(char * str)
{
	if(str[0] == '(') {
		int len = strlen(str);

		if(str[len - 1] == ')') {
			char * comma = strchr(str, ',');
			char * comma2;

			if(comma == NULL) {
				return FALSE;
			}

			comma2 = strchr(comma + 1, ',');

			if(comma2 == NULL) {
				return FALSE;
			} else {
				char reg[comma - str];
				char reg2[comma2 - comma];

				strncpy(reg, str + 1, comma - str - 1);
				reg[comma - str - 1] = '\0';

				if(!is_reg(reg)) {
					return FALSE;
				}

				strncpy(reg2, comma + 1, comma2 - comma - 1);
				reg2[comma2 - comma - 1] = '\0';

				if(!is_reg(reg2)) {
					return FALSE;
				}

				return *(comma2 + 1) == '1' ||
						*(comma2 + 1) == '2' ||
						*(comma2 + 1) == '4' ||
						*(comma2 + 1) == '8';
			}
		}
	}

	return FALSE;
}

bool is_index_ptr(char * str)
{
	if(str[0] == '*') {
		return is_index(str + 1);
	}

	return FALSE;
}

bool is_index_into_fs(char * str)
{
	if(strlen(str) < 5) {
		return FALSE;
	} else {
		uint32_t val = '%' | 'f' << 8 | 's' << 16 | ':' << 24;
		if(val == *(uint32_t *)str) {
			return is_address(str + 4);
		}
	}

	return FALSE;
}

bool is_index_into_cs(char * str)
{
	int len = strlen(str);

	if(len < 5) {
		return FALSE;
	} else {
		uint32_t val = '%' | 'c' << 8 | 's' << 16 | ':' << 24;
		if(val == *(uint32_t *)str) {
			char tmp[len-4];
			char * bracket;

			strcpy(tmp, str + 4);
			bracket = strchr(tmp, '(');

			if(bracket != NULL && is_index(bracket)) {
				bracket[0] = '\0';
				return is_address(tmp);
			}
		}
	}

	return FALSE;
}

bool is_index_into_es(char * str)
{
	int len = strlen(str);

	if(len < 5) {
		return FALSE;
	} else {
		uint32_t val = '%' | 'e' << 8 | 's' << 16 | ':' << 24;
		if(val == *(uint32_t *)str) {
			return is_reg(str + 4);
		}
	}

	return FALSE;
}

bool is_index_into_ds(char * str)
{
	int len = strlen(str);

	if(len < 5) {
		return FALSE;
	} else {
		uint32_t val = '%' | 'd' << 8 | 's' << 16 | ':' << 24;
		if(val == *(uint32_t *)str) {
			return is_reg(str + 4);
		}
	}

	return FALSE;
}

bool is_operand(char * str)
{
	return is_val(str) || is_reg(str) || is_immediate(str) ||
			is_absolute(str) || is_index(str) ||
			is_index_ptr(str) ||is_index_into_fs(str) ||
			is_index_into_cs(str) || is_index_into_es(str) ||
			is_index_into_ds(str);
}

bool is_macro_mnemonic(char * str)
{
	uint64_t mnemonic = 0;
	strncpy((char *)&mnemonic, str, sizeof(uint64_t));
	
	switch(mnemonic) {
	case rep:
	case repe:
	case repne:
	case repnz:
	case repz:
		return TRUE;
	default:
		return FALSE;
	}
}

void set_operand_info(struct insn_operand * op, char * str)
{
	if(is_val(str)) {
		op->tag = OP_VAL;

		if(str[0] == '-') {
			op->operand_info.val = 0 - get_vma_target(str + 1);
		} else {
			op->operand_info.val = get_vma_target(str);
		}
	} else if(is_immediate(str)) {
		op->tag		     = OP_IMM;
		op->operand_info.imm = get_vma_target(str + 1);
	} else if(is_reg(str)) {
		op->tag = OP_REG;
		op->operand_info.reg = 0;
		strncpy((char *)&op->operand_info.reg, str, sizeof(uint64_t));
	} else if(is_absolute(str)) {
		op->tag = OP_ABS_PTR;

		if(is_address(str + 1)) {
			op->operand_info.abs_ptr.tag	       = ABS_PTR_ADDR;
			op->operand_info.abs_ptr.ptr_info.addr =
					get_vma_target(str + 1);
		} else {
			op->operand_info.abs_ptr.tag	      = ABS_PTR_REG;
			op->operand_info.abs_ptr.ptr_info.reg = 0;
			strncpy((char *)&op->operand_info.abs_ptr.ptr_info.reg,
					str, sizeof(uint64_t));
		}
	} else if(is_index(str)) {
		char tmp[strlen(str) + 1];
		strcpy(tmp, str);

		op->tag = OP_INDEX;
		op->operand_info.arr_index.array_member_size =
				strrchr(tmp, ',')[1] - '0';

		strrchr(tmp, ',')[0]		   = '\0';
		op->operand_info.arr_index.counter = 0;
		strncpy((char *)&op->operand_info.arr_index.counter,
				strchr(tmp, ',') + 1, sizeof(uint64_t));

		strchr(tmp, ',')[0]			= 0;
		op->operand_info.arr_index.base_address = 0;
		strncpy((char *)&op->operand_info.arr_index.base_address,
				strchr(tmp, '(') + 1, sizeof(uint64_t));
	} else if(is_index_ptr(str)) {
		struct array_index arr_index;
		set_operand_info(op, str + 1);

		op->tag			       = OP_INDEX_PTR;
		arr_index		       = op->operand_info.arr_index;
		op->operand_info.arr_index_ptr = arr_index;
	} else if(is_index_into_fs(str)) {
		op->tag			       = OP_INDEX_INTO_FS;
		op->operand_info.index_into_fs = get_vma_target(str + 4);
	} else if(is_index_into_cs(str)) {
		struct array_index arr_index;
		char tmp[strlen(str) + 1];

		set_operand_info(op, strchr(str, '('));
		op->tag					 = OP_INDEX_INTO_CS;
		arr_index				 =
				op->operand_info.arr_index;
		op->operand_info.index_into_cs.arr_index = arr_index;

		strcpy(tmp, str);
		strchr(tmp, '(')[0]		    = '\0';
		op->operand_info.index_into_cs.addr = get_vma_target(str + 4);
	} else if(is_index_into_es(str)) {
		op->tag			       = OP_INDEX_INTO_ES;
		op->operand_info.index_into_es = 0;

		strncpy((char *)&op->operand_info.index_into_es,
					str, sizeof(uint64_t));
	} else if(is_index_into_ds(str)) {
		op->tag			       = OP_INDEX_INTO_DS;
		op->operand_info.index_into_ds = 0;

		strncpy((char *)&op->operand_info.index_into_ds,
					str, sizeof(uint64_t));
	}
}
