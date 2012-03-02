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

#define INSN_TO_ENUM(c...) A(0, c, 0, 0, 0, 0, 0, 0, 0)

#include "binary_file.h"
#include "inttypes.h"

/**
 * @enum insn_mnemonic
 * @brief Enumeration of x86-32 and x86-64 mnemonics. This enumeration does not
 * include FPU or SIMD instructions such as SSE, MMX, etc.
 * @details This enumeration works by assigning the integer value of mnemonics
 * as that mnemonic's enum value. This allows <b>libind</b> to store semantic
 * information about instructions very efficiently.
 */
/*enum insn_mnemonic {
	AAA		= INSN_TO_ENUM('A','A','A'),
	AAD		= INSN_TO_ENUM('A','A','D'),
	AAM		= INSN_TO_ENUM('A','A','M'),
	AAS		= INSN_TO_ENUM('A','A','S'),
	ADC		= INSN_TO_ENUM('A','D','C'),
	ADD		= INSN_TO_ENUM('A','D','D'),
	AND		= INSN_TO_ENUM('A','N','D'),
	CALL		= INSN_TO_ENUM('C','A','L','L'),
	CBW		= INSN_TO_ENUM('C','B','W'),
	CLC		= INSN_TO_ENUM('C','L','C'),
	CLD		= INSN_TO_ENUM('C','L','D'),
	CLI		= INSN_TO_ENUM('C','L','I'),
	CMC		= INSN_TO_ENUM('C','M','C'),
	CMP		= INSN_TO_ENUM('C','M','P'),
	CMPSB		= INSN_TO_ENUM('C','M','P','S','B'),
	CMPSW		= INSN_TO_ENUM('C','M','P','S','W'),
	CWD		= INSN_TO_ENUM('C','W','D'),
	DAA		= INSN_TO_ENUM('D','A','A'),
	DAS		= INSN_TO_ENUM('D','A','S'),
	DEC		= INSN_TO_ENUM('D','E','C'),
	DIV		= INSN_TO_ENUM('D','I','V'),
	ESC		= INSN_TO_ENUM('E','S','C'),
	HLT		= INSN_TO_ENUM('H','L','T'),
	IDIV		= INSN_TO_ENUM('I','D','I','V'),
	IMUL		= INSN_TO_ENUM('I','M','U','L'),
	IN		= INSN_TO_ENUM('I','N'),
	INC		= INSN_TO_ENUM('I','N','C'),
	INT		= INSN_TO_ENUM('I','N','T'),
	INTO		= INSN_TO_ENUM('I','N','T','O'),
	IRET		= INSN_TO_ENUM('I','R','E','T'),
	JA		= INSN_TO_ENUM('J','A'),
	JAE		= INSN_TO_ENUM('J','A','E'),
	JB		= INSN_TO_ENUM('J','B'),
	JBE		= INSN_TO_ENUM('J','B','E'),
	JC		= INSN_TO_ENUM('J','C'),
	JCXZ		= INSN_TO_ENUM('J','C','X','Z'),
	JE		= INSN_TO_ENUM('J','E'),
	JG		= INSN_TO_ENUM('J','G'),
	JGE		= INSN_TO_ENUM('J','G','E'),
	JL		= INSN_TO_ENUM('J','L'),
	JLE		= INSN_TO_ENUM('J','L','E'),
	JNA		= INSN_TO_ENUM('J','N','A'),
	JNAE		= INSN_TO_ENUM('J','N','A','E'),
	JNB		= INSN_TO_ENUM('J','N','B'),
	JNBE		= INSN_TO_ENUM('J','N','B','E'),
	JNC		= INSN_TO_ENUM('J','N','C'),
	JNE		= INSN_TO_ENUM('J','N','E'),
	JNG		= INSN_TO_ENUM('J','N','G'),
	JNGE		= INSN_TO_ENUM('J','N','G','E'),
	JNL		= INSN_TO_ENUM('J','N','L'),
	JNLE		= INSN_TO_ENUM('J','N','L','E'),
	JNO		= INSN_TO_ENUM('J','N','O'),
	JNP		= INSN_TO_ENUM('J','N','P'),
	JNS		= INSN_TO_ENUM('J','N','S'),
	JNZ		= INSN_TO_ENUM('J','N','Z'),
	JO		= INSN_TO_ENUM('J','O'),
	JP		= INSN_TO_ENUM('J','P'),
	JPE		= INSN_TO_ENUM('J','P','E'),
	JPO		= INSN_TO_ENUM('J','P','O'),
	JS		= INSN_TO_ENUM('J','S'),
	JZ		= INSN_TO_ENUM('J','Z'),
	JMP		= INSN_TO_ENUM('J','M','P'),
	LAHF		= INSN_TO_ENUM('L','A','H','F'),
	LDS		= INSN_TO_ENUM('L','D','S'),
	LEA		= INSN_TO_ENUM('L','E','A'),
	LES		= INSN_TO_ENUM('L','E','S'),
	LOCK		= INSN_TO_ENUM('L','O','C','K'),
	LODSB		= INSN_TO_ENUM('L','O','D','S','B'),
	LODSW		= INSN_TO_ENUM('L','O','D','S','W'),
	LOOP		= INSN_TO_ENUM('L','O','O','P'),
	LOOPE		= INSN_TO_ENUM('L','O','O','P','E'),
	LOOPNE		= INSN_TO_ENUM('L','O','O','P','N','E'),
	LOOPNZ		= INSN_TO_ENUM('L','O','O','P','N','Z'),
	LOOPZ		= INSN_TO_ENUM('L','O','O','P','Z'),
	MOV		= INSN_TO_ENUM('M','O','V'),
	MOVSB		= INSN_TO_ENUM('M','O','V','S','B'),
	MOVSW		= INSN_TO_ENUM('M','O','V','S','W'),
	MUL		= INSN_TO_ENUM('M','U','L'),
	NEG		= INSN_TO_ENUM('N','E','G'),
	NOP		= INSN_TO_ENUM('N','O','P'),
	NOT		= INSN_TO_ENUM('N','O','T'),
	OR		= INSN_TO_ENUM('O','R'),
	OUT		= INSN_TO_ENUM('O','U','T'),
	POP		= INSN_TO_ENUM('P','O','P'),
	POPF		= INSN_TO_ENUM('P','O','P','F'),
	PUSH		= INSN_TO_ENUM('P','U','S','H'),
	PUSHF		= INSN_TO_ENUM('P','U','S','H','F'),
	RCL		= INSN_TO_ENUM('R','C','L'),
	RCR		= INSN_TO_ENUM('R','C','R'),
	REP		= INSN_TO_ENUM('R','E','P'),
	REPE		= INSN_TO_ENUM('R','E','P','E'),
	REPNE		= INSN_TO_ENUM('R','E','P','N','E'),
	REPNZ		= INSN_TO_ENUM('R','E','P','N','Z'),
	REPZ		= INSN_TO_ENUM('R','E','P','Z'),
	RET		= INSN_TO_ENUM('R','E','T'),
	RETN		= INSN_TO_ENUM('R','E','T','N'),
	RETF		= INSN_TO_ENUM('R','E','T','F'),
	ROL		= INSN_TO_ENUM('R','O','L'),
	ROR		= INSN_TO_ENUM('R','O','R'),
	SAHF		= INSN_TO_ENUM('S','A','H','F'),
	SAL		= INSN_TO_ENUM('S','A','L'),
	SAR		= INSN_TO_ENUM('S','A','R'),
	SBB		= INSN_TO_ENUM('S','B','B'),
	SCASB		= INSN_TO_ENUM('S','C','A','S','B'),
	SCASW		= INSN_TO_ENUM('S','C','A','S','W'),
	SHL		= INSN_TO_ENUM('S','H','L'),
	SHR		= INSN_TO_ENUM('S','H','R'),
	STC		= INSN_TO_ENUM('S','T','C'),
	STD		= INSN_TO_ENUM('S','T','D'),
	STI		= INSN_TO_ENUM('S','T','I'),
	STOSB		= INSN_TO_ENUM('S','T','O','S','B'),
	STOSW		= INSN_TO_ENUM('S','T','O','S','W'),
	SUB		= INSN_TO_ENUM('S','U','B'),
	TEST		= INSN_TO_ENUM('T','E','S','T'),
	WAIT		= INSN_TO_ENUM('W','A','I','T'),
	XCHG		= INSN_TO_ENUM('X','C','H','G'),
	XLAT		= INSN_TO_ENUM('X','L','A','T'),
	XOR		= INSN_TO_ENUM('X','O','R'),
	BOUND		= INSN_TO_ENUM('B','O','U','N','D'),
	ENTER		= INSN_TO_ENUM('E','N','T','E','R'),
	INS		= INSN_TO_ENUM('I','N','S'),
	LEAVE		= INSN_TO_ENUM('L','E','A','V','E'),
	OUTS		= INSN_TO_ENUM('O','U','T','S'),
	POPA		= INSN_TO_ENUM('P','O','P','A'),
	PUSHA		= INSN_TO_ENUM('P','U','S','H','A'),
	ARPL		= INSN_TO_ENUM('A','R','P','L'),
	CLTS		= INSN_TO_ENUM('C','L','T','S'),
	LAR		= INSN_TO_ENUM('L','A','R'),
	LGDT		= INSN_TO_ENUM('L','G','D','T'),
	LIDT		= INSN_TO_ENUM('L','I','D','T'),
	LLDT		= INSN_TO_ENUM('L','L','D','T'),
	LMSW		= INSN_TO_ENUM('L','M','S','W'),
	LOADALL		= INSN_TO_ENUM('L','O','A','D','A','L','L'),
	LSL		= INSN_TO_ENUM('L','S','L'),
	LTR		= INSN_TO_ENUM('L','T','R'),
	SGDT		= INSN_TO_ENUM('S','G','D','T'),
	SIDT		= INSN_TO_ENUM('S','I','D','T'),
	SLDT		= INSN_TO_ENUM('S','L','D','T'),
	SMSW		= INSN_TO_ENUM('S','M','S','W'),
	STR		= INSN_TO_ENUM('S','T','R'),
	VERR		= INSN_TO_ENUM('V','E','R','R'),
	VERW		= INSN_TO_ENUM('V','E','R','W'),
	BSF		= INSN_TO_ENUM('B','S','F'),
	BSR		= INSN_TO_ENUM('B','S','R'),
	BT		= INSN_TO_ENUM('B','T'),
	BTC		= INSN_TO_ENUM('B','T','C'),
	BTR		= INSN_TO_ENUM('B','T','R'),
	BTS		= INSN_TO_ENUM('B','T','S'),
	CDQ		= INSN_TO_ENUM('C','D','Q'),
	CMPSD		= INSN_TO_ENUM('C','M','P','S','D'),
	CWDE		= INSN_TO_ENUM('C','W','D','E'),
	INSB		= INSN_TO_ENUM('I','N','S','B'),
	INSW		= INSN_TO_ENUM('I','N','S','W'),
	INSD		= INSN_TO_ENUM('I','N','S','D'),
	IRETD		= INSN_TO_ENUM('I','R','E','T','D'),
	IRET		= INSN_TO_ENUM('I','R','E','T'),
	JCXZ		= INSN_TO_ENUM('J','C','X','Z'),
	JECXZ		= INSN_TO_ENUM('J','E','C','X','Z'),
	LFS		= INSN_TO_ENUM('L','F','S'),
	LGS		= INSN_TO_ENUM('L','G','S'),
	LSS		= INSN_TO_ENUM('L','S','S'),
	LODSD		= INSN_TO_ENUM('L','O','D','S','D'),
	LOOPW		= INSN_TO_ENUM('L','O','O','P','W'),
	LOOPD		= INSN_TO_ENUM('L','O','O','P','D'),
	LOOPEW		= INSN_TO_ENUM('L','O','O','P','E','W'),
	LOOPED		= INSN_TO_ENUM('L','O','O','P','E','D'),
	LOOPZW		= INSN_TO_ENUM('L','O','O','P','Z','W'),
	LOOPZD		= INSN_TO_ENUM('L','O','O','P','Z','D'),
	LOOPNEW		= INSN_TO_ENUM('L','O','O','P','N','E','W'),
	LOOPNED		= INSN_TO_ENUM('L','O','O','P','N','E','D'),
	LOOPNZW		= INSN_TO_ENUM('L','O','O','P','N','Z','W'),
	LOOPNZD		= INSN_TO_ENUM('L','O','O','P','N','Z','D'),
	MOVSW		= INSN_TO_ENUM('M','O','V','S','W'),
	MOVSD		= INSN_TO_ENUM('M','O','V','S','D'),
	MOVSX		= INSN_TO_ENUM('M','O','V','S','X'),
	MOVZX		= INSN_TO_ENUM('M','O','V','Z','X'),
	POPAD		= INSN_TO_ENUM('P','O','P','A','D'),
	POPFD		= INSN_TO_ENUM('P','O','P','F','D'),
	PUSHAD		= INSN_TO_ENUM('P','U','S','H','A','D'),
	PUSHFD		= INSN_TO_ENUM('P','U','S','H','F','D'),
	SCASD		= INSN_TO_ENUM('S','C','A','S','D'),
	SETA		= INSN_TO_ENUM('S','E','T','A'),
	SETAE		= INSN_TO_ENUM('S','E','T','A','E'),
	SETB		= INSN_TO_ENUM('S','E','T','B'),
	SETBE		= INSN_TO_ENUM('S','E','T','B','E'),
	SETC		= INSN_TO_ENUM('S','E','T','C'),
	SETE		= INSN_TO_ENUM('S','E','T','E'),
	SETG		= INSN_TO_ENUM('S','E','T','G'),
	SETGE		= INSN_TO_ENUM('S','E','T','G','E'),
	SETL		= INSN_TO_ENUM('S','E','T','L'),
	SETLE		= INSN_TO_ENUM('S','E','T','L','E'),
	SETNA		= INSN_TO_ENUM('S','E','T','N','A'),
	SETNAE		= INSN_TO_ENUM('S','E','T','N','A','E'),
	SETNB		= INSN_TO_ENUM('S','E','T','N','B'),
	SETNBE		= INSN_TO_ENUM('S','E','T','N','B','E'),
	SETNC		= INSN_TO_ENUM('S','E','T','N','C'),
	SETNE		= INSN_TO_ENUM('S','E','T','N','E'),
	SETNG		= INSN_TO_ENUM('S','E','T','N','G'),
	SETNGE		= INSN_TO_ENUM('S','E','T','N','G','E'),
	SETNL		= INSN_TO_ENUM('S','E','T','N','L'),
	SETNLE		= INSN_TO_ENUM('S','E','T','N','L','E'),
	SETNO		= INSN_TO_ENUM('S','E','T','N','O'),
	SETNP		= INSN_TO_ENUM('S','E','T','N','P'),
	SETNS		= INSN_TO_ENUM('S','E','T','N','S'),
	SETNZ		= INSN_TO_ENUM('S','E','T','N','Z'),
	SETO		= INSN_TO_ENUM('S','E','T','O'),
	SETP		= INSN_TO_ENUM('S','E','T','P'),
	SETPE		= INSN_TO_ENUM('S','E','T','P','E'),
	SETPO		= INSN_TO_ENUM('S','E','T','P','O'),
	SETS		= INSN_TO_ENUM('S','E','T','S'),
	SETZ		= INSN_TO_ENUM('S','E','T','Z'),
	SHLD		= INSN_TO_ENUM('S','H','L','D'),
	SHRD		= INSN_TO_ENUM('S','H','R','D'),
	STOSB		= INSN_TO_ENUM('S','T','O','S','B'),
	STOSW		= INSN_TO_ENUM('S','T','O','S','W'),
	STOSD		= INSN_TO_ENUM('S','T','O','S','D'),
	BSWAP		= INSN_TO_ENUM('B','S','W','A','P'),
	CMPXCHG		= INSN_TO_ENUM('C','M','P','X','C','H','G'),
	INVD		= INSN_TO_ENUM('I','N','V','D'),
	INVLPG		= INSN_TO_ENUM('I','N','V','L','P','G'),
	WBINVD		= INSN_TO_ENUM('W','B','I','N','V','D'),
	XADD		= INSN_TO_ENUM('X','A','D','D'),
	CPUID		= INSN_TO_ENUM('C','P','U','I','D'),
	RDMSR		= INSN_TO_ENUM('R','D','M','S','R'),
	RDTSC		= INSN_TO_ENUM('R','D','T','S','C'),
	WRMSR		= INSN_TO_ENUM('W','R','M','S','R'),
	RSM		= INSN_TO_ENUM('R','S','M'),
	RDPMC		= INSN_TO_ENUM('R','D','P','M','C'),
	SYSCALL		= INSN_TO_ENUM('S','Y','S','C','A','L','L'),
	SYSRET		= INSN_TO_ENUM('S','Y','S','R','E','T'),
	CMOVA		= INSN_TO_ENUM('C','M','O','V','A'),
	CMOVAE		= INSN_TO_ENUM('C','M','O','V','A','E'),
	CMOVB		= INSN_TO_ENUM('C','M','O','V','B'),
	CMOVBE		= INSN_TO_ENUM('C','M','O','V','B','E'),
	CMOVC		= INSN_TO_ENUM('C','M','O','V','C'),
	CMOVE		= INSN_TO_ENUM('C','M','O','V','E'),
	CMOVG		= INSN_TO_ENUM('C','M','O','V','G'),
	CMOVGE		= INSN_TO_ENUM('C','M','O','V','G','E'),
	CMOVL		= INSN_TO_ENUM('C','M','O','V','L'),
	CMOVLE		= INSN_TO_ENUM('C','M','O','V','L','E'),
	CMOVNA		= INSN_TO_ENUM('C','M','O','V','N','A'),
	CMOVNAE		= INSN_TO_ENUM('C','M','O','V','N','A','E'),
	CMOVNB		= INSN_TO_ENUM('C','M','O','V','N','B'),
	CMOVNBE		= INSN_TO_ENUM('C','M','O','V','N','B','E'),
	CMOVNC		= INSN_TO_ENUM('C','M','O','V','N','C'),
	CMOVNE		= INSN_TO_ENUM('C','M','O','V','N','E'),
	CMOVNG		= INSN_TO_ENUM('C','M','O','V','N','G'),
	CMOVNGE		= INSN_TO_ENUM('C','M','O','V','N','G','E'),
	CMOVNL		= INSN_TO_ENUM('C','M','O','V','N','L'),
	CMOVNLE		= INSN_TO_ENUM('C','M','O','V','N','L','E'),
	CMOVNO		= INSN_TO_ENUM('C','M','O','V','N','O'),
	CMOVNP		= INSN_TO_ENUM('C','M','O','V','N','P'),
	CMOVNS		= INSN_TO_ENUM('C','M','O','V','N','S'),
	CMOVNZ		= INSN_TO_ENUM('C','M','O','V','N','Z'),
	CMOVO		= INSN_TO_ENUM('C','M','O','V','O'),
	CMOVP		= INSN_TO_ENUM('C','M','O','V','P'),
	CMOVPE		= INSN_TO_ENUM('C','M','O','V','P','E'),
	CMOVPO		= INSN_TO_ENUM('C','M','O','V','P','O'),
	CMOVS		= INSN_TO_ENUM('C','M','O','V','S'),
	CMOVZ		= INSN_TO_ENUM('C','M','O','V','Z'),
	SYSENTER	= INSN_TO_ENUM('S','Y','S','E','N','T','E','R')
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
