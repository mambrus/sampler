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

/* cut and reversed-cut parsers
  Functions with the same API as regexp. API is as follows:

	int regexec(const regex_t *preg, const char *string, size_t nmatch,
				   regmatch_t pmatch[], int eflags);

	void regfree(regex_t *preg);

*/

#include <string.h>
#include <stdlib.h>

#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include "assert_np.h"
#include <limits.h>
#include <regex.h>
#include <parsers.h>

#ifndef REG_EEND
#define REG_EEND 1 /* non-zero number, for Android */
#endif

int cutexec(const void *dS, const char *string, size_t nmatch,
                   regmatch_t pmatch[], int eflags) {
	char d = ((char*)dS)[0];
	int i,instring=0,linstr=0,j=strlen(string),finish=0,g=1;

	for (i=0; i<j && g<nmatch && !finish; i++,linstr=instring) {
		if (
			(string[i] == 0x0D) ||
			(string[i] == 0x0A) ||
			(string[i] == '\n')
		)
			finish=1;

		if (!instring) {
			if (string[i] != d) {
				if (!linstr) {
					instring=1;
					MATCH_START(pmatch,g) = i;
				}
			}
		}else{
			if (string[i] == d) {
				if (linstr) {
					instring=0;
					MATCH_END(pmatch,g) = i;
					g++;
				}
			}
		}
	}
	if (instring){
		MATCH_END(pmatch,g) = i;
		g++;
	}
	if (g>=nmatch)
		return REG_NOMATCH;

	MATCH_START(pmatch,0) = MATCH_START(pmatch,1);
	MATCH_END(pmatch,0) = MATCH_END(pmatch,g-1);

	return 0;
}

void cutfree(regex_t *preg){
	assert_np("TBD" == 0);
}

int rcutexec(const void *dS, const char *string, size_t nmatch,
                   regmatch_t pmatch[], int eflags) {
	char d = ((char*)dS)[0];
	int i,instring=0,linstr=0,j=strlen(string),finish=0,g=1;

	for (i=j-1; i>=0 && g<nmatch && !finish; i--,linstr=instring) {
		if (!(
			(string[i] == 0x0D) ||
			(string[i] == 0x0A) ||
			(string[i] == '\n')
		))
		{
		if (!instring) {
			if (string[i] != d) {
				if (!linstr) {
					instring=1;
					MATCH_END(pmatch,g) = i+1;
				}
			}
		}else{
			if (string[i] == d) {
				if (linstr) {
					instring=0;
					MATCH_START(pmatch,g) = i+1;
					g++;
				}
			}
		}
	}
	}
	if (!instring){
		MATCH_START(pmatch,g) = i+1;
		g++;
	}
	if (g>=nmatch)
		return REG_NOMATCH;

	MATCH_START(pmatch,0) = MATCH_START(pmatch,g-1);
	MATCH_END(pmatch,0) = MATCH_END(pmatch,1);

	return 0;
}

void rcutfree(regex_t *preg){
	assert_np("TBD" == 0);
}

/* Local definitions */
#include "local.h"
