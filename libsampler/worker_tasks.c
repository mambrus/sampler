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

/* This is a test-task to produce measurements for the samplers own
 * performance and impact */
int produce_sinus_data(struct sig_sub* sig_sub, int cnt) {
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
	snprintf(sig_sub->val,VAL_STR_MAX,"%f",
		sin(x)+((float)sig_sub->id/10000.0))+\
		((float)(sig_sub->id%10)/100.0);

	return 0;
}

