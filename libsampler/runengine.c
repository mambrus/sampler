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
#include <limits.h>
#include <errno.h>
#include <sys/inotify.h>
#include <sys/prctl.h>
#include <sys/types.h>

/* Include module common stuff */
#include "sampler.h"
#include "sigstruct.h"
#include "local.h"

#ifndef MAX_WORKERS
#define MAX_WORKERS 250
#endif //MAX_WORKERS

static char *get_signame(const char *fname, char *buf, int *ret_len) {
	FILE *f;

	f = fopen(fname, "r");
	assert_ext(f);
	if (!f){
		if (ret_len)
			*ret_len = 0;
		return NULL;
	}
	memset(buf, 0, NAME_STR_MAX);
	fgets(buf, NAME_STR_MAX, f);
	fclose(f);

	/* fgets should have done this already, but just to be
	 * safe: */
	buf[NAME_STR_MAX-1]='\0';

	/* Get rid of last LF is such exists */
	if (buf[strlen(buf) -1] == '\n')
		buf[strlen(buf) -1] = '\0';

	if (ret_len)
		*ret_len = strlen(buf);

	if (strlen(buf)) {
		return buf;
	}
	return NULL;
}

/* Output first line describing the contents of each columns. These strings
 * will be be used as legends in plotting software */
void outputlegends( handle_t list ) {
	int j,from_file=0;
	struct node *np;
	struct sig_sub *sig_sub;
	struct sig_data *sig_data;
	struct sig_def *sig_def;
	struct smpl_signal *smpl_signal;
	char *tname;
	char tname_buff[NAME_STR_MAX];

	if (sampler_setting.dolegend) {
		printf("Time-now%cTSince-last",
				sampler_setting.delimiter);

		for(np=mlist_head(list); np; np=mlist_next(list)){
			assert_ext(np->pl);

			smpl_signal=(struct smpl_signal *)(np->pl);
			sig_data=&((*smpl_signal).sig_data);
			sig_def=&((*smpl_signal).sig_def);

			if (sig_def->fname && strnlen(sig_def->fname, NAME_STR_MAX)) {
				from_file = 1;
				tname =sig_def->fname;
			} else {
				from_file = 0;
				tname =sig_def->name;
			}

			assert_ext(tname && strnlen(tname, NAME_STR_MAX) &&
				("Symbolic name and file where to get it from can't both be nil" !=
				 NULL)
			);

			if (sig_data->nsigs > 1) {
				for(j=0; j<sig_data->nsigs; j++){
					sig_sub=&((sig_data->sig_sub)[j]);
					if (from_file) {
						printf("%c%s_%d=%s_%d",
							sampler_setting.delimiter,
							sig_def->name, j,
							get_signame(tname,tname_buff, NULL), j);
					} else {
						printf("%c%s_%d", sampler_setting.delimiter,
							tname, j);
					}
				}
			} else {
				if (from_file) {
					printf("%c%s=%s",
						sampler_setting.delimiter,
						sig_def->name,
						get_signame(tname,tname_buff, NULL)
					);
				} else {
					printf("%c%s",
						sampler_setting.delimiter, tname );
				}
			}
		}

		printf("\n");
		fflush(stdout);
	}
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
	struct sig_def *sig_def;
	int pipe_complete_startval=1;

	/* Store list for threads to pick-up */
	sampler_data.list = list;

	/* Master unlocks this when started and initialized. This synchronizes
	 * queue creation for asynchronous threads */
	assert_ext(pthread_mutex_lock(&sampler_data.mx_master_up) == 0);

	/* Event-barrier. Start with no tokens, master will block until first
	 * event arrives.
	 */
	assert_ext(sem_init(&sampler_data.master_event, 0, 0) == 0);

	/* Main sync-barrier. Start with no tokens, all workers will block
	 * waiting for the master */
	assert_ext(sem_init(&sampler_data.workers_barrier_1, 0, 0) == 0);

	/* Create a worker-threads, one for each sub-signal */
	for(n=mlist_head(list); n; n=mlist_next(list)){
		assert_ext(n->pl);

		smpl_signal=(struct smpl_signal *)(n->pl);
		sig_data=&((*smpl_signal).sig_data);
		sig_def=&(sig_data->ownr->sig_def);
		for(j=0; j<sig_data->nsigs; j++){
			sig_sub=&((sig_data->sig_sub)[j]);

			switch (sig_sub->daq_type){
				case LOCKSTEP_POLLED:
					sig_sub->thread_sfun = poll_lockstep_worker_thread;
					sig_sub->sub_function = poll_fdata;
				break;
				case LOCKSTEP_NOTIFIED:
					sig_sub->thread_sfun = poll_lockstep_worker_thread;
					sig_sub->sub_function = poll_fdata;
					pipe_complete_startval--;
					if (j==0) {
						INFO(("Monitoring on updates requested for %s\n",sig_def->name));
						INFO(("File: %s\n",sig_def->fdata));
						inotify_add_watch(sampler_data.fd_notify, sig_def->fdata,
							IN_MODIFY);
					}
					break;
				case FREERUN_ASYNCHRONOUS:
					sig_sub->thread_sfun = blocked_read_thread;
					if (sig_def->sops.bits.dont_squash || sig_def->sops.bits.trigger) {
						/* use a buffer */
						sig_sub->val_pipe=malloc(sizeof(struct val_pipe));
					}
					/* Keep it simple, let thread init figure out any
					 * further variations */
					sig_sub->sub_function = NULL;
					assert_ext(!pthread_mutex_init(&sig_sub->lock, NULL));
				break;
				default:
					INFO(("Missing daq_type for %s\n", sig_def->name));
					assert_ext("Missing daq_typ (not set)" == NULL);
			}

			if ((sig_sub->sub_id == 0) ||
				(sig_def->sops.bits.tops != NEVER_THREAD_SUB_SIG)
			) {
				INFO(("*** Starting worker %d (nr: %d for line %d)\n",
						sig_sub->id,
						sig_sub->sub_id,
						sig_sub->ownr->ownr->sig_def.id));

				assert_np(rc=pthread_create(
					&sig_sub->worker,
					NULL,
					sig_sub->thread_sfun,
					sig_sub
				) == 0);
				if (!sig_def->sops.bits.canblock) {
					/* Only synchronous threads need to participate in
					 * lock-step */
					ADD_NWORKERS(1);
				}
				assert_ext(GET_NWORKERS()<MAX_WORKERS);
			}
		}
	}

	/* All is ready to start, output legends if needed */
	if (sampler_setting.dolegend)
		outputlegends(list);

	/* Make master wait for asynchronous to complete q-init.
	 * */
	assert_ext(sem_init(&sampler_data.pipes_complete, 0,
		pipe_complete_startval) == 0);

	assert_ign(!pthread_create(
		&sampler_data.master,
		NULL,
		poll_master_thread,
		NULL
	));

	/* If periodical, create thread to produce time events with that
	 * periodicity */
	if (sampler_setting.ptime > 0)
		assert_ign(!pthread_create(
			&sampler_data.timer_events,
			NULL,
			time_eventgen_thread,
			NULL
		));

	/* If at least one file id monitored, create a file-monitor thread
	 */
	if (sampler_data.files_monitored > 0) {
		/*Make sure to empty any pending buffers first ting*/
		assert_ext(sem_post(&sampler_data.master_event)== 0);
		assert_ign(!pthread_create(
			&sampler_data.filesmon_events,
			NULL,
			filemon_eventgen_thread,
			NULL
		));
	}

	while(1){
		/* Really should joint the threads. But good for now. Root thread
		 * will handle stdin, whilst poll_master_thread  handles output.*/
		DUSLEEP(MEDIUM);
		usleep(25000);
		fflush(stdout);
	}
}
