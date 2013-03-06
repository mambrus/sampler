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

/* Include module common stuff */
#include "sampler.h"
#include "sigstruct.h"

/* Counting semaphore, main synchronizer. Lock is taken once for each thread
 * and all are released at once by the master releasing it n-times
 * simultaneously */
static sem_t start_barrier;
static sem_t end_barrier;

/* Used non-counting, as a simple synchronizer letting master know at least
 * one thread has started and counting end-barrier is taken.*/
static pthread_mutex_t workers = PTHREAD_MUTEX_INITIALIZER;

/* Simple worker thread. As in-data it takes it's own list-slot (i.e. no
 * need to search and no concurrency aspects. It knows nothing about when to
 * run, the poll_master dictates each run. */
void *poll_worker_thread(void* inarg) {
	struct sig_sub *signal = inarg;
	struct sig_data *sigadmin = signal->belong;

	while(1) {
		sem_wait(&start_barrier);
		sem_wait(&end_barrier);    /* Will not block, just takes a token */
		pthread_mutex_unlock(&workers);    /* Tell master OK to continue */
		printf("Running %d\n",signal->nr_sig);

		/*Tell master one more is finished*/
		sem_post(&end_barrier);
	}
}

/* Simple poll master. It tells when it's group of workers should run my
 * releasing a semaphore periodically according to period time it
 * administers. It's responsible for keeping time of each sample, and to
 * jitter compensate if needed. */
void *poll_master_thread(void* inarg) {
	struct sig_data *sigadmin = inarg;
	int i;

	while(1) {
		/* Tell all workers go!*/
		for(i=0; i<sigadmin->nsigs; i++) sem_post(&start_barrier);
		pthread_mutex_lock(&workers); /* Wait for one worker to start */
		printf("Master has released %d workers\n",sigadmin->nsigs);

		/* Wait for all to finish */
		sem_wait(&end_barrier);

		/* Record time, output sample, calculate jitter-factor*/
		/* TBD */
	}
}

int create_executor(handle_t list) {
	int rc,i,j;
	volatile int k=0;
	struct node *n;
	struct sig_sub *signal;
	struct sig_data *sigadmin;
	struct smpl_signal *sample;

	/* stub this for now. Just glue together to get compiler to check for
	 * missing dependencies. What's missing is a implemented list. Remove
	 * ASAP or when better stub is available */
	//return(0);

	rc=sem_init(&start_barrier, 0, 0); /*Start with no tokens, all workers
										 will block waiting for the master*/
	assert(rc==0);
	rc=sem_init(&end_barrier, 0, 0);   /*Start with no tokens. Worker must
										 take tokens */
	assert(rc==0);
	rc=pthread_mutex_lock(&workers);    /*Make sure Master blocks */
	assert(rc==0);

	/* Create a worker-thread for each sub-signal */
	for(n=mlist_head(list); n; n=mlist_next(list)){
		assert(n->pl);

		sample=(struct smpl_signal *)(n->pl);
		sigadmin=&((*sample).sig_data);
		for(j=0; j<sigadmin->nsigs; j++){
			signal=&((sigadmin->sigs)[j]);
			rc=pthread_create(
				&signal->worker,  
				NULL, 
				poll_worker_thread, 
				signal
			);
			assert(rc==0);
			assert(k++<100); //Sanity-check,
		}
	}

	rc=pthread_create(
		&sigadmin->master,  
		NULL, 
		poll_master_thread, 
		sigadmin
	);
	assert(rc==0);
	while(1){ 
	/* Really should joint the threads. But good for now. Root thread will
	handle stdin, whilst poll_master_thread  handles output.*/
		usleep(1000000);
	}
}
