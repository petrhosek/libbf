#include "binary_file.h"

#ifndef BF_DISASM_H
#define BF_DISASM_H

/*
 * This file contains libopcodes callbacks
 */

/*
 * Provides the callback as part of libopcodes initialisation.
 * Will eventually be renamed.
 */
extern int binary_file_fprintf(void *, const char *, ...);

#endif
