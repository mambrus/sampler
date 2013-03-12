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

/* This file contains stuff needed for running the sampler. I.e. the threads
 * and logic synchronizing them */

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

/* Owner of some needed globals...*/
sem_t workers_start_barrier;
sem_t workers_end_barrier;
int waiting1 = 0;
pthread_rwlock_t rw_lock1 = PTHREAD_RWLOCK_INITIALIZER;
int waiting2 = 0;
pthread_rwlock_t rw_lock2 = PTHREAD_RWLOCK_INITIALIZER;

inline int get_waiting1(){
	int tmp;

	pthread_rwlock_rdlock(&rw_lock1);
	tmp = waiting1;
	pthread_rwlock_unlock(&rw_lock1);
	return tmp;
}

inline void inc_waiting1( int inc ){
	pthread_rwlock_wrlock(&rw_lock1);
	waiting1 += inc;
	pthread_rwlock_unlock(&rw_lock1);
}

inline int get_waiting2(){
	int tmp;

	pthread_rwlock_rdlock(&rw_lock2);
	tmp = waiting2;
	pthread_rwlock_unlock(&rw_lock2);
	return tmp;
}

inline void inc_waiting2( int inc ){
	pthread_rwlock_wrlock(&rw_lock2);
	waiting2 += inc;
	pthread_rwlock_unlock(&rw_lock2);
}

/* Simple worker thread. As in-data it takes it's own list-slot (i.e. no
 * need to search and no concurrency aspects. It knows nothing about when to
 * run, the poll_master dictates each run. */
void *poll_worker_thread(void* inarg) {
	struct sig_sub *sig_sub = inarg;
	struct sig_data *sig_data = sig_sub->ownr;
	struct sig_def *sig_def = &(sig_sub->ownr->ownr->sig_def);
	float fakt = 10.0*1000000.0/(float)samplermod_data.ptime;
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

		/*Will be a switch-case here (TBD)*/
		rc=produce_sinus_data(sig_sub, cnt);

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

/* Simple poll master. It tells when it's group of workers should run by
 * releasing a semaphore periodically according to period time it
 * administers. It's responsible for keeping time of each sample, and to
 * jitter compensate if needed. */
void *poll_master_thread(void* inarg) {
	struct sig_data *sig_data = inarg;
	int i,nw;
	handle_t list = samplermod_data.list;

	while(1) {
		/*Simulate sync*/
		//DUSLEEP(MEDIUM);

		nw=get_waiting1();
		INFO(("<-- %d of %d workers have blocked (i.e. started)\n",nw,nworkers));
		for ( ; abs(nw)<samplermod_data.nworkers; nw=get_waiting1() )
		{
				INFO(("<-- Waiting for %d of %d workers to block\n",nw,nworkers));
				/* Some workers are late, wait a little for them */
				usleep(100);
		}

		/* Tell all workers go!*/
		INFO(("<-- Workers armed! Will release %d worker now\n",nworkers));
		for(i=0; i<samplermod_data.nworkers; i++) assert_ext(sem_post(&workers_start_barrier) == 0);
		INFO(("<-- Master continues...\n",nworkers));
		//DUSLEEP(LONG);

		nw=get_waiting2();
		INFO(("<-- %d of %d workers have blocked (i.e. started)\n",nw,nworkers));
		for ( ; abs(nw)<samplermod_data.nworkers; nw=get_waiting2() )
		{
				INFO(("<-- Waiting for %d of %d workers to block\n",nw,nworkers));
				/* Some workers are late, wait a little for them */
				usleep(100);
		}

		/* Tell all workers go!*/
		INFO(("<-- Workers gathered! Will release %d worker now\n",nworkers));
		for(i=0; i<samplermod_data.nworkers; i++) assert_ext(sem_post(&workers_end_barrier) == 0);
		INFO(("<-- Master continues (again)...\n",nworkers));

		/* Wait for all to finish (Should always block. Workers have more work to
		 * do than master.*/

		/* Harvest, record time, output sample, calculate jitter-factor*/
		harvest_sample(list);
		INFO(("<-- Master harvest sample nr#: %llu\n",samplermod_data.smplcntr));
		/* TBD */
	}
}

int create_executor(handle_t list) {
	int rc,i,j;
	struct node *n;
	struct sig_sub *sig_sub;
	struct sig_data *sig_data;
	struct smpl_signal *smpl_signal;

	/* Store list for threads to pick-up */
	samplermod_data.list = list;

	/* Main sync-barrier. Start with no tokens, all workers will block
	 * waiting for the master */
	assert_ext(sem_init(&workers_start_barrier, 0, 0) == 0);

	/* Create a worker-threads, one for each sub-signal */
	for(n=mlist_head(list); n; n=mlist_next(list)){
		assert(n->pl);

		smpl_signal=(struct smpl_signal *)(n->pl);
		sig_data=&((*smpl_signal).sig_data);
		for(j=0; j<sig_data->nsigs; j++){
			sig_sub=&((sig_data->sigs)[j]);
			INFO(("*** Starting worker %d (nr: %d for line %d)\n",
					sig_sub->id,
					sig_sub->sub_id,
					sig_sub->ownr->ownr->sig_def.id));

			rc=pthread_create(
				&sig_sub->worker,
				NULL,
				poll_worker_thread,
				sig_sub
			);
			assert(rc==0);
			assert(samplermod_data.nworkers++<100); //Sanity-check,
		}
	}

	rc=pthread_create(
		&sig_data->master,
		NULL,
		poll_master_thread,
		sig_data
	);
	assert(rc==0);
	while(1){
		/* Really should joint the threads. But good for now. Root thread
		 * will handle stdin, whilst poll_master_thread  handles output.*/
		DUSLEEP(MEDIUM);
		sleep(1);
	}
}
