/*
 * This file is part of libbf.
 *
 * libbf is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * libbf is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with libbf.  If not, see <http://www.gnu.org/licenses/>.
 */

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
