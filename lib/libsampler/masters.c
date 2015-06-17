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

/* This file contains master thread(s). Threads and tasks are separated from
 * each other. This is a design goal which makes framework more maintainable
 * and tasks able to run under other forms. Alternatives could be (depending
 * on the purpose of the measurement and disturbance thereof, different
 * strategies could be deployed):
 *
 * - Non host-OS threading
 * - As threads but grouped together by super-threads. Possibly evenly
 *   grouped in as many groups as there are CPU:s
 * - Non-threaded. Function table with tasks are iterated through in one
 *   single thread (main thread of process).
 */

#include <pthread.h>
#include <semaphore.h>
#include <mqueue.h>
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
#include <sampler.h>
#include <syncvar.h>
#include "sigstruct.h"

#ifndef TESTSPEED
#define TESTSPEED 5
#endif

#include "local.h"
#define MSGSIZE sizeof(struct lockstep_msg)

#define _RELEASE_WORKER( AT ) \
	assert_ext(sem_post( \
		&sampler_data.workers_barrier_##AT) == 0 )

#define RELEASE_WORKER( AT ) \
	(AT==1)?_RELEASE_WORKER(1):_RELEASE_WORKER(2)

static void master_lockstep(int step, mqd_t *q) {
	int i,nc,n,rc;
	struct lockstep_msg m;

	DBG_INF(4,("<-- %d: Waiting for %d workers to reach sync-point\n",
		step,
		GET_NWORKERS()
	));

	for (nc=0, n=GET_NWORKERS(); nc < n; nc++,n=GET_NWORKERS()) {
		rc = mq_receive(*q, (char*)&m, MSGSIZE, NULL);
		DBG_INF(4,("<-- %d: (%d/%d) Signal [%s], worker [%d] has reached check-point [%d]\n",
			step, nc+1, n, m.name, m.sub_id, m.cp
		));
	};

	DBG_INF(8,("<-- %d: Workers armed! Will release %d worker now\n",
		step, GET_NWORKERS()));

	for(i=0; i<GET_NWORKERS(); i++)
		RELEASE_WORKER(step);

	DBG_INF(8,("<-- %d: All workers released! Master continues...\n", step));
}

/* Simple poll master thread. It tells when it's group of workers should run
 * by releasing either one of two semaphores periodically according to
 * either period time or event telling it to run, thereby starting a
 * controlled lock-step chain reaction. It's responsible for keep track of
 * time of each sample, either execution time or period time depending of
 * setting (default is period time). It does not need to jitter compensate
 * as the metronome (time event thread) does that but it will gather
 * execution time for either statistics or validity checking.
 */
void *poll_master_thread(void* inarg) {
	handle_t list = sampler_data.list, tlist;
	mqd_t q;
	struct mq_attr qattr;
	int i;
	struct node *np;


	qattr.mq_maxmsg = 100;
	qattr.mq_msgsize = sizeof (struct val_msg);
	assert_np(mlist_dup(&tlist, list) == 0);
	for(np=mlist_head(tlist); np; np=mlist_next(tlist)){
		struct smpl_signal *smpl_signal;
		struct sig_data *sig_data;
		struct sig_sub *sig_sub;
		int j;

		assert_ext(np->pl);

		smpl_signal=(struct smpl_signal *)(np->pl);
		sig_data=&((*smpl_signal).sig_data);
		for(j=0; j<sig_data->nsigs; j++){
			sig_sub=&((sig_data->sig_sub)[j]);
			if (sig_sub->val_pipe) {
				INFO(("%s: Initializing receiving val-pipe\n",
					sig_sub->name));
				assert_ext((sig_sub->val_pipe->read = mq_open(
					sig_sub->name,
					O_CREAT|O_RDONLY|O_NONBLOCK,
					OPEN_MODE_REGULAR_FILE, &qattr)) != (mqd_t)-1);
			}
		}
	}
	assert_np(mlist_close(tlist) == 0);


	if ((sampler_setting.prio_master != INT_MAX) ||
	    (sampler_setting.policy_master != INT_MAX))
	{
		assert_ext(!set_thread_prio(sampler_setting.policy_master,
			sampler_setting.prio_master));
	}

	/* Create lock-step queue with at least the same number of elements as
	 * there are sub-signals writing to it (we never want block-on-write
	 * here)
	 */
	qattr.mq_maxmsg = sampler_data.nworkers+1;
	qattr.mq_msgsize = MSGSIZE;
	assert_ext((q = mq_open(
		LOCKSTEP_Q,
		O_CREAT|O_RDONLY,
		OPEN_MODE_REGULAR_FILE, &qattr)) != (mqd_t)-1);

	for(i=0; i<GET_NWORKERS(); i++) {
		DBG_INF(4,("<--     Worker release\n"));
		RELEASE_WORKER(2);
	}

	/* Master is primed and ready for akchion */
	assert_ext(pthread_mutex_unlock(&sampler_data.mx_master_up) == 0);
	assert_ext(sem_wait(&sampler_data.pipes_complete)== 0);

	while(1) {
		assert_ext(sem_wait(&sampler_data.master_event) == 0);
		/* Run event received. Time-stamp as early as possible. This stamp
		   represents the complete group of data as a "tag" */
		BEGIN_WR(&sampler_data.sampler_data_lock)
			assert_np(time_now(&sampler_data.tstarted) == 0);
		END_WR(&sampler_data.sampler_data_lock)
		master_lockstep(1,&q);
		master_lockstep(2,&q);

		/* Can this be bundled into master_lockstep to cover two cases? */
		if (sampler_data.ndied) {
			DBG_INF(8,("<-- X: Worker death detected. Reseting logic for %d deaths\n",
				sampler_data.ndied));
			ADD_NWORKERS(0-sampler_data.ndied);
			sampler_data.ndied=0;
		}

		/* Continue to harvest workers work, record time, output sample,
		 * calculate run-time e.t.a.*/
		harvest_sample(list);
		DBG_INF(8,("<-- Master harvests sample nr#: %u\n",
			sync_get(&sampler_data.diag_lock, &sampler_data.diag.smplID)));
	}
}

