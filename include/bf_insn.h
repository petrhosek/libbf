/**
 * @file bf_insn.h
 * @brief Definition and API of bf_insn.
 * @details bf_insn objects are <b>libind</b>'s abstraction of machine
 * instructions.
 * @author Mike Kwan <michael.kwan08@imperial.ac.uk>
 */

#ifndef BF_INSN_H
#define BF_INSN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "binary_file.h"
#include "bf_basic_blk.h"
#include "bf_insn_decoder.h"
#include "libase/htable.h"

/**
 * @internal
 * @struct bf_insn_part
 * @brief The constituent unit composing a bf_insn.
 * @details A bf_insn_part represents a list of strings composing the bf_insn.
 * e.g. push %rbp consists of <i>push</i> and <i>%rbp</i>
 */
struct bf_insn_part {
	struct list_head list;
	char *		 str;
};

/**
 * @struct bf_insn
 * @brief <b>libind</b>'s abstraction of an instruction.
 * @details A bf_insn consists of a list of its constituent parts
 * (bf_insn_part objects).
 */
struct bf_insn {
	/**
	 * @var vma
	 * @brief The VMA of the instruction.
	 */
	bfd_vma		      vma;

	/**
	 * @var is_data
	 * @brief TRUE if the contents at vma represent data.
	 */
	bool		      is_data;

	/**
	 * @var mnemonic
	 * @brief One of the mnemonics defined by insn_mnemonic in
	 * bf_insn_decoder.h. If the value is 0, the instruction uses an
	 * unrecognised mnemonic.
	 */
	enum insn_mnemonic    mnemonic;

	/**
	 * @var secondary_mnemonic
	 * @brief If mnemonic is a macro mnemonic, secondary_mnemonic holds
	 * the secondary mnemonic. For example, the secondary_mnemonic of
	 * 'rep movs' would be `movs`.
	 */
	enum insn_mnemonic    secondary_mnemonic;

	/**
	 * @var operand1
	 * @brief The first operand of the bf_insn. If the value is NULL, the
	 * instruction uses an unrecognised operand or the information is not
	 * valid because the bf_insn represents data (is_data flag will be
	 * set).
	 */
	struct insn_operand   operand1;

	/**
	 * @var operand2
	 * @brief The second operand of the bf_insn. If the value is NULL, the
	 * instruction either only has one operand or it uses an unrecognised
	 * operand or the information is not valid because the bf_insn
	 * represents data (is_data flag will be set).
	 */
	struct insn_operand   operand2;

	/**
	 * @var operand3
	 * @brief The third operand of the bf_insn. If the value is NULL, the
	 * instruction either does not have a third operand or it uses an
	 * unrecognised operand or the information is not valid because the
	 * bf_insn represents data (is_data flag will be set).
	 */
	struct insn_operand   operand3;

	/**
	 * @var extra_info
	 * @brief <b>libopcodes</b> often makes suggestions about the memory
	 * address an instruction interacts with. In the majority of
	 * circumstances, if extra_info is not 0, it is the suspected value
	 * of the memory reference of the bf_insn.
	 */
	bfd_vma		      extra_info;

	/**
	 * @internal
	 * @var part_list
	 * @brief Start of linked list of parts (bf_insn_part objects).
	 */
	struct list_head      part_list;

	/**
	 * @internal
	 * @var entry
	 * @brief Entry into the binary_file.insn_table hashtable of
	 * binary_file.
	 */
	struct htable_entry   entry;

	/**
	 * @var bb
	 * @brief The bf_basic_blk containing the bf_insn.
	 */
	struct bf_basic_blk * bb;
};

/**
 * @internal
 * @brief Creates a new bf_insn object.
 * @param bb The bf_basic_blk containing the bf_insn.
 * @param vma The VMA of the bf_insn.
 * @return A bf_insn object.
 * @note bf_close_insn must be called to allow the object to properly clean up.
 */
extern struct bf_insn * bf_init_insn(struct bf_basic_blk * bb, bfd_vma vma);

/**
 * @internal
 * @brief Appends to the tail of the parts list.
 * @param insn The bf_insn whose parts list is to be appended to.
 * @param str The string to append.
 */
extern void bf_add_insn_part(struct bf_insn * insn, char * str);

/**
 * @internal
 * @brief Sets the is_data flag of the bf_insn.
 * @param insn The bf_insn to set the information of.
 * @param is_data TRUE if the contents at the vma of the bf_insn represent
 * data. FALSE otherwise.
 */
extern void bf_set_is_data(struct bf_insn * insn, bool is_data);

/**
 * @internal
 * @brief Assigns semantic mnemonic information to the bf_insn.
 * @param insn The bf_insn to add information to.
 * @param str The string representing the mnemonic information to be assigned.
 */
extern void bf_set_insn_mnemonic(struct bf_insn * insn, char * str);

/**
 * @internal
 * @brief Assigns semantic secondary mnemonic information to the bf_insn.
 * @param insn The bf_insn to add information to.
 * @param str The string representing the mnemonic information to be assigned.
 */
extern void bf_set_insn_secondary_mnemonic(struct bf_insn * insn, char * str);

/**
 * @internal
 * @brief Assigns semantic information about the first operand to the bf_insn.
 * @param insn The bf_insn to add information to.
 * @param str The string representing the operand information to be assigned.
 */
extern void bf_set_insn_operand(struct bf_insn * insn, char * str);

/**
 * @internal
 * @brief Assigns semantic information about the second operand to the bf_insn.
 * @param insn The bf_insn to add information to.
 * @param str The string representing the operand information to be assigned.
 */
extern void bf_set_insn_operand2(struct bf_insn * insn, char * str);

/**
 * @internal
 * @brief Assigns semantic information about the third operand to the bf_insn.
 * @param insn The bf_insn to add information to.
 * @param str The string representing the operand information to be assigned.
 */
extern void bf_set_insn_operand3(struct bf_insn * insn, char * str);

/**
 * @brief Prints the bf_insn to stdout.
 * @param The bf_insn to be printed.
 * @details This function would generally be used for debug or demonstration
 * purposes since the format of the output is not easily customisable. The
 * bf_insn should be manually printed if customised output is desired.
 */
extern void bf_print_insn(struct bf_insn * insn);

/**
 * @internal
 * @brief Prints the bf_insn to a FILE in dot format.
 * @param stream An open FILE to be written to.
 * @param insn The bf_insn to be written.
 */
extern void bf_print_insn_dot(FILE * stream, struct bf_insn * insn);

/**
 * @internal
 * @brief Closes a bf_insn object.
 * @param insn The bf_insn to be closed.
 */
extern void bf_close_insn(struct bf_insn * insn);

/**
 * @internal
 * @brief Adds a bf_insn to the binary_file.insn_table.
 * @param bf The binary_file holding the binary_file.insn_table to be added to.
 * @param insn The bf_insn to be added.
 */
extern void bf_add_insn(struct binary_file * bf, struct bf_insn * insn);

/**
 * @brief Gets the bf_insn object for the starting VMA.
 * @param bf The binary_file to be searched.
 * @param vma The VMA of the bf_insn being searched for.
 * @return The bf_insn starting at vma or NULL if no bf_insn has been
 * discovered at that address.
 */
extern struct bf_insn * bf_get_insn(struct binary_file * bf, bfd_vma vma);

/**
 * @brief Checks whether a discovered bf_insn exists for a VMA.
 * @param bf The binary_file to be searched.
 * @param vma The VMA of the bf_insn being searched for.
 * @return TRUE if a bf_insn could be found, otherwise FALSE.
 */
extern bool bf_exists_insn(struct binary_file * bf, bfd_vma vma);

/**
 * @internal
 * @brief Releases memory for all currently discovered bf_insn objects.
 * @param bf The binary_file holding the binary_file.insn_table to be purged.
 */
extern void bf_close_insn_table(struct binary_file * bf);

/**
 * @brief Invokes a callback for each discovered bf_insn.
 * @param bf The binary_file holding the bf_insn objects.
 * @param handler The callback to be invoked for each bf_insn.
 * @param param This will be passed to the handler each time it is invoked. It
 * can be used to pass data to the callback.
 */
extern void bf_for_each_insn(struct binary_file * bf,
		void (*handler)(struct binary_file *, struct bf_insn *,
		void *), void * param);

/**
 * @brief Invokes a callback for each part in a bf_insn.
 * @param insn The bf_insn being analysed.
 * @param handler The callback to be invoked for each bf_insn.
 * @param param This will be passed to the handler each time it is invoked. It
 * can be used to pass data to the callback.
 */
extern void bf_for_each_insn_part(struct bf_insn * insn,
		void (*handler)(struct bf_insn *, char *,
		void *), void * param);

#ifdef __cplusplus
}
#endif

#endif
