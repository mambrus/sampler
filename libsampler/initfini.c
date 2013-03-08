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

/* Module initializers. Not much to do here for init, but fini might have
 * clean-up to do, iterate through the list and:
 * * kill threads
 * * Free allocated memory
 */
void __init __sampler_init(void) {
#ifndef NDEBUG
	fprintf(stderr,"==========_init  "__FILE__"==========\n");
#endif
	assert(!samplermod_data.isinit);
	samplermod_data.smplcntr=0;

	samplermod_data.isinit=1;
}

void __fini __sampler_fini(void) {
#ifndef NDEBUG
	fprintf(stderr,"==========_fini "__FILE__" ==========\n");
#endif
	if (!samplermod_data.isinit)
		/* Someone allready did this in a more controlled way. Nothing to do
		 * here, return */
		 return;
	self_destruct(samplermod_data.list);
	samplermod_data.isinit=0;
}
