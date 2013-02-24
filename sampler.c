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
#include <stdio.h>
#include <regex.h>
#include <string.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#include <assert.h>

/* Private definitions */
#include "sigstruct.h"

#define TESTF( F ) (feof(F) || ferror(F))

/* Compiled version of definition regex */
regex_t preg;

/* How a definition line is expected to be formatted. Be very permissive,
 * especially where regex strings go (#5, #6). Let client or any post
 * process code figure out what's not right (i.e. strings instead of numbers
 * eta.) */
#define DFN_LINE "^\
([[:alnum:] _/,]*);\
([[:alnum:] _/,]*);\
([[:alnum:] _/,]*);\
([[:alnum:] _/,]*);\
([[:print:]]*);\
([[:print:]]*);\
([[:alnum:] _/,]*)\
$"

/* Number of sub-expressions. NOTE: Very important to match this with
 * DFN_LINE. If regex is written with more subexpressions than columns it
 * has to be adjusted here */
#define MAX_DFN_COLUMNS 7

/* Max length of a definition input-line */
#define MAX_DFN_LINE_LEN 1024

/* Max length of a definition input-file. Just a number for sanity-checks
 * really */
#define MAX_DFN_LINES 1024

/* Takes one line describing a sample-signal and make a signal scruct of it.
 * Return 0 if all OK.*/
static int parse_dfn_line(char *line, struct smpl_signal *rsig, int n) {
	int rc,i;
	char err_str[80]="\0"; /* Auto-fills with \0 */
	/*Need to add +1 (extra) as index matches the whole pattern */
	regmatch_t mtch_idxs[MAX_DFN_COLUMNS+1];

	if (!rsig)
		return -1;
	if (n<0 || n>MAX_DFN_LINES)
		return -2;

	rc=regexec(&preg, line, MAX_DFN_COLUMNS+1, mtch_idxs, 0);
	if (rc) {
		regerror(rc, &preg, err_str, 80);
		fprintf(stderr, "Regcomp faliure: %s\n", err_str);
		return(rc);
	}
	
	/* Loop trough and put '\0' everywhere where pattern ends. Note that
	 * index=0 matches the compete pattern. I.e. add +1 */
	for (i=1; i<=MAX_DFN_COLUMNS; i++) {
		line[mtch_idxs[i].rm_eo]=0;
		printf("%02d: %s\n", i, &(line[mtch_idxs[i].rm_so]));
		
	}
		
	return 0;
} 

static int parse_initfile(const char *fn) {
	FILE *fl;
	int rc,lno=0;
	char line[MAX_DFN_LINE_LEN],*rcs;
	struct smpl_signal tsig;

	fl=fopen(fn,"r");
	if (fl==NULL) {
		printf("%s",line);
		perror("Signal description file-error:");
		exit(1);
	}
	
	for (
		lno=0,rc=0;
		rc==0; 
		lno++
	) {
		rcs=fgets(line, MAX_DFN_LINE_LEN, fl);
		if (rcs){
			int len=strnlen(line, MAX_DFN_LINE_LEN);
			if (len>0 && len<MAX_DFN_LINE_LEN) {
				if ( line[0] != '#' && len>0 ) {
					//Ignore lines starting with '#' and empty lines
					//Can't handle "empty" lines with whites yet TBD
					
					/* Line below: Get rid of the EOL. Not sure why this is
					 * needed as REG_NOTEOL was not given to regexec.
					 * Perhaps miss-understanding standard. Need check TBD
					 * */
					line[len-1]=0;
					rc=parse_dfn_line(line, &tsig, lno);
				} else {
					//OK, just ignored
					rc=0;
				}
			}
		}

		if (!rc)
			rc=TESTF(fl);
	}

	if (rc!=0) {
		if (ferror(fl)) {
			perror("Signal description file read error:");
			rc = EBADF;
			goto fin_pars_init;
		} else if (feof(fl)) {
			fprintf(stderr,"Info: Scanned %d lines successfully\n",lno-1);
			rc = 0;
			goto fin_pars_init;
		} else {
			fprintf(stderr,"Error: Scanned line %d is bad:\n",lno);
			fprintf(stderr,":%s\n",line);
			fprintf(stderr,":%s\n",DFN_LINE);

			rc = ENOEXEC;
			goto fin_pars_init;
		}
	}
	
	rc = ENOSYS;
fin_pars_init:
	
	fclose(fl);
	return(rc);
} 

int sampler_init(const char *siginitfn) {
	int rc;
	char err_str[80];
	FILE *initfile;

	memset(err_str,0,80); 
	rc=regcomp(&preg, DFN_LINE, REG_EXTENDED);
	if (rc) {
		regerror(rc, &preg, err_str, 80);
		fprintf(stderr, "Regcomp faliure("__FILE__":%d): %s\n", 
			__LINE__, err_str);
		exit(1);
	}
	
	rc=parse_initfile(siginitfn); 
	/*TBD: Add better error-handling*/
	assert(rc==0);

	regfree(&preg);

	//Dear gcc, shut up
	return 1;
}

