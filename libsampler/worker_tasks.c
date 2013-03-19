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

/* Worker tasks go  in this file (i.e. what they actually do), there might
 * be different variations of tasks, unless they become too many all will be
 * kept here. */

#define LDATA struct smpl_signal
#include <mlist.h>
#include <stdio.h>
#include <assert.h>
#include "assert_np.h"
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <error.h>
#include <errno.h>

/* Include module common stuff */
#include "sampler.h"
#include "sigstruct.h"
#ifndef DBGLVL
#define DBGLVL 0
#endif

#ifndef TESTSPEED
#define TESTSPEED 5
#endif
#include "local.h"

#ifndef SIMULATE_WTASK_BLOCKED_ERROR
#define SIMULATE_WTASK_BLOCKED_ERROR 0
#endif

#define BUF_MAX 1024
#define MAX_SUBLINES 1024
#define MAX_MTCH_PERLINE 16

int sinus_data(struct sig_sub* sig_sub, int cnt) {
	float x;
	float fakt = 10.0*1000000.0/(float)samplermod_data.ptime;

	x=(cnt/fakt/*100.0*/)*2.0*3.1415;
#if SIMULATE_WTASK_BLOCKED_ERROR == 1
	/* Simulate blocking error */
	if (sig_sub->id == 2 && samplermod_data.smplcntr==30) {
		sleep(200);
	}
#endif
	sig_sub->rtime.tv_sec=sig_sub->id;
	snprintf(sig_sub->val, VAL_STR_MAX, "%f",
		sin(x)+((float)sig_sub->id/10000.0))+\
		((float)(sig_sub->id%10)/100.0);

	return 0;
}


/* Normal read-file function. Read interpret data from file. Very simplified
 * version, no lseek, no block/unblock-driving events e.t.a. */
int poll_fdata(struct sig_sub* sig_sub, int cnt) {
	struct sig_data* sig_data = sig_sub->ownr;
	struct sig_def* sig_def = &sig_data->ownr->sig_def;
	int rc;
	FILE *tfile;
	char buf[BUF_MAX];

	if (sig_data->fd < 0) {
		sig_data->fd=open(sig_def->fdata, O_RDONLY);
		rc=errno;
		//assert(sig_data->fd != -1);
		if (sig_data->fd == -1) {
			perror("Can't open data-file for read");
			return rc;
		}
	}

	rc = lseek(sig_data->fd, 0, SEEK_SET);
	if (rc < 0) {
		sig_data->fd=-1;
		rc=errno;
		if (sig_data->fd == -1) {
			perror(	"Warning: Data-file has stopped existing or is renamed"
					"signal is lost, will try again next period.");
			return rc;
		}
	}

	tfile = fdopen(sig_data->fd, "r");
	if ( !sig_def->rgx_line.str ||
		 strnlen(sig_def->rgx_line.str, BUF_MAX) == 0)
	{
		/* Simple case. Whole file or first line contains data */
		/* Need to replace this with read for whole file parsing if special
		 * cases are satisfied (TBD) */
		fgets(buf, BUF_MAX, tfile);
	} else {
		int i,j,k = sig_def->lindex, found=0;
		for (i=0,j=0; i<MAX_SUBLINES && !found; i++) {
			fgets(buf, BUF_MAX, tfile);
			if (regexec(&sig_def->rgx_line.rgx, buf, 0, NULL, 0) == 0) {
				/* There is a match, but is it the n'th one? */
				j++;
				found = (j==k);
			}
		}
	}

	if ( !sig_def->rgx_sig.str ||
		 strnlen(sig_def->rgx_sig.str, BUF_MAX) == 0)
	{
		/* Simplest case. Whole line contains data */
		/* Get rid of last LF is such exists */
		if (buf[strlen(buf) -1] == '\n')
			buf[strlen(buf) -1] = '\0';
		strncpy(sig_sub->val, buf, VAL_STR_MAX);
	} else {
		int rc;
		regmatch_t mtch_idxs[MAX_MTCH_PERLINE+1];
		char err_str[80];

		rc=regexec(&sig_def->rgx_sig.rgx, buf, MAX_MTCH_PERLINE+1, mtch_idxs, 0);
		if (rc) {
			regerror(rc, &sig_def->rgx_sig.rgx, err_str, 80);
			fprintf(stderr, "Regexec faliure: %s\n", err_str);
			return(rc);
		} else {
			/*Note: The correct idx to match against is stored in sig_def*/
			int idx = sig_def->idxs[sig_sub->sub_id];
			strncpy(
				sig_sub->val,
				&buf[mtch_idxs[idx].rm_so],
				mtch_idxs[idx].rm_eo - mtch_idxs[idx].rm_so);
		}
	}

	/*Close the file but not the descriptor*/
	fclose(tfile);
	sig_data->fd = -1;
	return 0;
}
