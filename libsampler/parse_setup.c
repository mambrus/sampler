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
#include <limits.h>

/* Local definitions */
#include "local.h"
/* Private definitions */
#include "sigstruct.h"

#define LDATA struct smpl_signal
#include <mlist.h>

#define TESTF( F ) (feof(F) || ferror(F))

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

/* Max number of sub-signals. Just a number for sanity-checks really */
#define MAX_SUBSIGS 10

/* Max number of signals. A number for sanity-checks really, but also the
   offset for signal ID:s (i.e. the unique number each sample thread has).
   Nice and even 10-base number for debugging ease */
#define MAX_SIGS 1000

#if (!(MAX_SUBSIGS < (MAX_SIGS-1)))
#error Impossible MAX_SUBSIGS & MAX_SIGS values
#endif

static int sigdef_compare(LDATA *lval, LDATA *rval);

/* Takes one line describing a sample-signal and make a signal scruct of it.
 * Return 0 if all OK.*/
static int parse_dfn_line(const regex_t *preg, char *line, struct smpl_signal *rsig, int lno) {
	int rc,i,j,len;
	char err_str[80]="\0"; /* Auto-fills with \0 */
	/*Need to add +1 (extra) as index 0 matches the whole pattern */
	regmatch_t mtch_idxs[MAX_DFN_COLUMNS+1];
	char *sptr;

	if (!rsig)
		return -1;
	if (lno<0 || lno>MAX_DFN_LINES)
		return -2;

	rc=regexec(preg, line, MAX_DFN_COLUMNS+1, mtch_idxs, 0);
	if (rc) {
		regerror(rc, preg, err_str, 80);
		fprintf(stderr, "Regexec faliure: %s\n", err_str);
		return(rc);
	}

	/* Loop trough input-line and put '\0' everywhere where pattern ends.
	 * Note that index=0 matches the compete pattern. I.e. add +1 */
	for (i=1; i<=MAX_DFN_COLUMNS; i++) {
		line[mtch_idxs[i].rm_eo]=0;
		printf("%02d: %s\n", i, &(line[mtch_idxs[i].rm_so]));
	}

	rsig->sig_def.id			= lno;
	rsig->sig_def.name			= strdup(&(line[mtch_idxs[SNAME].rm_so]));
	rsig->sig_def.fname			= strdup(&(line[mtch_idxs[SFNAME].rm_so]));
	rsig->sig_def.fdata			= strdup(&(line[mtch_idxs[SFDATA].rm_so]));
	rsig->sig_def.fopmode.mask	= atoi(&(line[mtch_idxs[SPERS].rm_so]));

	printf("Size of fopmode: %d\nFile operation bits:\n"
			"   openclose:%d\n"
			"   canblock:%d\n"
			"   always:%d\n",
			sizeof(union fopmode),
			rsig->sig_def.fopmode.bits.openclose,
			rsig->sig_def.fopmode.bits.canblock,
			rsig->sig_def.fopmode.bits.always
	);

	/*Parse the line identifier regex*/
	rsig->sig_def.rgx_line.str	= strdup(&(line[mtch_idxs[SRGXL].rm_so]));
	if (rsig->sig_def.rgx_line.str[0]) {
		rc=regcomp(
			&(rsig->sig_def.rgx_line.rgx),
			rsig->sig_def.rgx_line.str,
			REG_EXTENDED
		);
		if (rc) {
			regerror(rc, &(rsig->sig_def.rgx_line.rgx), err_str, 80);
			fprintf(stderr, "Regcomp faliure: %s\n", err_str);
			return(rc);
		}
	}

	/*Parse the signal pattern */
	rsig->sig_def.rgx_sig.str	= strdup(&(line[mtch_idxs[SRGXS].rm_so]));
	if (rsig->sig_def.rgx_sig.str[0]) {
		rc=regcomp(
			&(rsig->sig_def.rgx_sig.rgx),
			rsig->sig_def.rgx_sig.str,
			REG_EXTENDED
		);
		if (rc) {
			regerror(rc, &(rsig->sig_def.rgx_sig.rgx), err_str, 80);
			fprintf(stderr, "Regcomp faliure: %s\n", err_str);
			return(rc);
		}
	}

	sptr=&(line[mtch_idxs[SIDXS].rm_so]);
	len=strlen(sptr);

	if (!len) {
		/* Field is not filled in, which is a special case. It corresponds
		 * to one sub-signal, matching the whole regex, but there's no need
		 * to parse */
		rsig->sig_def.idxs				= calloc(1, sizeof(int));
		assert(rsig->sig_def.idxs);
		(rsig->sig_def.idxs)[0]			= 0;
		rsig->sig_data.nsigs			= 1;
		rsig->sig_data.ownr				= rsig;
		rsig->sig_data.sigs				= calloc(1, sizeof(struct sig_sub));
		assert(rsig->sig_data.sigs);
		(rsig->sig_data.sigs)[0].ownr	= &rsig->sig_data;
		(rsig->sig_data.sigs)[0].sub_id = 0;
		(rsig->sig_data.sigs)[0].id     = MAX_SIGS * (lno+1);
	} else {
		/* Parse the sub-signals string. */
		int instr=0;
		char *tptr=sptr;
		int nidx=0;

		/* To make separation into array easier, replace every non ASCII
		 * digit with \0. Also take the opportunity to count the number of
		 * sub-signals */
		for (instr=0, i=0, nidx=0; i<len; i++) {
			if (tptr[i] >= '0' && tptr[i] <= '9') {
				if (!instr) {
					instr=1;
					nidx++;
				}
			} else {
				tptr[i] = 0;
				instr=0;
			}
		}

		rsig->sig_data.nsigs = nidx;
		rsig->sig_data.ownr = rsig;
		rsig->sig_def.idxs = calloc(nidx, sizeof(int));
		memset(rsig->sig_def.idxs, 0, nidx*sizeof(int));
		rsig->sig_data.sigs	= calloc(nidx, sizeof(struct sig_sub));
		assert(rsig->sig_data.sigs);
		memset(rsig->sig_data.sigs, 0, nidx*sizeof(struct sig_sub));

		/* Initialize sub-signal data harvester (data for each work-thread) */
		for (i=0; i<nidx; i++){
			(rsig->sig_data.sigs)[i].ownr	= &rsig->sig_data;
			(rsig->sig_data.sigs)[i].sub_id = i;
			(rsig->sig_data.sigs)[i].id     = MAX_SIGS * (lno+1) + i;
		}

		tptr=sptr;

		/* Store sub-match index */
		for (instr=0,i=0,j=0; i<len; i++) {
			if (tptr[i] >= '0' && tptr[i] <= '9') {
				if (!instr) {
					instr=1;
					(rsig->sig_def.idxs)[j++]=atoi(&tptr[i]);
				}
			} else
				instr=0;
		}
	}
	return 0;
}

/* Converts signals described in abstract file and put it in a mlist
 * container. Handle to the mlist is placed in list. Return value tells
 * if parsing went well, i.e. if the returned list is valid or not. */
int parse_initfile(const char *fn, handle_t *list) {
	FILE *fl;
	int rc,lno=0;
	char line[MAX_DFN_LINE_LEN],*rcs;
	struct smpl_signal *tsig;
	struct node *smpl_rc;
	char err_str[80];
	regex_t preg; /* Compiled version of definition regex for one line */

	memset(err_str,0,80);
	rc=regcomp(&preg, DFN_LINE, REG_EXTENDED);
	if (rc) {
		regerror(rc, &preg, err_str, 80);
		fprintf(stderr, "Regcomp faliure("__FILE__":%d): %s\n",
			__LINE__, err_str);
		return(1);
	}
	rc=create_mlist(sizeof(struct smpl_signal),NULL/*sigdef_compare*/, list);
	assert(rc==0); assert(list);

	fl=fopen(fn,"r");
	if (fl==NULL) {
		printf("%s",line);
		perror("Signal description file-error:");
		exit(1);
	}

	for (lno=0,rc=0; rc==0; lno++) {
		rcs=fgets(line, MAX_DFN_LINE_LEN, fl);
		if (rcs){
			int len=strnlen(line, MAX_DFN_LINE_LEN);
			if (len>0 && len<MAX_DFN_LINE_LEN) {
				if ( line[0] != '#' && len>0 ) {
					/* Ignore lines starting with '#' and empty lines
					 * Can't handle "empty" lines with whites yet TBD */

					/* Line below: Get rid of the EOL. Not sure why this is
					 * needed as REG_NOTEOL was not given to regexec.
					 * Perhaps miss-understanding standard. Need check TBD
					 * */
					line[len-1]=0;
					tsig=malloc(sizeof(struct smpl_signal));
					assert(tsig);
					rc=parse_dfn_line(&preg, line, tsig, lno);
					if (!rc) {
						/* Add to mlist */
						smpl_rc=mlist_add(*list,tsig);
						assert(smpl_rc);
					} else {
						/* Handle error */
						/* TBD */
					}
				} else {
					/* OK, just ignored */
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
	regfree(&preg);

	fclose(fl);
	return(rc);
}


static int sigdef_compare(LDATA *lval, LDATA *rval){
	return 0;
}
