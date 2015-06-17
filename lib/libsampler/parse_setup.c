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
#include <parsers.h>
#include <string.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <assert.h>
#include <assure.h>
#include <limits.h>
#define LDATA struct smpl_signal
#include <mlist.h>

/* Local definitions */
#include "local.h"
/* Private definitions */
#include "sigstruct.h"


#define TESTF( F ) (feof(F) || ferror(F))

/* How a definition line is expected to be formatted. Be very permissive,
 * especially where regex strings go (#5, #6). Let client or any post
 * process code figure out what's not right (i.e. strings instead of numbers
 * eta.)
 * The order, pattern and meaning is as specified below;

([[:alnum:] ~#_/,.:-]*); Name (symbolic)
([[:alnum:] _/,.:-]*);   Name (from filename)
([[:alnum:] _/,.:-]*);   Data content file
([[:alnum:]]*);          File operation (hex permitted)
([[:print:]]*);          Line pattern
([0-9]*);                Line match index
([[:print:]]*);          Signal pattern
([[[:alnum:] _/,$]*);    Match index(s). Valid indexes are numbers, the rest
						 are valid delimiters, except $ which is special and
						 which tells is subexpression for that index is a
						 string or not. Note parser is concerning this
                         field: $ is supposed to come directly before a number.
([0-9]*)                 NUB

*/

#define DFN_LINE "^\
([[:alnum:] ~#_/,.:-]*);\
([[:alnum:] _/,.:-]*);\
(<[[:alnum:] _/,.:-]*|[[:alnum:] _/,.:-]*);\
([[:alnum:]]*);\
([[:print:]]*);\
([0-9]*);\
([[:print:]]*);\
([[:alnum:] _/,$]*);\
([0-9]*)\
$"

//Any of these should be correct:
//(<[(][[:alnum:] _/,.:-]*[)]|[[:alnum:] _/,.:-]*);\
//(<[(][[:alnum:] _/,.:-]*|[[:alnum:] _/,.:-]*);\
//
/* Number of sub-expressions. NOTE: Very important to match this with
 * DFN_LINE. If regex is written with more subexpressions than columns it
 * has to be adjusted here */
#define MAX_DFN_COLUMNS 9

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
static int parse_dfn_line(
	const regex_t *preg,
	char *line,
	struct smpl_signal *smpl_signal,
	int lno)
{
	int rc,i,j,sigsub_strlen;
	char err_str[80]="\0",isS='X';
	struct sig_def *sig_def;
	struct sig_data *sig_data;
	char *subdef_str;
	/* Note on below line: Need to add +1 (extra) as index 0 matches the
	 * whole pattern */
	regmatch_t mtch_idxs[MAX_DFN_COLUMNS+1];

	if (!smpl_signal)
		return -1;
	else {
		sig_def = &(smpl_signal->sig_def);
		sig_data = &(smpl_signal->sig_data);
	}
	if (lno<0 || lno>MAX_DFN_LINES)
		return -2;

	rc=regexec(preg, line, MAX_DFN_COLUMNS+1, mtch_idxs, 0);
	if (rc) {
		regerror(rc, preg, err_str, 80);
		fprintf(stderr, "Regexec match failure: %s\n", err_str);
		return(rc);
	}

	/* Loop trough input-line and put '\0' everywhere where pattern ends.
	 * Note that index=0 matches the complete pattern. I.e. add +1 */
	for (i=1; i<=MAX_DFN_COLUMNS; i++) {
		line[mtch_idxs[i].rm_eo]=0;
		INFO(("%02d: %s\n", i, &(line[mtch_idxs[i].rm_so])));
	}

	sig_def->id           = lno;
	sig_def->name         = strdup(&(line[mtch_idxs[SNAME].rm_so]));
	sig_def->fname        = strdup(&(line[mtch_idxs[SFNAME].rm_so]));
	sig_def->fdata        = strdup(&(line[mtch_idxs[SFDATA].rm_so]));
	if (
		line[mtch_idxs[SFOPMOD].rm_so] &&
		!strncmp(&line[mtch_idxs[SFOPMOD].rm_so],"0x",2))
	{
		assert_ext("Line format error" != NULL &&
			sscanf(&(line[mtch_idxs[SFOPMOD].rm_so]),"%x",&sig_def->sops.mask)
				!= EOF);
	} else {
		assert_ext("Line format error" != NULL &&
			sscanf(&(line[mtch_idxs[SFOPMOD].rm_so]),"%u",&sig_def->sops.mask)
				!= EOF);
	}

	if (line[mtch_idxs[SLIDX].rm_so] == 0)
		if ((line[mtch_idxs[SRGXL].rm_so] == 0) ||
			(*sig_def).sops.bits.always)
		{
			sig_def->lindex = -1;
		} else {
			sig_def->lindex = 1;
		}
	else
		sig_def->lindex = atoi(&line[mtch_idxs[SLIDX].rm_so]);

	sig_def->nuce = atoi(&(line[mtch_idxs[SNUCE].rm_so]));

	INFO(("Size of sops in bytes (should be 4): %u\n"
			"File operation bitss 0x%08X:\n"
			"   openclose:%d\n"
			"   canblock:%d\n"
			"   trigger:%d\n"
			"   timed:%d\n"
			"   no_rewind:%d\n"
			"   thread_ops:%d\n"
			"   tprio:%d\n"
			"   parser:%d\n"
			"   always:%d\n",
			(uint32_t)sizeof(union sops),
			sig_def->sops.mask,
			sig_def->sops.bits.openclose,
			sig_def->sops.bits.canblock,
			sig_def->sops.bits.trigger,
			sig_def->sops.bits.timed,
			sig_def->sops.bits.no_rewind,
			sig_def->sops.bits.tops,
			sig_def->sops.bits.tprio,
			sig_def->sops.bits.parser,
			sig_def->sops.bits.always
	));

	/*Parse the line identifier regex*/
	sig_def->rgx_line.str	= strdup(&(line[mtch_idxs[SRGXL].rm_so]));
	if (sig_def->rgx_line.str[0]) {
		rc=regcomp(
			&(sig_def->rgx_line.rgx),
			sig_def->rgx_line.str,
			REG_EXTENDED
		);
		if (rc) {
			regerror(rc, &(sig_def->rgx_line.rgx), err_str, 80);
			fprintf(stderr, "Regexec compilation failure: %s\n", err_str);
			return(rc);
		}
	}

	/*Parse the signal pattern */
	sig_def->rgx_sig.str	= strdup(&(line[mtch_idxs[SRGXS].rm_so]));
	if (sig_def->rgx_sig.str[0]) {
		if (sig_def->sops.bits.parser==PARS_REGEXP) {
			rc=regcomp(
				&(sig_def->rgx_sig.rgx),
				sig_def->rgx_sig.str,
				REG_EXTENDED
			);
			if (rc) {
				regerror(rc, &(sig_def->rgx_sig.rgx), err_str, 80);
				fprintf(stderr, "Regexec compilation failure: %s\n", err_str);
				return(rc);
			}
		} /*else: Nothing needed. Delimiter is already in place */

	} else  if (
		(sig_def->sops.bits.parser==PARS_CUT) ||
		(sig_def->sops.bits.parser==PARS_REVCUT) )
	{
		/* If field empty but pattern right for this parser, fill in field
		 * with default */
		sig_def->rgx_sig.str=PARS_CUT_DEF_DELIM;
	}

	subdef_str=&(line[mtch_idxs[SIDXS].rm_so]);
	sigsub_strlen=strlen(subdef_str);
	if ((sigsub_strlen == 1) && (subdef_str[0] == '$')) {
		sigsub_strlen = 0;
		isS='Y';
	} else
		isS='N';

	/*Parsing of definition ended. Initialization of the work-data
	 * (sig_data/sub_sig) follows */
	sig_data->ownr				= smpl_signal;
	sig_data->backlog_events	= 0;
	sig_data->cleared_backlog	= 0;
	if (!sigsub_strlen) {
		/* Field is not filled in or contained a singe '$', which is a
		 * special case. It corresponds to one sub-signal, matching the
		 * whole signal regex (if existing).  Note that signal-regexp can be
		 * missing, but still have a (the only) sub_signal. That would be
		 * the most common case in sysfs where "one file" = "one value" is
		 * predominant.
		 */
		sig_def->idxs				= calloc(1, sizeof(int));
		assert_ext(sig_def->idxs);
		(sig_def->idxs)[0]			= 0;
		sig_def->issA				= calloc(1+1, sizeof(char));
		assert_ext(sig_def->issA);
		(sig_def->issA)[0]			= isS;
		(sig_def->issA)[1]			= '0';
		sig_data->nsigs				= 1;
		sig_data->sig_sub			= calloc(1, sizeof(struct sig_sub));
		assert_ext(sig_data->sig_sub);
		(sig_data->sig_sub)[0].fd		= -1;
		(sig_data->sig_sub)[0].ownr	= sig_data;
		(sig_data->sig_sub)[0].sub_id	= 0;
		(sig_data->sig_sub)[0].id		= MAX_SIGS * (lno+1);
		(sig_data->sig_sub)[0].fp_curr	= 0;
		(sig_data->sig_sub)[0].fp_was	= 0;
		(sig_data->sig_sub)[0].fp_valid	= 0;
		(sig_data->sig_sub)[0].valS     = NULL;
		(sig_data->sig_sub)[0].isstr  = isS=='Y' ? 1 : 0;
		snprintf( (sig_data->sig_sub)[0].val, VAL_STR_MAX,
			"%s", "NO_START");
		snprintf( (sig_data->sig_sub)[0].presetval, VAL_STR_MAX,
			"%s %d", SIG_VAL_DFLT,(sig_data->sig_sub)[0].id);
		if (sig_def->sops.bits.canblock) {
			(sig_data->sig_sub)[0].daq_type=FREERUN_ASYNCHRONOUS;
		} else if (sig_def->sops.bits.trigger && !sig_def->sops.bits.no_rewind) {
			/* Valid combination for iNotify*/
			sampler_data.files_monitored++;
			(sig_data->sig_sub)[0].daq_type=LOCKSTEP_NOTIFIED;
			(sig_data->sig_sub)[0].sub_function=logfile_fdata;
		} else {
			(sig_data->sig_sub)[0].daq_type=LOCKSTEP_POLLED;
			(sig_data->sig_sub)[0].sub_function=NULL;
		}
	} else {
		/* Parse the sub-signals string. */
		int instr=0;
		char *tptr=subdef_str;
		int nidx=0;

		/* To make separation into array easier, replace every non ASCII
		 * digit with \0. Also take the opportunity to count the number of
		 * sub-signals */
		for (instr=0, i=0, nidx=0; i<sigsub_strlen; i++) {
			if ((tptr[i] >= '0' && tptr[i] <= '9') || tptr[i] == '$') {
				if (!instr) {
					instr=1;
					nidx++;
				}
			} else {
				tptr[i] = 0;
				instr=0;
			}
		}

		sig_data->nsigs = nidx;
		sig_def->idxs = calloc(nidx, sizeof(int));
		memset(sig_def->idxs, 0, nidx*sizeof(int));
		sig_def->issA = calloc(nidx+1, sizeof(char));
		memset(sig_def->issA, 0, (nidx+1)*sizeof(char));
		sig_data->sig_sub	= calloc(nidx, sizeof(struct sig_sub));
		assert_ext(sig_data->sig_sub);
		memset(sig_data->sig_sub, 0, nidx*sizeof(struct sig_sub));

		/* Initialize sub-signal data harvester (data for each work-thread) */
		for (i=0; i<nidx; i++){
			(sig_data->sig_sub)[i].fd		= -1;
			(sig_data->sig_sub)[i].ownr		= sig_data;
			(sig_data->sig_sub)[i].sub_id	= i;
			(sig_data->sig_sub)[i].id		= MAX_SIGS * (lno+1) + i;
			(sig_data->sig_sub)[i].fp_was	= 0;
			(sig_data->sig_sub)[i].fp_curr	= 0;
			(sig_data->sig_sub)[i].fp_valid	= 0;
			(sig_data->sig_sub)[i].valS     = NULL;
			snprintf( (sig_data->sig_sub)[i].val, VAL_STR_MAX,
				"%s", "NO_START");
			snprintf( (sig_data->sig_sub)[i].presetval, VAL_STR_MAX,
				"%s %d", SIG_VAL_DFLT,(sig_data->sig_sub)[i].id);
			if (sig_def->sops.bits.canblock) {
				(sig_data->sig_sub)[i].daq_type=FREERUN_ASYNCHRONOUS;
				if (i==0) {
					if (sig_def->sops.bits.tops < NEVER_THREAD_SUB_SIG) {
						INFO(("WARNING: You have chosen to sub-thread [%s] "
								"If file is pipe or char-device this will most "
								"likely not give the expected behaviour",
							sig_def->name
						));
					}
				}
			} else if (sig_def->sops.bits.trigger && !sig_def->sops.bits.no_rewind) {
				/* Valid combination for iNotify*/
				if (i==0)
					sampler_data.files_monitored++;
				(sig_data->sig_sub)[i].daq_type=LOCKSTEP_NOTIFIED;
				(sig_data->sig_sub)[i].sub_function=logfile_fdata;
			} else {
				(sig_data->sig_sub)[i].daq_type=LOCKSTEP_POLLED;
				(sig_data->sig_sub)[i].sub_function=NULL;
			}
		}

		tptr=subdef_str;

		/* Store sub-match index */
		for (instr=0,i=0,j=0; i<sigsub_strlen; i++) {
			if ((tptr[i] >= '0' && tptr[i] <= '9') || tptr[i] == '$') {
				if (!instr) {
					instr=1;
					if (tptr[i] == '$'){
						(sig_def->issA)[j]='Y';
						(sig_def->idxs)[j]=atoi(&tptr[i+1]);
					} else {
						(sig_def->issA)[j]='N';
						(sig_def->idxs)[j]=atoi(&tptr[i]);
					}
					j++;
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
	char err_str[80];
	regex_t preg; /* Compiled version of definition regex for one line */
	int init_inotify = 0;

	memset(err_str,0,80);
	rc=regcomp(&preg, DFN_LINE, REG_EXTENDED);
	if (rc) {
		regerror(rc, &preg, err_str, 80);
		fprintf(stderr, "Regexec compilation failure("__FILE__":%d): %s\n",
			__LINE__, err_str);
		return(1);
	}
	assert_ext((
		rc=mlist_opencreate(sizeof(struct smpl_signal*), NULL, list)
	)==0);
	assert_ext(list);

	fl=fopen(fn,"r");
	if (fl==NULL) {
		fprintf(stderr,"%s",line);
		perror("Signal description file-error:");
		exit(1);
	}

	for (lno=0,rc=0; rc==0; lno++) {
		rcs=fgets(line, MAX_DFN_LINE_LEN, fl);
		if (rcs){
			int len=strnlen(line, MAX_DFN_LINE_LEN);
			if (len>0 && len<MAX_DFN_LINE_LEN) {
				/* Ignore lines starting with '#' and empty lines*/
				if (
					line[0] != '#' &&
					line[0] != ' ' &&
					line[0] != '\t' &&
					len>0
				) {

					/* Line below: Get rid of the EOL. Not sure why this is
					 * needed as REG_NOTEOL was not given to regexec.
					 * Perhaps miss-understanding standard. Need check TBD
					 * */
					line[len-1]=0;
					tsig=malloc(sizeof(struct smpl_signal));
					assert_ext(tsig);
					rc=parse_dfn_line(&preg, line, tsig, lno);
					if (!rc) {
						/* Add to mlist */
						assert_ext(mlist_add(*list, tsig) != NULL);
					} else {
						/* Handle error */
						/* TBD */
					}
					if (sampler_data.files_monitored>0 &&
						!init_inotify)
					{
						sampler_data.fd_notify = inotify_init();
						init_inotify=0;
					}
				} else {
					if ( line[0] == ' ' || line[0] == '\t')
						DBG_INF(2,("Line %d is starting with white-space "
							"and is ignored. Check your mistake!\n",lno));
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
			perror("Signal description file read error");
			rc = EBADF;
			goto fin_pars_init;
		} else if (feof(fl)) {
			INFO(("Info: Scanned %d lines successfully\n",lno-1));
			rc = 0;
			goto fin_pars_init;
		} else {
			fprintf(stderr,"Error: Scanned line %d in file %s is badly "
				"formatted\n",lno,fn);
			fprintf(stderr,"Offending line is:\n");
			fprintf(stderr,"%s\n",line);
			fprintf(stderr,"Rule pattern:\n");
			fprintf(stderr,"%s\n",DFN_LINE);

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
