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
#include "assert_np.h"
#include <unistd.h>
#include <time.h>
#include <sys/times.h>
#include <stdint.h>
#include <string.h>

/* Include module common stuff */
#include "sampler.h"
#include "sigstruct.h"
#include "local.h"

static char *get_signame(const char *fname, char *buf, int *ret_len) {
	FILE *f;

	f = fopen(fname, "r");
	assert_ext(f);
	if (!f){
		if (ret_len)
			*ret_len = 0;
		return NULL;
	}
	memset(buf, 0, NAME_STR_MAX);
	fgets(buf, NAME_STR_MAX, f);
	fclose(f);

	/* fgets should have done this already, but just to be
	 * safe: */
	buf[NAME_STR_MAX-1]='\0';

	/* Get rid of last LF is such exists */
	if (buf[strlen(buf) -1] == '\n')
		buf[strlen(buf) -1] = '\0';

	if (ret_len)
		*ret_len = strlen(buf);

	if (strlen(buf)) {
		return buf;
	}
	return NULL;
}

/* Output first line describing the contents of each columns. These strings
 * will be be used as legends in plotting software */
void outputlegends( void ) {
	int j,from_file=0;
	struct node *np;
	struct sig_sub *sig_sub;
	struct sig_data *sig_data;
	struct sig_def *sig_def;
	struct smpl_signal *smpl_signal;
	char *tname;
	char tname_buff[NAME_STR_MAX];

	/* All modglobals are finalized by now. It's safe to use them */
	handle_t list = samplermod_data.list;

	if (samplermod_data.dolegend) {
		printf("Time-now%cTSince-last",
				samplermod_data.delimiter);

		for(np=mlist_head(list); np; np=mlist_next(list)){
			assert(np->pl);

			smpl_signal=(struct smpl_signal *)(np->pl);
			sig_data=&((*smpl_signal).sig_data);
			sig_def=&((*smpl_signal).sig_def);

			if (sig_def->fname && strnlen(sig_def->fname, NAME_STR_MAX)) {
				from_file = 1;
				tname =sig_def->fname;
			} else {
				from_file = 0;
				tname =sig_def->name;
			}

			assert_ext(tname && strnlen(tname, NAME_STR_MAX) &&
				("Symbolic name and file where to get it from can't both be nil" !=
				 NULL)
			);

			if (sig_data->nsigs > 1) {
				for(j=0; j<sig_data->nsigs; j++){
					sig_sub=&((sig_data->sigs)[j]);
				}
			} else {
				if (from_file) {
					printf("%c%s",
						samplermod_data.delimiter,
						get_signame(
							tname,tname_buff, NULL)
					);
				} else {
					printf("%c%s",
						samplermod_data.delimiter,
						tname
					);
				}
			}
		}

		printf("\n");
		fflush(stdout);
	}
}

/* Output in format according to settings */
void output(int cid, const char *val, int always ) {
	int wa = 0;
	static const char *fakeval = "0";
	const char *v = val;
	volatile static int face_cntr = 0;
	volatile static int wut_cntr = 0;

	face_cntr++;
	wut_cntr++;
	if ( always && !(val[0]>='-' && val[0]<='z') ){
		if (always != 1) {
			fprintf(stderr, "ERROR: must print but have nothing"
				" to fall-back on. Faking it... %d:%d\n",face_cntr,cid);
		/* This is a clear error indication of the framework, but don't
		 * assert for now as it else makes it even harder to see and debug
		 * with feedgnuplot */
		} else {
			fprintf(stderr, "ERROR: Wut had you been smoking when you wrote "
				"["__FILE__"] !!!? [%d:%d]\n",wut_cntr,cid);
		/* Impossible combo */
		}
		wa = 1;
		v = fakeval;
	}
	switch (samplermod_data.plotmode) {
		case driveGnuPlot:
			fputc('0'+cid+samplermod_data.cid_offs,stdout);
			fputc(':',stdout);
			fputs(v,stdout);
			fputc('\n',stdout);
			break;
		case feedgnuplot:
		default:
			fputc(samplermod_data.delimiter,stdout);
			fputs(v,stdout);
	}
}

/* What to do if data isn't updated. Both drivegnuplot and feedgnuplot
 * currently get their streams confused if there's no data for a stream and
 * we in such case we need to output something (preferred would be
 * outputting nothing and to also get a corresponding break in the
 * continuity of the plot) */
static void ondataempty(int cid, const struct sig_sub* sig_sub) {
	switch (samplermod_data.whatTodo) {
		case Lastval:
			output(cid, sig_sub->val ,1);
			break;
		case PresetSigStr:
			output(cid, sig_sub->presetval, 1);
			break;
		case PresetSmplStr:
			output(cid, samplermod_data.presetval ,1);
			break;
		case Nothing:
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
		assert(np->pl);

		smpl_signal=(struct smpl_signal *)(np->pl);
		sig_data=&((*smpl_signal).sig_data);
		for(j=0; j<sig_data->nsigs; j++){
			sig_sub=&((sig_data->sigs)[j]);
			sig_sub->val[VAL_STR_MAX-1] = '\0';
			if (sig_sub->is_updated) {
				output(j, sig_sub->val, 2);
			} else {
				ondataempty(j,sig_sub);
			}
			sig_sub->is_updated=0;
		}
	}
	if (!samplermod_data.plotmode == driveGnuPlot) {
		fputc('\n',stdout);
	}
}

/* Harvest finished sample and print it */
void harvest_sample(const handle_t list) {
		static struct timeval tlast = {0,0};
		static int first = 1;
		struct timeval tnow1,tnow2;
		struct timeval tdiff;
		int uComp;
		struct timeval tv_comp;
		struct timespec hr_tnow;

		assert_ext(time_now(&hr_tnow) == 0);
		/* Down-scale. Kernel log doesn't show better res than us anyway.
		 * int-div can't be more costly than outputting 3 extra characters.
		 * Might want higher resolution for time-keeping later though, which
		 * should give more precise time calculation and minimize (positive)
		 * drift when jitter-compensate. */
		tnow1.tv_sec = hr_tnow.tv_sec;
		tnow1.tv_usec = hr_tnow.tv_nsec/1000;

		if (first) {
			first = 0;
			tlast=tnow1;
		}

		tdiff=tv_diff(tlast,tnow1);
		if (samplermod_data.plotmode == driveGnuPlot) {
			printf("0:%d.%06d\n1:%d.%06d\n",
				SEC(tnow1),USEC(tnow1),SEC(tdiff),USEC(tdiff));
		} else {
			printf("%d.%06d%c%d.%06d",
				SEC(tnow1),USEC(tnow1),
				samplermod_data.delimiter,
				SEC(tdiff),USEC(tdiff));
		}

		collect_and_print(list);
		++samplermod_data.smplcntr;

		assert_ext(time_now(&hr_tnow) == 0);
		tnow2.tv_sec = hr_tnow.tv_sec;
		tnow2.tv_usec = hr_tnow.tv_nsec/1000;
		tdiff=tv_diff(tlast,tnow2);

		tv_comp = tv_diff(
			(struct timeval){0,samplermod_data.ptime},
			tdiff
		);
		uComp = USEC(tv_comp);

		if (uComp > samplermod_data.ptime)
			uComp = 1000000 - uComp;

		tlast = tnow2;
		//assert(uComp<samplermod_data.ptime);
		if (uComp>=samplermod_data.ptime) {
#ifdef never
			/* Don't warn. Disturbs time even more */
			fprintf(stderr,"Cant compensate jitter: %d us (%d.%05d)\n",
			uComp,SEC(tdiff),USEC(tdiff));
#endif
			uComp=0;
		}
		usleep(samplermod_data.ptime/*-uComp*/);
}

