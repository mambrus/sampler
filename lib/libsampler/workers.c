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
#include <assure.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <limits.h>

/* Include module common stuff */
#include "sampler.h"
#include <syncvar.h>
#include "sigstruct.h"
#ifndef VERBOSE_TYPE
#define VERBOSE_TYPE 0
#endif

#include "local.h"
#define MSGSIZE sizeof(struct lockstep_msg)

/* Exit handler for worker threads */
static void poll_lockstep_worker_thread_exit(void* inarg);

struct poll_lockstep_cancel {
	void *inarg;     /* The original thread argument */
	mqd_t *q;
	struct lockstep_msg *m;
};


/* Simple worker thread. As in-data it takes it's own list-slot (i.e. no
 * need to search and no concurrency aspects. It knows nothing about when to
 * run, the poll_master dictates each run. */
void *poll_lockstep_worker_thread(void* inarg) {
	struct sig_sub *sig_sub = inarg;
	struct sig_data *sig_data = sig_sub->ownr;
	struct sig_def *sig_def = &(sig_sub->ownr->ownr->sig_def);
	char lastval[VAL_STR_MAX];
	int rc,cnt = 0;
	mqd_t q;
	struct lockstep_msg m;
	struct poll_lockstep_cancel poll_lockstep_cancel;

	memset(sig_sub->val,0,VAL_STR_MAX);
	snprintf(sig_sub->val,VAL_STR_MAX,"%d",0);

	if ((sampler_setting.prio_workers != INT_MAX) ||
	    (sampler_setting.policy_workers != INT_MAX))
	{
		assert_ext(!set_thread_prio(sampler_setting.policy_workers,
			sampler_setting.prio_workers));
	}
	/* Use second barrier to make sure master is first up and has set up queue
	 * properly before workers continuing trying to open it. */
	assert_ext(sem_wait(&sampler_data.workers_barrier_2) == 0);

	assert_ext((q = mq_open( LOCKSTEP_Q, O_WRONLY, 0, NULL )) != (mqd_t)-1);
	poll_lockstep_cancel.inarg=inarg;
	poll_lockstep_cancel.q = &q;
	poll_lockstep_cancel.m = &m;
	pthread_cleanup_push(poll_lockstep_worker_thread_exit,
			&poll_lockstep_cancel);

	while(1) {
		DBG_INF(8,("--> Worker %d starts (nr: %d for line %d)\n",
					sig_sub->id,
					sig_sub->sub_id,
					sig_def->id));

		/*Tell master one more has reached check-point 1*/
		m.cp=1;
		m.name=sig_def->name;
		m.sub_id=sig_sub->sub_id;
		DBG_INF(4,("--> %d: (%d/%d) Signal [%s], worker [%d] notifies reaching check-point [%d]\n",
			1, 0, 0, m.name, m.sub_id, m.cp
		));
		assert_ext(mq_send(q, (char*)&m, MSGSIZE, 1) != (mqd_t)-1);

		DBG_INF(8,("--> Worker %d will wait for sync...\n",sig_sub->id));
		/* Main sync point. Wait here. */
		assert_ext(sem_wait(&sampler_data.workers_barrier_1) == 0);
		DBG_INF(8,("--> Worker %d in sync. Continues...\n",sig_sub->id));

		DUSLEEP(MEDIUM);

		/* Prepare the run:
		1) Copy to last-value
		2) Clear current-value-to-be
		3) Clear is_updated marker
		*/
		memcpy(lastval,sig_sub->val,VAL_STR_MAX);
		memset(sig_sub->val,0,VAL_STR_MAX);
		sig_sub->is_updated=0;

		/* Run task accordingly to specific vartiant of task.*/
		assert_ext(sig_sub->sub_function);
		rc=sig_sub->sub_function(sig_sub, cnt);

		/* Can fail due to failed file-operations OR if data expected to be
		 * present is missing (most probably regular expression failure.
		 * Please check *.ini file. Simplified error-handling*/
		if (sig_def->sops.bits.always && rc!=0) {
			fprintf(stderr, "Data-source problem detected for signal with "
				"ALWAYS-bit set: [#%d:%s;%s:%d:%d] "
				"([line-nr:symbolic-name:data-file name:sub_sig:return-code])\n",
				sig_def->id, sig_def->name, sig_def->fdata, sig_sub->sub_id, rc);
			fprintf(stderr, "(Hint: To see more details about fatal errors, pass -d0 option\n");
			assert_ext("Aborting. Either data-file or regexp problem."
				"Please check your *.ini file" == NULL);
		}

		if (!rc) {
			cnt++;

			sig_sub->is_updated=1;
			/*Tell master one more is finished*/
			DBG_INF(8,("--> Worker %d finished work. Now waiting for buddies...\n",
				sig_sub->id));
		} else {
			memcpy(sig_sub->val,lastval,VAL_STR_MAX);
		}

		/*Tell master one more has reached check-point 2*/
		m.cp=2;
		m.name=sig_def->name;
		m.sub_id=sig_sub->sub_id;
		mq_send(q, (char*)&m, MSGSIZE, 1);

		/*Catch-up sync point here.*/
		assert_ext(sem_wait(&sampler_data.workers_barrier_2) == 0);

		DBG_INF(8,("--> Worker %d all done!\n",sig_sub->id));
	}
	pthread_cleanup_pop(1);
}

/* Default poll_lockstep_cancellation handler for the thread with the same name */
static void poll_lockstep_worker_thread_exit(void* inarg) {
	/* Thread has self-destructed (or been destroyed), Make sure balance is
	 * restored to the threads that remain */
	struct poll_lockstep_cancel *poll_lockstep_cancel = inarg;
	struct sig_sub *sig_sub = poll_lockstep_cancel->inarg;
	struct sig_data *sig_data = sig_sub->ownr;
	struct sig_def *sig_def = &(sig_sub->ownr->ownr->sig_def);

	DBG_INF(3,("   Worker %d id dieing!\n", sig_sub->id));
	sampler_data.ndied++;
	poll_lockstep_cancel->m->cp=3;
	poll_lockstep_cancel->m->name=sig_def->name;
	poll_lockstep_cancel->m->sub_id=sig_sub->sub_id;
	mq_send(poll_lockstep_cancel->q, (char*)poll_lockstep_cancel->m, MSGSIZE, 1);
	assert_ext(sem_wait(&sampler_data.workers_barrier_2) == 0);

	DBG_INF(3,("   Worker %d has died\n", sig_sub->id));
}

/* Worker for asynchronous read */
/* Note: output is still synchronized */

void *blocked_read_thread(void* inarg) {
	struct sig_sub *sig_sub = inarg;
	struct sig_data *sig_data = sig_sub->ownr;
	struct sig_def *sig_def = &(sig_sub->ownr->ownr->sig_def);
	int rc,cnt = 0;
	
//	if (sig_def->sops.bits.trigger) {
		sig_sub->sub_function = blocked_reader;
//	} else {
//		sig_sub->sub_function = blocked_reader_wtrigger;
//	}
	assert_ext(sig_sub->sub_function);
	rc=sig_sub->sub_function(sig_sub, cnt);
}
