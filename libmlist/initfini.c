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

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include "local.h"

/* "Constructor" / "Destructor" */
/*----------------------------------------------------------------------*/
/* Module initializers */
void __init mlist_init(void) {
#ifndef NDEBUG
	printf("==========_init==========\n");
#endif
	assert(!mlistmod_data.isinit);
	mlistmod_data.nlists = 0,
	mlistmod_data.mlists = NULL,

	mlistmod_data.isinit=1;
}

void __fini mlist_fini(void) {
	struct node *tnext;   /* Needed because race could happen */
#ifndef NDEBUG
	printf("==========_fini==========\n");
#endif
	assert(mlistmod_data.isinit);
	/* Destroy all lists if not already done. Note: will not take care of
	 * lists containing allocated payloads. This is not a garbage collector
	 * */
	for (
		;
		mlistmod_data.mlists;
		mlistmod_data.nlists--, mlistmod_data.mlists=tnext
	) {
		fprintf(stderr,
			"WARNING: Destructing un-freed list [%p]. "
				"Possible leak\n",
			mlistmod_data.mlists
		);
		if ((struct listhead*)(mlistmod_data.mlists->pl)){
			struct node *tlist=((struct listhead*)(mlistmod_data.mlists->pl))->pstart;
			int rc=0;

			fprintf(stderr,
				"WARNING: Destructing un-freed pay-load [%p]. "
					"List id: [%p]\n",
				mlistmod_data.mlists->pl , tlist
			);
			rc=dstrct_mlist((handle_t)tlist);
			assert(rc==0);
		}

		tnext=mlistmod_data.mlists->next;
		free(mlistmod_data.mlists);
		mlistmod_data.mlists = NULL;
	}

	mlistmod_data.nlists = 0;
	mlistmod_data.isinit=0;
}
