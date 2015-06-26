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

/* This file contains various helper functions to keep track of, and handle,
 * time */

/* NOTE: !!! These functions are in need of thorough testing !!! (TBD) */

#define LDATA struct smpl_signal
#include <mlist.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <time.h>
#include <sys/times.h>
#include <stdint.h>
#include <string.h>

/* Include module common stuff */
#include "local.h"

/* Returns diff-time of two 'struct timeval' types
 * Note: Order matters. t0 means the first event, t1 the second on the same
 * time-line. If function is used for a generic diff of two times and the
 * actual time for the t1 come before t0, this function returns a negative
 * time */
struct timeval tv_diff(struct timeval t0, struct timeval t1) {
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

/* Adds time of two 'struct timeval' types */
struct timeval tv_add(struct timeval t0, struct timeval t1) {
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

/* Return "time-now" in best available format according to modality.
 * Currently using either:
 *  int clock_gettime(clockid_t clock_id, struct timespec *tp);
 *  int gettimeofday(struct timeval *restrict tp, void *restrict tzp);
 *
 * Both represent time in some "absolute" form. What's most interesting is a
 * time-representation as close to kernel-time as possible, in which case
 * clock_gettime(CLOCK_MONOTONIC_RAW) would be preferred. This however
 * either isn't available at all systems or requires root-privileges.
 * Therefore as first fall-back: CLOCK_MONOTONIC is used and as second
 * fall-back: calender-time in form of clock_gettime is used.
 *
 * Note however that all SW clocks, even global ones, are drift compensated
 * which is a disadvantage if a sample is to be compared with a kernel
 * log-entry event (except CLOCK_MONOTONIC_RAW i.e. which is a drift
 * uncompensated clock) */
int time_now(struct timeval *tv) {
	int rc;
	struct timespec tp;

	switch (sampler_setting.clock_type) {
		case KERNEL_CLOCK:
#ifdef CLOCK_MONOTONIC_RAW
			rc=clock_gettime(CLOCK_MONOTONIC_RAW, &tp);
#else
#warning Target does not support CLOCK_MONOTONIC_RAW
			rc=clock_gettime(CLOCK_MONOTONIC, &tp);
#endif
			break;
		case CALENDER_CLOCK:
			rc=clock_gettime(CLOCK_REALTIME, &tp);
			//Alternatively clock_gettimeofday can be used but with lower
			//resolution.
			break;
		case AUTODETECT:
		default:
			fprintf(stderr,"clock-type not supported\n");
			rc=-1;
	}
	if (rc==0) {
			/* Down-scale. Kernel log doesn't show better res than us anyway.
			 * int-div can't be more costly than outputting 3 extra characters.
			 * Might want higher resolution for time-keeping later though, which
			 * should give more precise time calculation and minimize (positive)
			 * drift when jitter-compensate. */
			tv->tv_sec = tp.tv_sec;
			tv->tv_usec = tp.tv_nsec/1000;
	}
	return rc;
}
