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

/* This file contains master threads (there might be several different
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


/* Simple poll master thread. It tells when it's group of workers should run
 * by releasing a semaphore periodically according to period time it
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
