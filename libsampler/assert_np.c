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


#include "assert_np.h"

#ifdef NDEBUG
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <limits.h>

void _assertfail(char *assertstr, char *filestr, int line) {
		char buff[PATH_MAX];

		snprintf(buff,PATH_MAX,"assert_ext: \"%s\" %s:%d\nerrno %d",
			assertstr, filestr, line, errno);
		perror(buff);
		/* Generate coredump */
		fprintf(stderr,"Calling abort() for coredump \n");
		fflush(ASSERT_ERROR_FILE);
		abort();
		fprintf(stderr,"Abort failed. Null-pointer assignement for coredump \n");
		fflush(ASSERT_ERROR_FILE);
		/* Should never return, but just in case lib is broken (Android?)
		 * make a deliberate null pointer assignment */
		*((int *)NULL) = 1;
}
#endif //#ifdef NDEBUG
