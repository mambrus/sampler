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
#include <stdint.h>

/* Most data are strings representing some sort of numerical value (int,
 * floats of various sizes. This size is currently adapted to string
 * representation of max precision single floats:
 * 2x11 characters + comma  + sign + terminating \0.
 * Kernels usually don't use floats internally. I.e. a value of 25 should
 * be more than enough) */
#define VAL_STR_MAX 25

/* Bit-values defining data-file operation per signal. Note that the final
value 0 (i.e.  * no bit set) has a special meaning and can't be "OR":ed to be
compared * only be compared with 0 */

/* Best effort. Try to reopen if not existing.*/
#define NOASSUMPTION    ((uint64_t)0<<0)

/* Sampler will always close after read and reopen prior next read. Useful for
files (normal data-files) that might have been renamed like normal log-files.*/
#define OPENCLOSE       ((uint64_t)1<<0)

/* File can block. For even-driven sampling this is one possible trigger.  I.e.
one file getting ready to deliver drivers the event even if files that can't
block gets read. In such case whether or not data gets delivered depends on if
the file has support or utime. If more than one blocking files are part of a
configuration, the ones still blocked will not be asked to deliver data. */
#define CANBLOCK        ((uint64_t)1<<1)

/*File must exist and always deliver data. Failure doing so terminates
execution */
#define ALWAYS          ((uint64_t)1<<31)

/* Enumeration to help determine combination of blocking state. Mostly not
 * useful and entries night be missing, but has it's usefulness when
 * debugging */
enum persist {
	automagic         = NOASSUMPTION,
	notpersistent     = OPENCLOSE,
	eventdriver       = CANBLOCK,
	canblock          = CANBLOCK |  OPENCLOSE,
	sentinel = UINT32_MAX   /*Assures enum will always be represented in at
							  least this number of bits (even if members
							  them self are still interpreted and limited to
							  "int")*/
};
/* Hmm, above this is probably a crappy attempt and would scale enormously
badly if covering more than a few bits.*/

/* Calling "persistence" for file-operation mode instead as it's more generic*/
union fopmode {
	uint32_t	mask; /* Clean access, works always */

	/* Bits below should be used for debugging purposes. Should be
	 * endian-ness OK as we are only working with 1 bit at a time. AFAIK
	 * there is no guarantee to avoid gaps and members are always considered
	 * "int" no matter of the element size. This is no big deal as code will
	 * not work on cross-platform data (i.e. could had chosen full scale
	 * data-types but I like the concept of bit-masks and this is a way to
	 * combine them	 */
	struct {
		uint32_t openclose  : 1;
		uint32_t canblock   : 1;
		uint32_t __pad1     : 29;
		uint32_t always     : 1;
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
	union fopmode fopmode;	/* Datafile op-mode (sub-signals shares this)*/
	struct regexp rgx_line;	/* Regex identifying line to parse */
	struct regexp rgx_sig;	/* Signal regex */
	int  *idxs;				/* Sub-match index */
};

/* Field indexes in definition line. If more data added, add to end to avoid
 * breaking code. More complete information in the README */
#define SID		0 /* Fake, doesn't exist. Index used in result as counter */
#define SNAME	1 /* Signal name (symbolic)                               */
#define SFNAME	2 /* Signal name from file                                */
#define SFDATA	3 /* Data-file name                                       */
#define SPERS	4 /* Datafile persistence                                 */
#define SRGXL	5 /* Regexp identifying which line to parse               */
#define SRGXS	6 /* Signal regex                                         */
#define SIDXS	7 /* Sub-match index                                      */

/* Sub-signal. A signal can have several sub-signal, but always has at
 * at least one */
struct sig_sub {
	struct sig_data *ownr;  /* Pointer back to the owner */
	int id;                 /* "ID" of the signal. I.e which column it
							   updates in the output. */
	int sub_id;				/* Which sub-signal this this (if sub-signals
							   defined). 0 if no sub-signals */
	char val[VAL_STR_MAX];  /* Read value */
	pthread_t worker;		/* Worker thread */
	struct timeval rtime;	/* Time-stamp of last time read. Note: this field
							   only used if needed (i.e. non fixed-rate
							   periodic sampling). */
	int is_updated;			/* Updated during this run. Example reasons for not
							   updating:
							   Periodic: Data-file stopped existing or error
							   Event: Not updated since last run, or error */
};

/* Structure containing data to be harvested on each iteration */
struct sig_data {
	int fd;					/* File descriptor of the data-file */
	struct smpl_signal *ownr; /* Pointer back to the owner */
	int nsigs;				/* Number of sub-signals */
	struct sig_sub *sigs;	/* Array of sub-signals. Note: sub_sig is of fixed
							   size. If this change, this table must be converted
							   to '**' or '*[]' (TBD) */
	pthread_t master;		/* Master thread (if master/worker model is used)*/
};

/* One complete signal. Might be missing pthread owning signal to make killing
 * easier by main thread just parsing data (TBD) */
struct smpl_signal {
	struct sig_def sig_def;
	struct sig_data sig_data;
};

#endif /* sigstruct_h */
