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
/* This file contains event generators to the master */

#include <pthread.h>
#include <semaphore.h>
#include <assert.h>
#include <assure.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <sys/inotify.h>


/* Include module common stuff */
#include "sampler.h"
#include "sigstruct.h"
#include "local.h"

#define EV_PEND_LIMIT1  100
#define EV_PEND_LIMIT2  1000

/*
 * Timer event generator. Principal idea is to keep an extremely tight loop
 * and to trigger the master periodically. This makes jitter compensation
 * both per sample and ever several samples when a time depth have occurred
 * much easier. This thread must have the process (systems) highest
 * priority.
 */
void *time_eventgen_thread(void* inarg) {
	int pend,wm=0;

	if ((sampler_setting.prio_events != INT_MAX) ||
	    (sampler_setting.policy_events != INT_MAX))
	{
		assert_ext(!set_thread_prio(sampler_setting.policy_events,
			sampler_setting.prio_events));
	}
	while (1) {
		DBG_INF(3,("Timer firing off event\n"));
		if (sampler_setting.debuglevel >= 2) {
			assert_ext(sem_getvalue(&sampler_data.master_event, &pend) == 0);
			if ( pend > EV_PEND_LIMIT1 ) {
					if ((sampler_setting.debuglevel >= 3) &&
						((pend-wm) > EV_PEND_LIMIT1))
					{
						DBG_INF(1,(
							"SAMPLER is not keeping up, "
							"metronome is now %d cycles ahead\n",
							pend));
						wm = pend;
					}
			} else
				wm = 0;
			assert_ext(pend < EV_PEND_LIMIT2);
		}
		assert_ext(sem_post(&sampler_data.master_event) == 0);
		usleep(sampler_setting.ptime/*-uComp*/);
	}
}

/*
 * Monitor file change
 * For linux this is done using inotify. Note that inotify works differently
 * on different types of file-systems (on sysfs, practically not at all
 */
void *filemon_eventgen_thread(void* inarg) {
	int rsz;
	struct inotify_event inotify_event;
	while (1) {
		rsz=read(sampler_data.fd_notify, &inotify_event,
			sizeof(inotify_event));

		if (sampler_setting.debuglevel >= 4) {
			INFO(("File monitor detects notification. Pre-process to find if "
				"is anyone is interested.\n"));
		}
		assert_ext(sem_post(&sampler_data.master_event) == 0);
	}
}

/*
 * Event generator template
 * Not to be used. Only a template to be derived from for other generators
 */
void *template_eventgen_thread(void* inarg) {
	while (1) {
		assert_ext(sem_post(&sampler_data.master_event) == 0);
	}
}
