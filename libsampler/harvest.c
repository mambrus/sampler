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
#include <unistd.h>
#include <time.h>
#include <sys/times.h>
#include <stdint.h>

/* Include module common stuff */
#include "sampler.h"
#include "sigstruct.h"
#include "local.h"

#define SEC( TV ) ((int)TV.tv_sec)
#define USEC( TV ) ((int)TV.tv_usec)

static struct timeval tv_diff( struct timeval t0, struct timeval t1 ) {
	struct timeval tv;

	/* We don't know the scalar base-types. Therefore creating extra
	 * temp of known type and let the cast occur in two steps */
	int32_t t0_us = t0.tv_usec;
	int32_t t1_us = t1.tv_usec;
	int32_t tr_us;

	tv.tv_sec = t1.tv_sec - t0.tv_sec;
	tr_us = t1_us - t0_us;
	if (tr_us<0 ){
		tv.tv_sec--;
		tv.tv_usec = 1000000l + tr_us;
	} else
		tv.tv_usec = tr_us;

	return tv;
}

static struct timeval tv_add( struct timeval t0, struct timeval t1 ) {
	struct timeval tv;

	/* We don't know the scalar base-types. Therefore creating extra
	 * temp of known type and let the cast occur in two steps */
	int32_t t0_us = t0.tv_usec;
	int32_t t1_us = t1.tv_usec;
	int32_t tr_us;

	tv.tv_sec = t1.tv_sec + t0.tv_sec;
	tr_us = t1_us + t0_us;
	if (tr_us>=1000000){
		tv.tv_sec++;
		tv.tv_usec = tr_us - 1000000l;
	} else
		tv.tv_usec = tr_us;

	return tv;
}

static void collect_and_print(const handle_t list){
	int rc,i,j;
	struct node *n;
	struct sig_sub *sig_sub;
	struct sig_data *sig_data;
	struct smpl_signal *smpl_signal;

	for(n=mlist_head(list); n; n=mlist_next(list)){
		assert(n->pl);

		smpl_signal=(struct smpl_signal *)(n->pl);
		sig_data=&((*smpl_signal).sig_data);
		for(j=0; j<sig_data->nsigs; j++){
			sig_sub=&((sig_data->sigs)[j]);
			sig_sub->val[VAL_STR_MAX] = 0;
			fputc(';',stdout);
			fputs(sig_sub->val,stdout);
		}
	}
	fputc('\n',stdout);
	//fflush(stdout);
}

/* Harvest finished sample and print it */
void harvest_sample(const handle_t list){
		static struct timeval tlast = {0,0};
		static int first = 1;
		struct timeval tnow1,tnow2;
		struct timeval tdiff;
		int uComp;
		struct timeval tv_comp;

		gettimeofday(&tnow1, NULL);
		if (first) {
			first = 0;
			tlast=tnow1;
		}

		tdiff=tv_diff(tlast,tnow1);

		printf("%d.%06d;%d.%06d",
			SEC(tnow1),USEC(tnow1),SEC(tdiff),USEC(tdiff));

		collect_and_print(list);
		++samplermod_data.smplcntr;

		gettimeofday(&tnow2, NULL);
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
		usleep(samplermod_data.ptime-uComp);
}

