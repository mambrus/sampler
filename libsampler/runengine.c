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

/* Counting semaphore, main synchronizer. Lock is taken once for each thread
 * and all are released at once by the master releasing it n-times
 * simultaneously */
static sem_t start_barrier;
static sem_t end_barrier;

/* Used non-counting, as a simple synchronizer letting master know at least
 * one thread has started and counting end-barrier is taken.*/
static pthread_mutex_t workers = PTHREAD_MUTEX_INITIALIZER;

/* Number of workers (we don't have a sample struct yet, therefore global)*/
static int nworkers;

/* Simple worker thread. As in-data it takes it's own list-slot (i.e. no
 * need to search and no concurrency aspects. It knows nothing about when to
 * run, the poll_master dictates each run. */
void *poll_worker_thread(void* inarg) {
	struct sig_sub *sig_sub = inarg;
	struct sig_data *sig_data = sig_sub->ownr;
	struct sig_def *sig_def = &(sig_sub->ownr->ownr->sig_def);

	while(1) {
		INFO(("--> Worker %d starts (nr: %d for line %d)\n",
					sig_sub->id,
					sig_sub->sub_id,
					sig_def->id));

		sem_post(&end_barrier);
		sem_wait(&start_barrier); //Main sync point. Wait here.
		INFO(("--> Worker %d in sync. Continues...\n",sig_sub->id));
		sem_wait(&end_barrier);    /* Will not block, just takes a token */
		INFO(("--> Worker %d notified master!\n",sig_sub->id));
		pthread_mutex_unlock(&workers);    /* Tell master OK to continue */
		INFO(("--> Running... %d\n",sig_sub->id));
		DUSLEEP(MEDIUM);
		/* Clear last value */
		memset(sig_sub->val,0,VAL_STR_MAX);
		{
			/* Work goes here */
			float x;

			x=(samplermod_data.smplcntr/100.0)*2.0*3.1415;
			if (sig_sub->id == 2 && samplermod_data.smplcntr==30) {
				//sleep(200);
			}
			sig_sub->rtime.tv_sec=sig_sub->id;
			snprintf(sig_sub->val,VAL_STR_MAX,"%f",sin(x));
		}
		sig_sub->is_updated=1;
		/*Tell master one more is finished*/
		sem_post(&end_barrier);
	}
}

/* Simple poll master. It tells when it's group of workers should run by
 * releasing a semaphore periodically according to period time it
 * administers. It's responsible for keeping time of each sample, and to
 * jitter compensate if needed. */
void *poll_master_thread(void* inarg) {
	struct sig_data *sig_data = inarg;
	int i;
	handle_t list = samplermod_data.list;

	while(1) {
		/*Simulate sync*/
		DUSLEEP(MEDIUM);
		/* Tell all workers go!*/
		INFO(("<-- Master sync. Will release %d workers\n",nworkers));
		for(i=0; i<nworkers; i++) sem_post(&start_barrier);
		INFO(("<-- Master has released %d workers\n",nworkers));
		pthread_mutex_lock(&workers); /* Wait for at least one worker to
										 start. That way we know "GO!" has
										 been heard and acted upon*/
		INFO(("<-- Master continues...\n",nworkers));
		DUSLEEP(LONG);

		/* Wait for all to finish */
		sem_wait(&end_barrier);

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
	rc=sem_init(&start_barrier, 0, 0);
	assert(rc==0);

	/* Init with 0. Workers posts +1 on each runs start. Worker must take
	 * token and will block until all threads have reached end-barrier. This
	 * is what makes the lock-step work.*/
	rc=sem_init(&end_barrier, 0, 0);
	assert(rc==0);

	/* Make sure Master blocks. This is to avoid master having a chance to
	 * race against the end-border if it for some reason would get to start
	 * first time before any worker. It's very unlikely to happen and the
	 * extra sync point costs time. Thinking about to remove it, worst that
	 * can happen is that lock-step gets disfunct the first 1-2 samples. */
	rc=pthread_mutex_lock(&workers);
	assert(rc==0);

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
			assert(nworkers++<100); //Sanity-check,
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
	}
}
