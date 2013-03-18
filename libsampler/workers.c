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

/* This file contains worker threads (there might be several different
 * depending on type instead of having logic "in thread". Separated threads
 * are faster an logic according to setting is only needed to be evaluated
 * once */

#include <pthread.h>
#include <semaphore.h>
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


/* Simple worker thread to produce test-data */
void *sinus_worker_thread(void* inarg) {
	struct sig_sub *sig_sub = inarg;
	struct sig_data *sig_data = sig_sub->ownr;
	struct sig_def *sig_def = &(sig_sub->ownr->ownr->sig_def);

	memset(sig_sub->val,0,VAL_STR_MAX);
	snprintf(sig_sub->val,VAL_STR_MAX,"%d",0);
	int rc,cnt = 0;

	while(1) {
		INFO(("--> Worker %d starts (nr: %d for line %d)\n",
					sig_sub->id,
					sig_sub->sub_id,
					sig_def->id));

		INFO(("--> Worker %d will wait for sync...\n",sig_sub->id));
		inc_waiting1(1);
		/* Main sync point. Wait here. */
		assert_ext(sem_wait(&workers_start_barrier) == 0);
		inc_waiting1(-1);
		INFO(("--> Worker %d in sync. Continues...\n",sig_sub->id));

		DUSLEEP(MEDIUM);

		/* Clear last value */
		memset(sig_sub->val,0,VAL_STR_MAX);

		rc=sinus_data(sig_sub, cnt);

		if (!rc) {
			cnt++;

			sig_sub->is_updated=1;
			/*Tell master one more is finished*/
			INFO(("--> Worker %d finished work waiting for buddies...\n",
				sig_sub->id));
		}

		/*Wait for buddies to catch-up before letting master update*/
		inc_waiting2(1);
		/*Catch-up sync point here.*/
		assert_ext(sem_wait(&workers_end_barrier) == 0);
		inc_waiting2(-1);

		/*Tell master one more is finished*/
		INFO(("--> Worker %d all done!\n",sig_sub->id));
	}
}

/* Simple worker thread. As in-data it takes it's own list-slot (i.e. no
 * need to search and no concurrency aspects. It knows nothing about when to
 * run, the poll_master dictates each run. */
void *poll_worker_thread(void* inarg) {
	struct sig_sub *sig_sub = inarg;
	struct sig_data *sig_data = sig_sub->ownr;
	struct sig_def *sig_def = &(sig_sub->ownr->ownr->sig_def);

	memset(sig_sub->val,0,VAL_STR_MAX);
	snprintf(sig_sub->val,VAL_STR_MAX,"%d",0);
	int rc,cnt = 0;

	while(1) {
		INFO(("--> Worker %d starts (nr: %d for line %d)\n",
					sig_sub->id,
					sig_sub->sub_id,
					sig_def->id));

		INFO(("--> Worker %d will wait for sync...\n",sig_sub->id));
		inc_waiting1(1);
		/* Main sync point. Wait here. */
		assert_ext(sem_wait(&workers_start_barrier) == 0);
		inc_waiting1(-1);
		INFO(("--> Worker %d in sync. Continues...\n",sig_sub->id));

		DUSLEEP(MEDIUM);

		/* Clear last value */
		memset(sig_sub->val,0,VAL_STR_MAX);

		rc=poll_fdata(sig_sub, cnt);
		assert(rc==0);

		if (!rc) {
			cnt++;

			sig_sub->is_updated=1;
			/*Tell master one more is finished*/
			INFO(("--> Worker %d finished work waiting for buddies...\n",
				sig_sub->id));
		}

		/*Wait for buddies to catch-up before letting master update*/
		inc_waiting2(1);
		/*Catch-up sync point here.*/
		assert_ext(sem_wait(&workers_end_barrier) == 0);
		inc_waiting2(-1);

		/*Tell master one more is finished*/
		INFO(("--> Worker %d all done!\n",sig_sub->id));
	}
}
