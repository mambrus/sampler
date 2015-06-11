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

/* Public interface of sampler-module  */

#ifndef sampler_h
#define sampler_h
#include <stdio.h>
#include <stdint.h>
#include <pthread.h>

#define VAL_STR_MAX 25
#ifndef SAMPLER_TMPDIR
#  define SAMPLER_TMPDIR /tmp
#endif

#include <stdlib.h>
#define SETFROMENV( envvar, locvar, buf_max)				\
{															\
	char *ts;												\
	if ((ts=getenv(#envvar)) != NULL ) {					\
		int l;												\
		memset(locvar,0,buf_max);							\
		l=strnlen(ts,buf_max);								\
		memcpy(locvar,ts,l<buf_max?l:buf_max);				\
	}														\
}

/* Module overall setting/start/stop/behaviour */
int sampler_init(const char *filename, int period);
int sampler_fini();

enum clock_type {
	AUTODETECT = 0,     /* Probes during startup and selects best type */
	KERNEL_CLOCK,       /* As close to kernel time as possible */
	CALENDER_CLOCK      /* Best form of calender clock is used */
};

enum plotmode {
	feedgnuplot = 0,    /* Standard one-line output */
	driveGnuPlot = 1    /* i.e. driveGnuPlotStreams */
};

/* Sub-process i.e. process-substitution handling */
struct procsubst {
	int inherit_env;    /* Inherit parent process environment */
	char **path;        /* List of paths to be appended to PATH
						   environment 	variable. Number of elements is
						   determined by the last element in table which
						   hence needs to be NULL.	*/
	char **env;         /* Environment, either to be used as is or appended
						   if inherit_env is also true*. Number of elements
						   is determined by the last element in table which
						   hence needs to be NULL. */
};

/* Settings */
struct module_sampler_setting {
	int isinit;
	int ptime; /* Period time in us (Max: 4294/2=2147s = 35min) */
	enum clock_type clock_type;
	enum plotmode plotmode;
	char delimiter;
	int dolegend;
	int legendonly;
	int debuglevel;		  /* Affects how much extra information is printed
							 for debugging */
	int verbose;		  /* Additional verbosity */
	char presetval[VAL_STR_MAX]; /* What to print in case is needed */
	int realtime;
	int policy_master;
	int policy_workers;
	int policy_events;
	int prio_master;
	int prio_workers;
	int prio_events;
	char *tmpdir;

	struct procsubst procsubst;
	pthread_rwlock_t sampler_setting_lock;
};

#endif /* sampler_h */

