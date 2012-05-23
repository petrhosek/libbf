#ifndef SEGMENT_H_
#define SEGMENT_H_

#include <libkern/list.h>

/** Data structure representing segment. */
struct segment {
  uint64_t addr; /* load addr */
  uint64_t off; /* file offset */
  uint64_t fsz; /* file size */
  uint64_t msz; /* memory size */
  uint64_t type; /* segment type */

  struct list_head sections;
  struct list_head segments;
};

#endif // SEGMENT_H_
