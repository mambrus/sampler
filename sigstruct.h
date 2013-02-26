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


/* Most data are strings representing some sort of numerical value (int,
 * floats of various sizes. This size is currently adapted to string
 * representation of max precision single floats:
 * 2x11 characters + comma  + sign + terminating \0.
 * Kernels usually don't use floats internally. I.e. a value of 25 should
 * be more than enough) */
#define VAL_STR_MAX 25

enum persist {
	no,						/* 0=not persistent i.e. constant reopen */
	persistent,				/* 1=persistent */
	best_effort				/* 2=best effort. If fd returns error, re-open
							   and retry */
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
	enum persist persist;	/* Datafile persistence */
	struct regexp rgx_line;	/* Regex identifying line to parse */
	struct regexp rgx_sig;	/* Signal regex */
	int  *idxs;				/* Sub-match index */
};

#define SID		0
#define SNAME	1
#define SFNAME	2
#define SFDATA	3
#define SPERS	4
#define SRGXL	5
#define SRGXS	6
#define SIDXS	7

/* Sub-signal. A signal can have several sub-signal, but always has at
 * at least one */
struct sig_sub {
	struct sig_data *belong;/* Pointer back to the owner */
	int sub_sig;			/* Which sub-signal this this (if sub-signals
							   defined). 0 if no sub-signals */
	char val[VAL_STR_MAX];  /* Read value */
};

/* Structure containing data to be harvested on each iteration */
struct sig_data {
	int fd;					/* File descriptor of the data-file */
	struct timeval rtime;	/* Time-stamp of last time read. Note: this field
							   only used if needed (i.e. non fixed-rate
							   periodic sampling). */
	int is_updated;			/* Updated during this run. Example reasons for not
							   updating:
							   Periodic: Data-file stopped existing or error
							   Event: Not updated since last run, or error */
	int nsigs;				/* Number of sub-signals */
	struct sig_sub *sigs;	/* Array of sub-signals. Note: sub_sig is of fixed
							   size. If this change, this table must be converted
							   to '**' or '*[]' (TBD) */
};

/* One compile signal. Might be missing pthread owning signal to make killing
 * easier by main thread just parsing data (TBD) */
struct smpl_signal {
	struct sig_def sig_def;
	struct sig_data sig_data;
};

#endif /* sigstruct_h */
