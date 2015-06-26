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
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

int set_thread_prio(int spolicy, int sprio) {
		pthread_attr_t attr;
		int policy;
		struct sched_param param;
		volatile int rc;

		assert_ext(!
				pthread_getschedparam(pthread_self(),
				&policy, &param));
		assert_ext(!
			pthread_attr_getschedpolicy(&attr, &policy));


		assert_ext(
			!pthread_attr_setschedpolicy(&attr,
				spolicy));
		param.sched_priority = sprio;
		assert_ext(
			!pthread_attr_setschedparam(&attr,
				&param));

		rc=pthread_setschedparam(pthread_self(),
				spolicy, &param);
		if (rc != 0 ) {
				DBG_INF(0,("pthread_setschedparam() failed "
					"with rc=%d (%s)\n", rc, strerror(rc)));
				return(rc);
		};
	return 0;
}

/* Module public functions */
int sampler(const char *filename, int period) {
	int rc;
	handle_t list;

	sampler_setting.ptime = period;

	assert_ext((rc=parse_initfile(filename, &list)) == 0);
	/*TBD: Add better error-handling*/

	assert_ext((rc=sampler_prep(list)) == 0);
	/*TBD: Add better error-handling*/

	/* Output legends if requested, then terminate */
	if (sampler_setting.dolegend && sampler_setting.legendonly ) {
		outputlegends(list);
		goto sampler_done;
	}

	assert_ext((rc=create_executor(list)) == 0);
	/*TBD: Add better error-handling*/

sampler_done:
	assert_ext((rc=mlist_close(list)) == 0);

	//Dear gcc, shut up
	return 0;
}

int sampler_exit() {
	int rc;
	sampler_data.isinit = 0,
	sampler_setting.isinit = 0,
	self_destruct(sampler_data.list);
	sampler_setting.ptime = UINT_MAX;
	return 0;
}

/* Module private functions */
int self_destruct(handle_t list) {
	return 0;
}
