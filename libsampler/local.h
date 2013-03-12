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

/* Local interface & stuff for the sampler library/module  */

#ifndef local_h
#define local_h
#include <mlist.h>
#include <stdio.h>
#include <semaphore.h>

#include "sigstruct.h"

#define __init __attribute__((constructor))
#define __fini __attribute__((destructor))

int parse_initfile(const char *fn, handle_t *list);
int create_executor(handle_t list);
int self_destruct(handle_t list);
void harvest_sample(const handle_t list);
int produce_sinus_data(struct sig_sub* sig_sub, int cnt);
struct timeval tv_diff( struct timeval t0, struct timeval t1 );

#ifndef DBGLVL
#define DBGLVL 3
#endif

#ifndef TESTSPEED
#define TESTSPEED 3
#endif

#if ( DBGLVL == 0 )
#  define INFO( S ) ((void)0)
#  define DUSLEEP( T ) ((void)0)
#elif ( DBGLVL == 1 )
#  define INFO( S )  printf S
#  define DUSLEEP( U ) usleep( U )
#elif ( DBGLVL == 2 )
#  define INFO( S ) { printf S; fflush(stdout); }
#  define DUSLEEP( U ) usleep( U )
#elif ( DBGLVL == 3 )
#  define eprintf(...) fprintf (stderr, __VA_ARGS__)
#  define INFO( S ) eprintf S
#  define DUSLEEP( U ) usleep( U )
#else
#error bad value of DBGLVL
#endif

#if ( TESTSPEED == 5 )
#  define SMALL      0
#  define MEDIUM     0
#  define LONG       0
#  undef  DUSLEEP
#  define DUSLEEP( T ) ((void)0)
#elif ( TESTSPEED == 4 )
#  define SMALL      1
#  define MEDIUM     10
#  define LONG       50
#elif ( TESTSPEED == 2 )
#  define SMALL      10
#  define MEDIUM     100
#  define LONG       500
#elif ( TESTSPEED == 3 )
#  define SMALL      100
#  define MEDIUM     1000
#  define LONG       5000
#elif ( TESTSPEED == 2 )
#  define SMALL      1000
#  define MEDIUM     10000
#  define LONG       50000
#elif ( TESTSPEED == 1 )
#  define SMALL      10000
#  define MEDIUM     100000
#  define LONG       500000
#elif ( TESTSPEED == 0 )
#  define SMALL      100000
#  define MEDIUM     1000000
#  define LONG       5000000
#else
#error bad value of TESTSPEED
#endif

enum plotmode {
	feedgnuplot = 0,		/* Standard one-line output */
	driveGnuPlot = 1        /* i.e. driveGnuPlotStreams */
};

/* What to do if worker hasn't updated data due to that it's blocked (if
 * that is allowed for the thread i.e.) or it has detected the data is
 * unchanged (polling with detectable timestamps)*/
enum whatTodo {
	/*Output...: */
	Nothing = 0,
	Lastval,
	PresetSigStr,
	PresetSmplStr
};

enum clock_type {
	AUTODETECT = 0,     /* Probes during startup and selects best type */
	KERNEL_CLOCK,		/* As close to kernel time as possible */
	CALENDER_CLOCK		/* Best form of calender clock is used */
};

struct samplermod_struct {
	int isinit;

/* Settings */
	handle_t list;
	int ptime; /* Period time in us (Max: 4294/2=2147s = 35min) */
	enum clock_type clock_type;
	uint64_t smplcntr;
	enum plotmode plotmode;
	char delimiter;
	enum whatTodo whatTodo;
	int cid_offs;
	char presetval[VAL_STR_MAX]; /* What to print in case is needed */

/* Helper variables*/
	int nworkers;
};

/* Counting semaphore, main synchronizer. Lock is taken once for each thread
 * and all are released at once by the master releasing it n-times
 * simultaneously */
extern sem_t workers_start_barrier;  /* Main sync point */
extern sem_t workers_end_barrier;    /* Second sync point */

extern struct samplermod_struct samplermod_data;
extern sem_t workers_start_barrier;  /* Main sync point */
extern sem_t workers_end_barrier;    /* Second sync point */

/* Non-counting, as a simple synchronizer letting master know at least
 * one thread has started and counting end-barrier is taken.*/
//static pthread_mutex_t workers = PTHREAD_MUTEX_INITIALIZER;

/* Sync count handling - of no value having in modglobas as they are
 * meaningless for humans */
extern int waiting1;
extern pthread_rwlock_t rw_lock1;
extern int waiting2;
extern pthread_rwlock_t rw_lock2;

inline int get_waiting1( void );
inline void inc_waiting1( int inc );
inline int get_waiting2( void );
inline void inc_waiting2( int inc );

/* Worker threads */
void *poll_worker_thread(void* inarg);

/* Master threads*/
void *poll_master_thread(void* inarg);

/* Various other module-global functions */
int parse_initfile(const char *fn, handle_t *list);
int create_executor(handle_t list);
int self_destruct(handle_t list);
void harvest_sample(const handle_t list);
int produce_sinus_data(struct sig_sub* sig_sub, int cnt);
#define SEC( TV ) ((int)TV.tv_sec)
#define USEC( TV ) ((int)TV.tv_usec)
struct timeval tv_diff(struct timeval t0, struct timeval t1);
struct timeval tv_add(struct timeval t0, struct timeval t1);

#endif /* local_h */

