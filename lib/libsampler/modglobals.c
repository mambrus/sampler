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

/* Module (global) data in two global structs to be easier to reference (by
 * GDB and code). */

#include "local.h"
#include <limits.h>
#include <stdint.h>
#include <pthread.h>

/* Run-time data:
   Thread shared data (must be global) */
struct module_sampler_data sampler_data = {
	.isinit = 0,

	/* Initialize fact variables with unreasonable values meant to fail if
	 * run-time initialization fails */
	.list = 0,
	.tstarted = {0},

	/* run diagnostics */
	.diag_lock = PTHREAD_RWLOCK_INITIALIZER,
	.diag = {
		.smplID     = INT_MAX,
		.triggID    = INT_MAX,
		.texec      = INT_MIN,
		.tp1        = INT_MIN,
		.tp2        = INT_MIN,
		.format_ary = {DIAG_NONE},
	},

	/* Initialize helper variables */
	.nworkers = 0,
	.ndied=0,
	.cid_offs = 2,
	.files_monitored = 0,
	.fd_notify = 0,
	.mx_stderr = PTHREAD_MUTEX_INITIALIZER,
	.mx_master_up = PTHREAD_MUTEX_INITIALIZER,

	/* Use this general rw-lock if sampler_data needs to be accessed
	   concurrently. If use is frequent, invent a specific (like in the case
	   below)*/
	.sampler_data_lock = PTHREAD_RWLOCK_INITIALIZER,
};

/* Settings:
   Need not be global ATM, but is convenient and avoids an excessive use of
   stack references which is instead reserved for data transformation (this
   is static after program starts) */
struct module_sampler_setting sampler_setting = {
	.isinit = 0,
	.ptime = -1,
	.clock_type = AUTODETECT,
	//.plotmode = driveGnuPlot,
	.plotmode = feedgnuplot,
	.delimiter = ';',
	.debuglevel = -1,
	.verbose = 0,
	.dolegend = 1,
	.legendonly = 0,
#ifdef SMPL_FALLBACK_VAL
	.presetval = xstr(SMPL_FALLBACK_VAL),
#else
	.presetval = "0",
#endif
	.realtime = 0, /* If set, policies and priorities below will be set
					  accordingly to known-good values without the need to
					  set them in specifically (and risk causing system
					  melt-down by mistake)
					*/

	/* Advanced settings. Should not be encouraged to be changed by either
	 * the end-user or coder.
	 *
	 * NOTE: !!!They can potentially starve out the
	 * kernel if misused!!!
	 *
	 * Set to "magic" values (or at least insane) to indicated no change
	 * intended. "No change" is the default as it requires no root permissions
	 */
	.policy_master    = INT_MAX,
	.policy_workers   = INT_MAX,
	.policy_events    = INT_MAX,
	.prio_master      = INT_MAX,
	.prio_workers     = INT_MAX,
	.prio_events      = INT_MAX,

	.procsubst.inherit_env = 1,
	.procsubst.path=NULL,
	.procsubst.env=NULL,
	.tmpdir=NULL,

	/* Use this rw-lock if sampler_setting needs to be accessed concurrently */
	.sampler_setting_lock = PTHREAD_RWLOCK_INITIALIZER,
};
