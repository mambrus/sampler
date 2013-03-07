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

#include <assert.h>
#include <sampler.h>
#include <mlist.h>
#include "assert_np.h"
#include "local.h"
#include <limits.h>

/* Module public functions */
int sampler_init(const char *filename, int period) {
	int rc;
	handle_t list;

	rc=parse_initfile(filename, &list);
	/*TBD: Add better error-handling*/
	assert(rc==0);

	samplermod_data.ptime = period;

	rc=create_executor(list);
	/*TBD: Add better error-handling*/
	assert(rc==0);

	//Dear gcc, shut up
	return(0);
}

int sampler_fini() {
	int rc;
	samplermod_data.isinit = 0,
	self_destruct(samplermod_data.list);
	samplermod_data.ptime = UINT_MAX;
}

/* Module private functions */
int self_destruct(handle_t list) {
}
