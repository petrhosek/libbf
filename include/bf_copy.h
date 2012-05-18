/**
 * @internal
 * @file bf_copy.h
 * @brief API of bf_copy.
 * @details bf_copy creates a writable copy of a given BFD object. The reason
 * this is required is that <b>libbfd</b> does not allow editing of an existing
 * BFD object. Instead, we manually duplicate the input BFD and work on top of
 * that one instead.
 */

#ifndef BF_COPY_H
#define BF_COPY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "binary_file.h"
#include "bfd/elf-bfd.h"

/**
 * @internal
 * @brief Creates a writable copy of a BFD object.
 * @param abfd The BFD object to be duplicated.
 * @param output_path The location the writable BFD will be saved to.
 * @returns The writable BFD object created.
 */

bfd * create_writable_bfd(bfd * abfd, char * output_path);

#ifdef __cplusplus
}
#endif

#endif
