/* Definitions for condition code handling in final.cc and output routines.
   Copyright (C) 1987-2025 Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 3, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

#ifndef GCC_CONDITIONS_H
#define GCC_CONDITIONS_H

/* These are the machine-independent flags:  */

/* Set if the sign of the cc value is inverted:
   output a following jump-if-less as a jump-if-greater, etc.  */
#define CC_REVERSED 1

/* This bit means that the current setting of the N bit is bogus
   and conditional jumps should use the Z bit in its place.
   This state obtains when an extraction of a signed single-bit field
   or an arithmetic shift right of a byte by 7 bits
   is turned into a btst, because btst does not set the N bit.  */
#define CC_NOT_POSITIVE 2

/* This bit means that the current setting of the N bit is bogus
   and conditional jumps should pretend that the N bit is clear.
   Used after extraction of an unsigned bit
   or logical shift right of a byte by 7 bits is turned into a btst.
   The btst does not alter the N bit, but the result of that shift
   or extract is never negative.  */
#define CC_NOT_NEGATIVE 4

/* This bit means that the current setting of the overflow flag
   is bogus and conditional jumps should pretend there is no overflow.  */
/* ??? Note that for most targets this macro is misnamed as it applies
   to the carry flag, not the overflow flag.  */
#define CC_NO_OVERFLOW 010

/* This bit means that what ought to be in the Z bit
   should be tested as the complement of the N bit.  */
#define CC_Z_IN_NOT_N 020

/* This bit means that what ought to be in the Z bit
   should be tested as the N bit.  */
#define CC_Z_IN_N 040

/* Nonzero if we must invert the sense of the following branch, i.e.
   change EQ to NE.  This is not safe for IEEE floating point operations!
   It is intended for use only when a combination of arithmetic
   or logical insns can leave the condition codes set in a fortuitous
   (though inverted) state.  */
#define CC_INVERTED 0100

/* Nonzero if we must convert signed condition operators to unsigned.
   This is only used by machine description files.  */
#define CC_NOT_SIGNED 0200

#endif /* GCC_CONDITIONS_H */
