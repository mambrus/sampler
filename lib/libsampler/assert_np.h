/***************************************************************************
 *   Copyright (C) 2013 by Michael Ambrus                                  *
 *   ambrmi09@gmail.com                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

/* Lazy error-handling based on variations of assert  */

/*
 * Like in assert, these macros can be reassigned and the listen to the same
 * NDEBUG compilation, but differently from assert, they have a different
 * behavior on error when NDEBUG is set.
 *
 * assert_ign - Do the stuff inside the brackets no matter how NDEBUG is
 *              set but if NDEBUG is set, also crash. I.e. this is a safe
 *              from miss-haps in, form of code not being executed, macro to
 *              use instead of assert practically everywhere where speed is
 *              preferred when NDEBUG is set.
 * assert_np  - same as assert_ign. Comparability reasons only.
 * assert_ret - Returns from function with the result. Assumes the function
 *              returns error-codes. Think twice before use.
 * assert_ext - Same as assert, but don't listen to NDEBUG - always assert.
 *              This is a safe macro to use, but might consume unnecessary
 *              CPU.
 */

#ifndef assert_np_h
#define assert_np_h
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>

#ifndef NDEBUG
#  define assert_ign assert
#  define assert_np  assert
#  define assert_ret assert
#  define assert_ext assert
#else
#include <stdio.h>

#ifndef ASSERT_ERROR_FILE
#define ASSERT_ERROR_FILE stderr
#endif
void _assertfail(char *assertstr, char *filestr, int line);

/* Do the stuff, just ignore acting on the result. */
#  define assert_np(p) (p)
#  define assert_ign assert_np

/* Mimic assert real behavior when NDEBUG is not set. I.e. always act. */

#  define assert_ext(p) ((p) ? (void)0 : (void) _assertfail( \
		#p, __FILE__, __LINE__ ) )

/* Lazy error-handling. Careful using this. Assumes function invoked from
accepts returning with code, and that the code means error */
#  define assert_ret(p) (                                    \
	{                                                        \
		int rc = (p);                                        \
                                                             \
		rc ? (void)0 :                                       \
		fprintf(ASSERT_ERROR_FILE,"_assert_ex %s (%s:%d)\n", \
			#p, __FILE__, __LINE__ );                        \
		fflush(ASSERT_ERROR_FILE);                           \
		return rc;                                           \
	}                                                        \
)
#endif
#endif /* assert_np_h */

