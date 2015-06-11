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

#ifndef libsampler_local_h
#define libsampler_local_h
#include <mlist.h>
#include <stdio.h>
#include <semaphore.h>
#include <mqueue.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#if defined(__GLIBC__)
#include <sys/syscall.h>
pid_t gettid(void);
#endif

#include "sigstruct.h"
#include "include/sampler.h"
#include "include/syncvar.h"

#define __init __attribute__((constructor))
#define __fini __attribute__((destructor))

#define xstr(s) str(s)
#define str(s) #s

/* OPEN_MODE_REGULAR_FILE is 0666, open() will apply umask on top of it */
#define OPEN_MODE_REGULAR_FILE \
    (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)

#define LOCKSTEP_Q "/Q_sampler_lockstep"
struct lockstep_msg {
	int cp;     /* Which step of the leg this event was for */
	char *name; /* Signal name, might not be unique if threaded sub_sig s*/
	int sub_id;	/* Which sub-signal this this */
};

int parse_initfile(const char *fn, handle_t *list);
int create_executor(handle_t list);
int self_destruct(handle_t list);
void harvest_sample(const handle_t list);
int time_now(struct timeval *tv);
struct timeval tv_diff( struct timeval t0, struct timeval t1 );
FILE *proc_subst_in(char *cmds);

#ifndef VERBOSE_TYPE
#define VERBOSE_TYPE 3
#endif

#ifndef TESTSPEED
#define TESTSPEED 3
#endif

/* Formatter for buffer content to be used with DBG_INF.
 * Makes it easier to see if EOL or any other white-space is at the end or
 * not, at the same time not garbling output if both stderr and stdout is
 * the same console. Makes no sense for Android as the logging mechanism
 * works differently there.
 * */
#if !defined(HAVE_ANDROID_OS)
# define BUF_FRMTR "\n{%s}\n"
#else
# define BUF_FRMTR "%s"
#endif

#define VERBOSE_TO( F, FNKN, S ) \
	{ \
		if (sampler_setting.verbose) { \
			FNKN S; \
			fflush( F ); \
		} \
	}

#if !defined(HAVE_ANDROID_OS) && !defined(NO_TIME_PID_LOG)
# define USE_TIME_AND_PID_IN_LOG
#endif

#if ! defined(USE_TIME_AND_PID_IN_LOG)
/* No need on Android as logcat already supports this */
# define DBG_TO( F, FNKN, S, L ) \
	{ \
		if (L <= sampler_setting.debuglevel) { \
			pthread_mutex_lock(&sampler_data.mx_stderr); \
			FNKN S; \
			pthread_mutex_unlock(&sampler_data.mx_stderr); \
			fflush( F ); \
		} \
	}

# else
/* Add time and thread-ID to log output */
/* Default case when debugging on host */
#   define DBG_TO( F, FNKN, S, L ) \
	{ \
		if (L <= sampler_setting.debuglevel) { \
			struct timeval tnow; \
			pthread_mutex_lock(&sampler_data.mx_stderr); \
			time_now(&tnow);\
			FNKN("[%d.%d:%d]: ", (int)tnow.tv_sec, \
				(int)(tnow.tv_usec), gettid() ); \
			FNKN S; \
			pthread_mutex_unlock(&sampler_data.mx_stderr); \
			fflush( F ); \
		} \
	}
#endif

#define RETURN( RC ) \
	DBG_INF(7,("Function %s returns from line %d with value %d\n", \
		__FUNCTION__,__LINE__,RC)); \
	return(RC)

#if ( VERBOSE_TYPE == 0 )
#  define INFO( S ) ((void)0)
#  define DBG_INF( L, S ) DBG_TO( stdout, printf, S, L )
#  define DUSLEEP( T ) ((void)0)
#elif ( VERBOSE_TYPE == 1 )
#  define INFO( S ) VERBOSE_TO( stdout, printf, S )
#  define DBG_INF( L, S ) DBG_TO( stdout, printf, S, L )
#  define DUSLEEP( U ) usleep( U )
#elif ( VERBOSE_TYPE == 2 )
#  define INFO( S ) VERBOSE_TO( stdout, printf, S )
#  define DBG_INF( L, S ) DBG_TO( stdout, printf, S , L )
#  define DUSLEEP( U ) usleep( U )
#elif ( VERBOSE_TYPE == 3 )
#  define eprintf(...) fprintf (stderr, __VA_ARGS__)
#  define INFO( S ) VERBOSE_TO( stderr, eprintf, S )
#  define DBG_INF( L, S ) DBG_TO( stderr, eprintf, S , L )
#  define DUSLEEP( U ) usleep( U )
#else
#  error bad value of VERBOSE_TYPE
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

#define DIAG_MAX_COLS 10
#define DIAG_MAX_SLEN 512

enum diag_out {
	DIAG_NONE=0,      /* This need to be first */
	EXEC_TIME,
	PERIOD_TIME_BEGIN,
	PERIOD_TIME_HARVESTED,
	SMPL_ID,
	TRIG_BY_ID,
	DIAG_SENTINEL     /* This need to be last */
};

struct smpl_diag {
	int smplID;      /* Sample ID */
	int triggID;     /* Sample-ID Who triggered OBD run */
	int texec;       /* RT execution time */
	int tp1;         /* Period time incoming */
	int tp2;         /* Period time outgoing */
	enum diag_out format_ary[DIAG_MAX_COLS];
};

/*This structure is prepared to be able to be put in a mlist object of it's
 * own should we consider multiple samplers to run in the same process (TBD)
 * */
struct module_sampler_data {
	int isinit;

/* Helper variables*/
	handle_t list;
	struct timeval tstarted;
	int nworkers;
	int ndied;
	int cid_offs;
	int files_monitored;
	int fd_notify;
	pthread_mutex_t mx_stderr;
	pthread_mutex_t mx_master_up;

	pthread_t master;		     /* Master to the slaves  */
	pthread_t filesmon_events;   /* Periodical events to the master */
	pthread_t timer_events;		 /* Monitoring events to the master */
	sem_t master_event;          /* Event sync-point */
	sem_t pipes_complete;        /* Pipes initialized  */

	pthread_rwlock_t sampler_data_lock;

	struct smpl_diag diag;
	pthread_rwlock_t diag_lock;

	/* Master lock-step mechanism. Lock-step contains 2 check-points. For
	 * master, one and the same queue us used. Workers send to the queue
	 * whence reaching check-point identifying them-selves. Master
	 * loop is reading the queue for the same amount of times as there
	 * are expected messages (i.e. pending workers) */

	/* -- Variables intentionally left empty-- */
	/* Note: Queue handles are not global, only their names are. Handles reside
	 * on each threads stack (local variable) */

	/* Workers lock-step mechanism are counting semaphores. Lock is
	 * taken once for each worker's thread. When all workers have gathered
	 * at a sync point, master then releases them all at once by posting to
	 * the semaphore n-times */
	sem_t workers_barrier_1;    /* First sync point */
	sem_t workers_barrier_2;    /* Second sync point */
};

extern struct module_sampler_data sampler_data;
extern struct module_sampler_setting sampler_setting;

/* Worker threads. These are the main framework on worker-side. It must be
 * paired with the right master */
void *poll_lockstep_worker_thread(void* inarg);  /* LOCKSTEP_NOTIFIED(s)*/

/* Worker tasks (subfunctions) of type LOCKSTEP_NOTIFIED. I.e. driven by
 * framework behind poll_lockstep_worker_thread-function */

int poll_fdata(                /* Read data from file. Generic catch all */
	struct sig_sub* sig_sub,   /* variant. I.e. slow & not optimized for */
	int cnt);                  /* the job */

int logfile_fdata(
	struct sig_sub* sig_sub,
	int cnt);

void *blocked_read_thread(void* inarg);
/* Currently worker tasks of type LOCKSTEP_NOTIFIED. Might benefit from  a
 * framework of their own */
int blocked_reader_wtrigger(
	struct sig_sub* sig_sub,
	int cnt);

int blocked_reader(
	struct sig_sub* sig_sub,
	int cnt);

/* Master threads. Each master administrates it's own type of workers */
void *poll_master_thread(void* inarg);

/* Event generator threads */
void *time_eventgen_thread(void* inarg);
void *filemon_eventgen_thread(void* inarg);

/* Various other module-global functions */
int parse_initfile(const char *fn, handle_t *list);
int sampler_prep(handle_t list);
void outputlegends(handle_t list);
int create_executor(handle_t list);
int set_thread_prio(int spolicy, int sprio);
int self_destruct(handle_t list);
void harvest_sample(const handle_t list);
#define SEC( TV ) ((int)TV.tv_sec)
#define USEC( TV ) ((int)TV.tv_usec)
#define NSEC( TV ) ((int)TV.ts_nsec)
struct timeval tv_diff(struct timeval t0, struct timeval t1);
struct timeval tv_add(struct timeval t0, struct timeval t1);
/* Returns diff-time of two 'struct timeval' types in uS
 * This is similar to difftime, except that the fraction part is fixed to 6
 * digits.
 * This is like a macro but with better type-check.
 * Otherwise same limitations and prerequisites as tv_diff
 * */
static inline long tv_diff_us(struct timeval t0, struct timeval t1) {
	struct timeval tv = tv_diff(t0, t1);
	return tv.tv_sec*1000000ul+tv.tv_usec;
};

/* Some helper macros using sync_var (thread safe use/update) */
#define ADD_NWORKERS( inc ) \
	sync_add(&sampler_data.sampler_data_lock, &sampler_data.nworkers, inc)

#define INC_NWORKERS( ) \
	sync_add(&sampler_data.sampler_data_lock, &sampler_data.nworkers)

#define GET_NWORKERS( ) \
	sync_get(&sampler_data.sampler_data_lock, &sampler_data.nworkers)

#endif /* libsampler_local_h */

