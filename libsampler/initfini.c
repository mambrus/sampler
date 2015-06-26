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

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <assert.h>
#include "assert_np.h"
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <sampler.h>
#include <mqueue.h>
#include "local.h"

/* Module initializers. Not much to do here for init, but fini might have
 * clean-up to do, iterate through the list and:
 * * kill threads
 * * Free allocated memory
 */
void __init __sampler_init(void) {
#ifdef INITFINI_SHOW
	fprintf(stderr,">>> Running module _init in ["__FILE__"]\n"
			">>> using CTORS/DTORS mechanism ====\n");
#endif
	assert_ext(!sampler_data.isinit);
	sampler_data.diag.smplID=0;
	sampler_data.diag.triggID=0;
	sampler_data.diag.texec=0;
	sampler_data.diag.tp1=0;
	sampler_data.diag.tp2=0;

	/* TBD: This part is only set temporarily to test feature. This should be
	 * replaced parsed options */
	sampler_data.diag.format_ary[0] = EXEC_TIME;
	sampler_data.diag.format_ary[1] = SMPL_ID;
	sampler_data.diag.format_ary[2] = TRIG_BY_ID;
	sampler_data.diag.format_ary[3] = PERIOD_TIME_BEGIN;
	sampler_data.diag.format_ary[4] = PERIOD_TIME_HARVESTED;
	/*END TBD*/

	if (sampler_setting.clock_type == AUTODETECT) {
		struct timespec tv;
#ifdef CLOCK_MONOTONIC_RAW
		if (clock_gettime(CLOCK_MONOTONIC_RAW, &tv) == 0)
			sampler_setting.clock_type = KERNEL_CLOCK;
#else
#  warning CLOCK_MONOTONIC_RAW undefined.
#  warning Best aproximation of kernel-time will be using CLOCK_MONOTONIC
		if (clock_gettime(CLOCK_MONOTONIC, &tv) == 0)
			sampler_setting.clock_type = KERNEL_CLOCK;
#endif
		else
			sampler_setting.clock_type = CALENDER_CLOCK;
	}
	assert_ext(time_now(&sampler_data.tstarted) == 0);
	DBG_INF(4,("Unlinking any old master queue (if used). \n"));
	mq_unlink(LOCKSTEP_Q);


	sampler_setting.procsubst.env=calloc(1, sizeof(char**));
	sampler_setting.procsubst.env[0]=NULL;
	sampler_setting.procsubst.path=calloc(1, sizeof(char**));
	sampler_setting.procsubst.path[0]=NULL;
	sampler_setting.tmpdir=malloc(PATH_MAX);
	strncpy(sampler_setting.tmpdir, xstr(SAMPLER_TMPDIR), PATH_MAX);
	sampler_data.isinit=1;
	sampler_setting.isinit=1;
}

void __fini __sampler_fini(void) {
#ifdef INITFINI_SHOW
	fprintf(stderr,">>> Running module _fini in ["__FILE__"]\n"
			">>> using CTORS/DTORS mechanism\n");
#endif
	if (!sampler_data.isinit)
		/* Someone already did this in a more controlled way. Nothing to do
		 * here, return */
		 return;
	self_destruct(sampler_data.list);
	DBG_INF(4,("Unlinking master queue. \n"));
	mq_unlink(LOCKSTEP_Q);
	free(sampler_setting.procsubst.env);
	free(sampler_setting.procsubst.path);
	free(sampler_setting.tmpdir);
	sampler_data.isinit=0;
	sampler_setting.isinit=0;
}
