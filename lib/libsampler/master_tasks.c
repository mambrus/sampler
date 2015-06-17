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
#define LDATA struct smpl_signal
#include <mlist.h>
#include <stdio.h>
#include <assert.h>
#include <assure.h>
#include <unistd.h>
#include <time.h>
#include <sys/times.h>
#include <stdint.h>
#include <string.h>
#include <mqueue.h>
#include <errno.h>

/* Include module common stuff */
#include "sampler.h"
#include "sigstruct.h"
#include "local.h"

/* Simple sanity-check, covers the most common case: the empty-string */
#define HAS_VALID_DATA (val != NULL && (val[0]>='-' && val[0]<='z'))

/* Output in format according to settings */
static void output(int cid, const char *val, enum nuce forceprint ) {
	static const char *fakeval = "0xCACACAFE";
	const char *v = val;
	static volatile int face_cntr = 0;
	static volatile int wut_cntr = 0;

	face_cntr++;
	wut_cntr++;
	if ( !HAS_VALID_DATA && forceprint ){
		if (forceprint == NOTHING) {
			fprintf(stderr, "ERROR: must print but have nothing"
				" to fall-back on. Faking it... %d:%d\n",face_cntr,cid);
		/* This is a clear error indication of the framework, but don't
		 * assert for now as it else makes it even harder to see and debug
		 * with feedgnuplot */
			v = fakeval;
		} else {
			INFO(("WARNING: Output-value is faked [%d]\n",forceprint));
		}
	}
	switch (sampler_setting.plotmode) {
		case driveGnuPlot:
			fputc('0'+cid+sampler_data.cid_offs,stdout);
			fputc(':',stdout);
			fputs(v,stdout);
			fputc('\n',stdout);
			break;
		case feedgnuplot:
		default:
			fputc(sampler_setting.delimiter,stdout);
			fputs(v,stdout);
	}
}

/* What to do if data isn't updated. Both drivegnuplot and feedgnuplot
 * currently get their streams confused if there's no data for a stream and
 * we in such case we need to output something (preferred would be
 * outputting nothing and to also get a corresponding break in the
 * continuity of the plot) */
static void ondataempty(int cid, const struct sig_sub* sig_sub) {
	switch (sig_sub->ownr->ownr->sig_def.nuce) {
		case LASTVAL:
			if (!sig_sub->isstr) {
				output(cid, sig_sub->val, LASTVAL);
			} else {
				assert_ext("A value of type string can't repeat previous values"
					== NULL);
			}
			break;
		case PRESET_SIG:
			output(cid, sig_sub->presetval, PRESET_SIG);
			break;
		case PRESET_SMPL:
			output(cid, sampler_setting.presetval, PRESET_SMPL);
			break;
		case NOTHING:
		default:
			output(cid, "", 0);
	}
}

static void collect_and_print(const handle_t list){
	int rc,i,j;
	struct node *np;
	struct sig_sub *sig_sub;
	struct sig_data *sig_data;
	struct smpl_signal *smpl_signal;

	for(np=mlist_head(list); np; np=mlist_next(list)){
		assert_ext(np->pl);

		smpl_signal=(struct smpl_signal *)(np->pl);
		sig_data=&((*smpl_signal).sig_data);
		for(j=0; j<sig_data->nsigs; j++){
			sig_sub=&((sig_data->sig_sub)[j]);
			if (sig_sub->daq_type == FREERUN_ASYNCHRONOUS)
				pthread_mutex_lock(&sig_sub->lock);
			if (sig_sub->val_pipe) {
				struct val_msg val_msg;
				int rc;
				unsigned prio;

				rc=mq_receive(sig_sub->val_pipe->read,
					(char*)&val_msg,
					sizeof(struct val_msg), &prio);

				if ((rc==(mqd_t)-1) && (errno==EAGAIN)) {
					ondataempty(j,sig_sub);
				} else if (rc==(mqd_t)-1) {
					fprintf(stderr,"errno=%d\n",errno);
					fflush(stderr);
					perror(strerror(errno));
					assert_ext("This is bad!" == NULL);
				} else {
					if (sig_sub->isstr) {
						output(j, val_msg.valS, NOTHING);
						free(val_msg.valS);
					} else {
						output(j, val_msg.valS, NOTHING);
						free(val_msg.valS);
					}
				}
			} else {
				if (sig_sub->is_updated) {
					if (sig_sub->isstr) {
						output(j, sig_sub->valS, NOTHING);
						free(sig_sub->valS);
					} else {
						sig_sub->val[VAL_STR_MAX-1] = '\0';
						output(j, sig_sub->val, NOTHING);
					}
				} else {
					ondataempty(j,sig_sub);
				}
			}
			sig_sub->is_updated=0;
			if (sig_sub->daq_type == FREERUN_ASYNCHRONOUS)
				pthread_mutex_unlock(&sig_sub->lock);
		}
	}
	if (!sampler_setting.plotmode == driveGnuPlot) {
		fputc('\n',stdout);
	}
}

/* Check if spurious run.
   This will help filter away samples made in vain because file-monitor
   fired of events which led to no match. If all MONITOR tasks did not
   produce any valid data AND this was not rate-monotonic run, output will
   be silenced. Works currently only for pure event-driven usage as SAMPLER
   can't detect the difference of the reason for running each iteration.
   Need mqueues for that.

   TBD Don't we have that now (1400524)? Check if this test is still needed
 */
int certified_run(const handle_t list) {
	int rc,i,j;
	struct node *np;
	struct sig_sub *sig_sub;
	struct sig_data *sig_data;
	struct smpl_signal *smpl_signal;
	int num_umon=0; /*Number of monitors with new data */

	if (sampler_data.files_monitored==0) {
		/* This is not a pure event-driven configuration */
		return 1;
	}

	for(np=mlist_head(list); np; np=mlist_next(list)){
		assert_ext(np->pl);

		smpl_signal=(struct smpl_signal *)(np->pl);
		sig_data=&((*smpl_signal).sig_data);
		for(j=0; j<sig_data->nsigs; j++){
			sig_sub=&((sig_data->sig_sub)[j]);
			sig_sub->val[VAL_STR_MAX-1] = '\0';
			if ((sig_sub->is_updated) &&
				(sig_sub->daq_type==LOCKSTEP_NOTIFIED))
			{
				num_umon++;
			}
		}
	}
	return num_umon;
}

/* Print a integer that is in uS in seconds with 6 fixed fractions and
 * correctly signed. This macro is specific to snprintf_fdiag
 * */
#define PRINT_uS_TIME_VALUE( T )                                        \
    tval   = T;                                                         \
    is_neg = tval<0;0;1;                                                \
    tval   = abs(tval);                                                 \
                                                                        \
    if (is_neg)                                                         \
        n=snprintf(&S[i],sz-i-tot,"%c-%d.%06d",                         \
            sampler_setting.delimiter, tval/1000000, tval%1000000);     \
    else                                                                \
        n=snprintf(&S[i],sz-i-tot,"%c%d.%06d",                          \
            sampler_setting.delimiter, tval/1000000, tval%1000000);     \

/* Print formatted diagnostics to passed-by-reference out string 'S' of max
 * size 'sz'. Variable 'where' is a index telling where in the string to start.
 *
 * Returns total characters printed.
 * */
static int snprint_fdiag(char *S, int sz, int where) {
	int c,i,n;
	int tot=0;

	/* 'format_ary' never changes after launch. I.e. safe to use without locks */
	for (
		n=0, i=where, c=0;
		sampler_data.diag.format_ary[c] && i<DIAG_MAX_SLEN;
		i+=n, tot+=n, c++
	) {
		assert_ext(c < DIAG_MAX_COLS);
		BEGIN_RD(&sampler_data.diag_lock)
			int tval;
			int is_neg;

			switch (sampler_data.diag.format_ary[c]) {
				case EXEC_TIME:
					PRINT_uS_TIME_VALUE(sampler_data.diag.texec);
					break;
				case PERIOD_TIME_BEGIN:
					PRINT_uS_TIME_VALUE(
						sampler_data.diag.tp1);
					break;
				case PERIOD_TIME_HARVESTED:
					PRINT_uS_TIME_VALUE(sampler_data.diag.tp2);
					break;
				case SMPL_ID:
					n=snprintf(&S[i],sz-i-tot,"%c%d",
						sampler_setting.delimiter,
						sampler_data.diag.smplID);
					break;
				case TRIG_BY_ID:
					n=snprintf(&S[i],sz-i-tot,"%c%d",
						sampler_setting.delimiter,
						sampler_data.diag.triggID);
					break;
				default:
					n=snprintf(&S[i],sz-i-tot,"%cFORMAT-ERROR",
						sampler_setting.delimiter);
			}
		END_RD(&sampler_data.diag_lock)
	}
	return tot;
}

/* Harvest finished sample and print it */
void harvest_sample(const handle_t list) {
	if (certified_run(list)) {
		static struct timeval t1last = {0,0};
		static struct timeval t2last = {0,0};
		static int first = 1;
		struct timeval tnow;
		struct timeval tstart;
		char diagS[DIAG_MAX_SLEN];
		int diagS_idx=0;

		BEGIN_RD(&sampler_data.sampler_data_lock)
			tstart=sampler_data.tstarted;
		END_RD(&sampler_data.sampler_data_lock)

		BEGIN_WR(&sampler_data.diag_lock)
			assert_np(time_now(&tnow) == 0);
			sampler_data.diag.texec=tv_diff_us(tstart,tnow);

			if (first) {
				first = 0;
			} else {
				sampler_data.diag.tp2=tv_diff_us(t2last,tnow);
				sampler_data.diag.tp1=tv_diff_us(t1last,tstart);
			}

		END_WR(&sampler_data.diag_lock);
		t2last = tnow;
		t1last = tstart;

		if (sampler_setting.plotmode == driveGnuPlot) {
			printf("0:%d.%06d\n1:%d.%06d\n",
				SEC(tstart),
				USEC(tstart),

				sync_get(&sampler_data.diag_lock,
					&sampler_data.diag.tp2)/1000000,
				sync_get(&sampler_data.diag_lock,
					&sampler_data.diag.tp2)%1000000
			);
		} else {
			diagS_idx=snprintf     ( diagS, DIAG_MAX_SLEN, "#0");
			diagS_idx=snprint_fdiag( diagS, DIAG_MAX_SLEN, diagS_idx)+diagS_idx;
			assert_ext(diagS_idx<DIAG_MAX_SLEN);
			printf("%d.%06d%c{%s}",
				SEC(tstart),
				USEC(tstart),
				sampler_setting.delimiter,
				diagS
			);
		}

		collect_and_print(list);
		/* Current ID is changed last of all. I.e. it's the current ID
		   for next iteration and is for a short time not accurate */
		sync_add(&sampler_data.diag_lock, &sampler_data.diag.smplID, 1);
	}
}

