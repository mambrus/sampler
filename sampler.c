/***************************************************************************
 *   Copyright (C) 2012 by Michael Ambrus                                  *
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
#include <stdio.h>
#include <regex.h>
#include <string.h>
#include <error.h>
#include <stdlib.h>

int sampler_init(const char *filename, int n) {
	int rc;
	regex_t preg;
	char tstr[80];
	char estr[80];
	char rstr[80];
	regmatch_t mtch_idxs[80];
	int so,eo;

	memset(tstr,0,80); 
	memset(estr,0,80); 
	memset(rstr,0,80); 
/*
	rc=regcomp(&preg, "\\(.*\\)",
		REG_EXTENDED);
*/	
	rc=regcomp(&preg, filename,
		REG_EXTENDED);
	if (rc) {
		regerror(rc, &preg, estr, 80);
		fprintf(stderr, "Regcomp faliure: %s\n", estr);
		exit(1);
	}
	scanf("%80c\n",tstr);
	printf("You entered: %s\n",tstr);
	/*
	int regexec(const regex_t *preg, const char *string, size_t nmatch,
					regmatch_t pmatch[], int eflags);
	*/
	rc=regexec(&preg, tstr, 80,
					mtch_idxs, /*REG_NOTBOL|REG_NOTEOL*/ 0);
	if (rc) {
		regerror(rc, &preg, estr, 80);
		fprintf(stderr, "Regcomp faliure: %s\n", estr);
		exit(1);
	}
	so=mtch_idxs[n].rm_so;
	eo=mtch_idxs[n].rm_eo;
	strncpy(rstr, &(tstr[so]), eo-so);
	printf("Your embedded data is: %s\n", rstr);
}
