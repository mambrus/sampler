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

/* This file contains stuff needed for creation of the sampler. I.e. the
 * threads and logic synchronizing them */

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
#include "local.h"

/* This compilation unit is the owner of some needed globals to coordinate
 * synchronization. Putting these in mod-globas play no purpose as the types
 * aren't human readable */

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

/* Creator of all living beings in this project (threads i.e.). Handles
 * creating both master and worker threads, different types if needed
 * according to their modalities. */
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
