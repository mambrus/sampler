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

#ifndef parsers_h
#define parsers_h

#define PARS_CUT_DEF_DELIM " "
#define MAX_MTCH_PERLINE 100
#define MATCH_START(A, I)          (A[I].rm_so)
#define MATCH_END(A, I)            (A[I].rm_eo)

int cutexec(const void *, const char *, size_t, regmatch_t [], int);
void cutfree(regex_t *);
int rcutexec(const void *, const char *, size_t, regmatch_t [], int);
void rcutfree(regex_t *);

#endif /* parsers_h */
