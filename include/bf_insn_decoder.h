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

#include "binary_file.h"

/**
 * @enum insn_mnemonic
 * @brief Enumeration of x86-32 and x86-64 mnemonics. This enumeration does not
 * include FPU or SIMD instructions such as SSE, MMX, etc.
 * @details This enumeration works by assigning the integer value of mnemonics
 * as that mnemonic's enum value. This allows <b>libind</b> to store semantic
 * information about instructions very efficiently.
 */
/*enum insn_mnemonic {
	AAA
	AAD
	AAM
	AAS
	ADC
	ADD
	AND
	CALL
	CBW
	CLC
	CLD
	CLI
	CMC
	CMP
	CMPSB
	CMPSW
	CWD
	DAA
	DAS
	DEC
	DIV
	ESC
	HLT
	IDIV
	IMUL
	IN
	INC
	INT
	INTO
	IRET
	JA
	JAE
	JB
	JBE
	JC
	JCXZ
	JE
	JG
	JGE
	JL
	JLE
	JNA
	JNAE
	JNB
	JNBE
	JNC
	JNE
	JNG
	JNGE
	JNL
	JNLE
	JNO
	JNP
	JNS
	JNZ
	JO
	JP
	JPE
	JPO
	JS
	JZ
	JMP
	LAHF
	LDS
	LEA
	LES
	LOCK
	LODSB
	LODSW
	LOOP
	LOOPE
	LOOPNE
	LOOPNZ
	LOOPZ
	MOV
	MOVSB
	MOVSW
	MUL
	NEG
	NOP
	NOT
	OR
	OUT
	POP
	POPF
	PUSH
	PUSHF
	RCL
	RCR
	REP
	REPE
	REPNE
	REPNZ
	REPZ
	RET
	RETN
	RETF
	ROL
	ROR
	SAHF
	SAL
	SAR
	SBB
	SCASB
	SCASW
	SHL
	SHR
	STC
	STD
	STI
	STOSB
	STOSW
	SUB
	TEST
	WAIT
	XCHG
	XLAT
	XOR

	BOUND
	ENTER
	INS
	LEAVE
	OUTS
	POPA
	PUSHA

	ARPL
	CLTS
	LAR
	LGDT
	LIDT
	LLDT
	LMSW
	LOADALL
	LSL
	LTR
	SGDT
	SIDT
	SLDT
	SMSW
	STR
	VERR
	VERW

	BSF
	BSR
	BT
	BTC
	BTR
	BTS
	CDQ
	CMPSD
	CWDE
	INSB
	INSW
	INSD
	IRETD
	IRET
	JCXZ
	JECXZ
	LFS
	LGS
	LSS
	LODSD
	LOOPW
	LOOPD
	LOOPEW
	LOOPED
	LOOPZW
	LOOPZD
	LOOPNEW
	LOOPNED
	LOOPNZW
	LOOPNZD
	MOVSW
	MOVSD
	MOVSX
	MOVZX
	POPAD
	POPFD
	PUSHAD
	PUSHFD
	SCASD
	SETA
	SETAE
	SETB
	SETBE
	SETC
	SETE
	SETG
	SETGE
	SETL
	SETLE
	SETNA
	SETNAE
	SETNB
	SETNBE
	SETNC
	SETNE
	SETNG
	SETNGE
	SETNL
	SETNLE
	SETNO
	SETNP
	SETNS
	SETNZ
	SETO
	SETP
	SETPE
	SETPO
	SETS
	SETZ
	SHLD
	SHRD
	STOSB
	STOSW
	STOSD

	BSWAP
	CMPXCHG
	INVD
	INVLPG
	WBINVD
	XADD

	CPUID
	RDMSR
	RDTSC
	WRMSR
	RSM

	RDPMC

	SYSCALL
	SYSRET
	CMOVA
	CMOVAE
	CMOVB
	CMOVBE
	CMOVC
	CMOVE
	CMOVG
	CMOVGE
	CMOVL
	CMOVLE
	CMOVNA
	CMOVNAE
	CMOVNB
	CMOVNBE
	CMOVNC
	CMOVNE
	CMOVNG
	CMOVNGE
	CMOVNL
	CMOVNLE
	CMOVNO
	CMOVNP
	CMOVNS
	CMOVNZ
	CMOVO
	CMOVP
	CMOVPE
	CMOVPO
	CMOVS
	CMOVZ
	SYSENTER
};*/

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

#ifdef __cplusplus
}
#endif

#endif
