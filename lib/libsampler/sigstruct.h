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

/* Internal. I.e. non-public. Exists only for ease of readability */

#ifndef sigstruct_h
#define sigstruct_h

#include <pthread.h>
#include <regex.h>
#include <parsers.h>
#include <stdint.h>
#include <mlist.h>
#include <mqueue.h>

/* Each thread belongs to a matrix of whatever "task" it has and which
 * function it should work under, i.e. what kind of worker it is. Easiest
 * way to understand this is by example. For example a time driven group of
 * threads that should work in lock-step. In this case each thread is a
 * "LOCKSTEP_POLLED" as each thread will use share synchronization points to
 * commonly agree when data is ready to be delivered.
 *
 * The other extreme is a thread that can block. This type of thread can't
 * work in lock-step and has to work independently (asynchronous). As in
 * every multi-threaded program with shared resources there has to be some
 * synchronization, one critical area is the shared list which inherently is
 * not thread safe as it intentionally lack all kinds of synchronization in
 * favor of speed, i.e. accessing it has to be done with careful
 * consideration. Another place worth consideration is the final output as
 * output will both access the shared list, but most importantly may run in
 * a thread of it's own and might compete with about the output. The latter
 * is managed by design by *one* extra lock defining the final outputting as
 * a critical region. This fits quite natural as output needs to be buffered
 * when intermixing data from different sources with different timestamps.
 * The main point is that the functionality itself will not run in
 * lock-step.
 *
 * Up until now the concept of master-worker has been deliberately avoided
 * in the above discussion.  This is because the "threads" mentioned above
 * are really oriented around the concept of "sub_signals", and the
 * crosspoint of the two entities  define a modus of operation of *both* a
 * master and at least one worker. Thus both together define a framework of
 * operation which in turn may define one or no master, one worker (basic
 * design, various optimization allow for fewer threads) and potentially one
 * or several helper threads.
 */
enum daq_type { LOCKSTEP_POLLED, LOCKSTEP_NOTIFIED, FREERUN_ASYNCHRONOUS };

/* [Up to 16] Thread modes/strategies: Define thread optimizations. Note
 * that not all are guaranteed to work for all modalities */
enum thread_ops {
	FULL_THREAD=0,
	NO_THREAD_SUB_SIG=1,
	NEVER_THREAD_SUB_SIG=2
};

/* [Up to 16] NUCE-codes: What to do if worker hasn't updated data due to that it's
 * blocked (if that is allowed for the thread i.e.) or SAMPLE has detected the
 * data is unchanged (polling with detectable timestamps) or when
 * data-source has stopped to exist (hot-plug sysfs) */
enum nuce {
	NOTHING = 0,
	LASTVAL,
	PRESET_SIG,  /*TBD: Code prepared, parser and manager tools are not */
	PRESET_SMPL
};

/* [Up to 16] Parser variants. */
enum parser {
	PARS_REGEXP = 0,    /* Default, will always work */
	PARS_CUT,           /* Parses equals cut -d[<delim>] -f[<field>] */
	PARS_REVCUT         /* As above, but field-list counts from end */
};

/* Most data are strings representing some sort of numerical value (int,
 * floats of various sizes. This size is currently adapted to string
 * representation of max precision single floats:
 * 2x11 characters + comma  + sign + terminating \0.
 * Kernels usually don't use floats internally. I.e. a value of 25 should
 * be more than enough) */
#ifndef VAL_STR_MAX
#define VAL_STR_MAX 25
#endif

/* A sanity value for signal names. Used for out buffer in legends as this
 * might have to be dynamic */
#define NAME_STR_MAX 40

/* Bit-values defining data-file operation per signal. Note that the final
 * value 0 (i.e.  * no bit set) has a special meaning and can't be "OR":ed
 * to be compared * only be compared with 0 */

/* Best effort. Try to reopen if not existing.*/
#define NOASSUMPTION    ((uint64_t)0<<0)

/* Sampler will always close after read and reopen prior next read. Useful
 * for files (normal data-files) that might have been renamed like normal
 * log-files.*/
#define OPENCLOSE       ((uint64_t)1<<0)

/* File can block. For event-driven sampling this is one possible trigger.
 * I.e. one file getting ready to deliver, drivers the event even if files
 * that can't block gets read. In such case whether or not data gets
 * delivered depends on if the file has support or utime. If more than one
 * blocking files are part of a sample-configuration, the ones still blocked
 * will not be asked to deliver data. */
#define CANBLOCK        ((uint64_t)1<<1)

/* File can trigger event */
#define TRIGGER         ((uint64_t)1<<2)

/* File is "timed", i.e. updates are recorded and file has support for
 * st_mtime */
#define TIMED           ((uint64_t)1<<3)

/* This is for files that can stay open but which needs rewinding. Typical
 * example would be persistent files in sysfs. Warning, use this modality
 * with care. It's an optimization setting and many files in sysfs actually
 * do cease to exist from time to time */
#define NO_REWIND       ((uint64_t)1<<4)

/* For signals via input that CANBLOCK, but that don't have TRIGGER bit set:
 * When set, indata is buffered and outputed with background period.
 * If not set (default), only the last data per period will appear in
 * output, the other data are squashed (discarded), hence the name. */
#define DONT_SQUASH       ((uint64_t)1<<5)

/*File must exist and always deliver data. Failure doing so terminates
execution */
#define ALWAYS          ((uint64_t)1<<31)


/* Sample operation mode. Determines how each signal's corresponding file
 * behaves. Implicitly what can be done with it, how it's expected to behave
 * and how it can alter a samples temporal domain */
union sops {
	uint32_t	mask; /* Clean access, works always */

	/* Bits below should be endian-ness OK as we are only working with 1 bit
	 * at a time. AFAIK there is no guarantee to avoid gaps and members are
	 * always considered "int" no matter of the element size. This is no big
	 * deal as code will not work across platform (i.e. not protocol and
	 * could hence had chosen full scale data-types but I like the concept
	 * of bit-masks and this is a way to combine them */
	struct {
		uint32_t openclose  : 1;                /*    <                     */
		uint32_t canblock   : 1;                /*    |  Nibble 1           */
		uint32_t trigger    : 1;                /*    |                     */
		uint32_t timed      : 1;                /*    <                     */
		uint32_t no_rewind  : 1;                /*    <  Nibble 2           */
		uint32_t dont_squash: 1;                /*    <                     */
		uint32_t __pad0     : 2;                /*    <                     */
		enum thread_ops tops: 4;                /*    <  Nibble 3           */
		uint32_t tprio      : 4;                /*    <  Nibble 4           */
		enum parser parser  : 4;                /*    <  Nibble 5           */
		uint32_t __pad1     : 8;                /*    <  Nibble 6,7         */
		uint32_t __pad2     : 3;                /*    <  Nibble 8           */
		uint32_t always     : 1;                /*    <                     */
	} __attribute__((__packed__,aligned (4))) bits;
};

struct regexp {
	char *str;			/* The string originally describing the regex. Note:
						   Might be invalid data depending on usage (as it's
						   used only once to compile the regex below). */
	regex_t rgx;		/* Compiled version of the regex. This data must
						   always be reliable */
};

/* Broken down signal. This struct is 1:1 mapped of one parsed definition
 * line */
struct sig_def {
	int  id;				/* Line number */
	char *name;				/* Signal name (symbolic) */
	char *fname;			/* Signal name from file */
	char *fdata;			/* Data-file name */
	union sops sops;	    /* Sample op-mode (sub-signals shares this)*/
	struct regexp rgx_line;	/* Regex identifying line to parse */
	int  lindex;			/* n'th line index. '1'=first, '0'=special */
	struct regexp rgx_sig;	/* Signal regex */
	int  *idxs;				/* Sub-match indexes (array)*/
	char *issA;				/* Is String Array: Sub-match value is a string
							   'Y'/'N' (array)*/
	enum nuce nuce;			/* "Not updated" behavior code*/
};

/* Field indexes in definition line. If more data added, add to end to avoid
 * breaking code. More complete information in the README */
#define SID		0 /* Fake, doesn't exist. Index used in result as counter */
#define SNAME	1 /* Signal name (symbolic)                               */
#define SFNAME	2 /* Signal name from file                                */
#define SFDATA	3 /* Data-file name                                       */
#define SFOPMOD	4 /* Datafile modality (32-bit bit-mask code)             */
#define SRGXL	5 /* Regexp identifying which line to parse               */
#define SLIDX	6 /* N'th line index match (positive int)                 */
#define SRGXS	7 /* Signal regex                                         */
#define SIDXS	8 /* Sub-match index                                      */
#define SNUCE	9 /* "Not updated" behavior code (positive int)           */

/* Constant string to use as default output if no update has occurred. To be
 * set specifically to help debug race conditions or other harvest errors*/
#define SIG_VAL_DFLT "VALUE_UNDEFINED"

/* Value queues for asynchronous modes. Struct defines a two-way queue, i.e
 * one queue with two handles, one handle for reading the other for writing,
 * hence "pipe". Not to be confused with a UNIX pipe even though the
 * use-case is identical and also happens to be used for piped input, among
 * others as it can be used for any kind of asynchronous input (sockets,
 * char-dev devices e.t.a.).
 *
 * Pipe is strictly meant to be two-point only, one receiver (harvester) and
 * one data producer. The receiver is usually non-blocking (note), sender is
 * blocking (note). This somewhat unusual combination is by design so that
 * one thread can empty many queues without blocking. Block-on-send is never
 * really supposed to happen except if queue is full which should never
 * happen (or design is wrong).
 *
 * Purpose of this pipe is to introduce RT-relaxation. I.e. length of queue
 * functions as a buffer and both propagation delay and jitter absorption
 * can hence be deducted by the length and in/out rates.
 *
 * */
struct val_pipe {
	mqd_t write;
	mqd_t read;
};

/* Simple message to be sent on val_pipe. Note that message is intentionally
 * small and that buff is a pointer. This is to avoid copying in favour for
 * speed, but also means sender and receiver *has to* agree on who owns
 * buffer, and if it's dynamic, who creates/destroys and when. */
struct val_msg {
	char *valS;
	int len;
};

/* Sub-signal. A signal can have several sub-signal, but always has at
 * at least one */
struct sig_sub {
	int fd;					/* File descriptor of the data-file */
	struct sig_data *ownr;  /* Pointer back to the owner */
	int id;                 /* "ID" of the signal. Per sample unique. */
	int sub_id;				/* Which sub-signal this this (if sub-signals
							   defined). 0 if no sub-signals */
	int  idx;				/* Corresponding subindex value (copy of def) */
	int  isstr;				/* Value is a string (copy of def) */
	char val[VAL_STR_MAX];  /* Read value (for number)*/
	char *valS;  			/* Read value (for strings). This permits
							   infinitely long texts as "values" but
							   handling is slower */
	char presetval[VAL_STR_MAX]; /* What to print in case needed */
	pthread_t worker;		/* Worker thread ID*/
	void *(*thread_sfun)    /* Start-function for the thread */
		(void*);
	int (*sub_function)		/* Specific task-function for the job */
		(struct sig_sub*, int);
	struct timeval rtime;	/* Time-stamp of last time read. Note: this field
							   only used if needed (i.e. non fixed-rate
							   periodic sampling). */
	int is_updated;			/* Updated during this run. Example reasons for not
							   updating:
							   Periodic: Data-file stopped existing or error
							   Event: Not updated since last run, or error */
	enum daq_type daq_type; /* What the workers main task is (if applicable) */
	FILE *tfile;			/* Each thread operate on it's own file */
	long int fp_curr;		/* File-position in file after last run */
	long int fp_was;		/* File-position before change with fseek (debug) */
	long int fp_valid;		/* Position in file after last recognized data
							   or if no data recognized last run, the same
							   as fp_last */
	/* Value pipe for asynchronous modes. pipe  itself exists in "void" and
	 * can only be reached via it's handles, or it's name. Name is deducted
	 * by the sub-signal owning it. */
	struct val_pipe *val_pipe;
	char *name;             /* Signal name in clear-text. Can be used to
							   identify queue */
	pthread_mutex_t lock;	/* protect data in sig_sub */
};

/* Structure containing data to be harvested on each iteration */
struct sig_data {
	struct smpl_signal *ownr; 	/* Pointer back to the owner */
	int nsigs;					/* Number of sub-signals */
	struct sig_sub *sig_sub;	/* Array of sub-signals. Note: sub_sig is of
								   fixed size. If this change, this table
								   must be converted to '**' or '*[]' (TBD) */
	int backlog_events;			/* Number of self-induced events (monitored
								   file back-log)*/
	int cleared_backlog;		/* If cleared a back-logged even during run or
							   		not.*/
	handle_t sub_list;			/* Handle to list of sub_signals, if any */
};

/* One complete signal. Might be missing pthread owning signal to make killing
 * easier by main thread just parsing data (TBD) */
struct smpl_signal {
	struct sig_def sig_def;
	struct sig_data sig_data;
};

#endif /* sigstruct_h */
