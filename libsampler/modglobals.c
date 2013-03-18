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

#include "local.h"
#include <limits.h>
#include <stdint.h>

/* Module (global) data. Placed in struct to be easier to find by GDB. Must
 * be global as several c-files belonging to the same module shares this */
struct samplermod_struct samplermod_data = {
	.isinit = 0,

/* Initialize settings with default values */
	.list = 0,
	.ptime = -1,
	.clock_type = AUTODETECT,
	.smplcntr = UINT64_MAX,
	//.plotmode = driveGnuPlot,
	.plotmode = feedgnuplot,
	.delimiter = ';',
	.verbose = 1,
	.dolegend = 1,
	//.dflt_worker = sinus_worker_thread,
	.dflt_worker = poll_worker_thread,
	//.dflt_task = sinus_data,
	.dflt_task = poll_fdata,
	.whatTodo = Lastval, /*Always Output something: feedgnuplot compatible
						   and error detectable */

/* Initialize helper variables */
	.nworkers = 0,
	.presetval = "0",
	.cid_offs = 2,
};
