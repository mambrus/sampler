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

/* Local interface & stuff of sampler-module  */

#ifndef local_h
#define local_h
#include <mlist.h>
#include <stdio.h>

#include "sigstruct.h"

#define __init __attribute__((constructor))
#define __fini __attribute__((destructor))

int parse_initfile(const char *fn, handle_t *list);
int create_executor(handle_t list);
int self_destruct(handle_t list);
void harvest_sample(const handle_t list);

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

struct samplermod_struct {
	int isinit;
	handle_t list;
	int ptime;              /* Period time in us (Max: 4294/2=2147s
							   = 35min) */
	uint64_t smplcntr;
	enum plotmode plotmode;
	char delimiter;
	enum whatTodo whatTodo;
	int cid_offs;
	char presetval[VAL_STR_MAX]; /* What to print in case is needed */
};

extern struct samplermod_struct samplermod_data;

#endif /* local_h */

