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
#include "mlist.h"


/* Administrative keeper of all lists */
struct modstruct {
	int isinit;           /* Is this module initialized? I.e. does mlist
							 below contain a valid pointer e.t.a.*/
	int nelem;            /* Current number of lists in  list */
	int hndlpool;         /* Variable to make lists unique */
	struct node *mlists;  /* List-head of lists. Sorted insert for quick
							 search, no need to sort it */
						  /* Function to search in list. Module provided */
	int (*cmpfunc)(struct modstruct *lval, struct modstruct *rval);
} mdldata = {
	.isinit = 0,
	.nelem = 0,
	.hndlpool = 0,
	.mlists = NULL,
	.cmpfunc = NULL
};

/* Data of this struct is the payload for the mlist variable in modstruct.
 * It's the administrative keeper of each list */
struct listhead {
	int id;               /* A unique value identifying the list */
	int iindx;            /* Iterator index. File-pointer so to speak */
	int nelem;            /* Current size of this list */

	/* Caller provided function used to search & sort list. Can be NULL if
	 * search and sort is not supported */
	int (*cmpfunc)(LDATA *lval, LDATA *rval);
	struct node *mlist;  /* The real list */
};

/* Search function to search for list of correct ID */
static int cmplists(struct modstruct *lval, struct modstruct *rval){
}

/* "Constructor" / "Destructor" */
/*----------------------------------------------------------------------*/
/* Module initializers */
void __init mlist_init(void) {
#ifndef NDEBUG	
	printf("==========_init==========\n");
#endif
	mdldata.nelem = 0;
	mdldata.hndlpool = 0;
	mdldata.mlists = NULL;
	mdldata.cmpfunc=cmplists;
	mdldata.isinit=1;
}

void __fini mlist_fini(void) {
	struct node *tnext;   /* Needed because race could happen */
#ifndef NDEBUG	
	printf("==========_fini==========\n");
#endif
	/* Destroy all lists if not already done. Note: will not take care of
	 * lists containing allocated payloads. This is not a garbage collector
	 * */
	for (
		;
		mdldata.mlists; 
		mdldata.nelem--, mdldata.mlists=tnext
	) {
		fprintf(stderr,"WARNING: Destructing un-freed list [%p]. Possible leak\n",mdldata.mlists);
		if ((struct listhead*)(mdldata.mlists->pl)){
			int tid=((struct listhead*)(mdldata.mlists->pl))->id;
			struct node *tlist=((struct listhead*)(mdldata.mlists->pl))->mlist;
			int rc=0;

			fprintf(stderr,"WARNING: Destructing un-freed pay-load [%p]. List id=%d [%p]\n",
					mdldata.mlists->pl, tid, tlist);
			rc=dstrct_mlist(tid);
			assert(rc==0);
		}

		tnext=mdldata.mlists->next;
		free(mdldata.mlists);
		mdldata.mlists = NULL;
	}
	
	mdldata.nelem = 0;
	mdldata.hndlpool = 0;
	mdldata.cmpfunc = NULL;
	mdldata.isinit=0;
}
/*----------------------------------------------------------------------*/

int create_mlist(int sz, int (*cmpfunc)(LDATA *lval, LDATA *rval)) {
	assert(mdldata.isinit);
};

int delete_mlist(int handle) {
	assert(mdldata.isinit);
};
int dstrct_mlist(int handle) {
	assert(mdldata.isinit);
};

struct node *mlist_next(int handle) {
	assert(mdldata.isinit);
};
struct node *mlist_prev(int handle) {
	assert(mdldata.isinit);
};

struct node *mlist_head(int handle) {
	assert(mdldata.isinit);
};
struct node *mlist_tail(int handle) {
	assert(mdldata.isinit);
};

struct node *mlist_new(int handle) {
	assert(mdldata.isinit);
};
struct node *mlist_new_last(int handle) {
	assert(mdldata.isinit);
};
struct node *mlist_new_first(int handle) {
	assert(mdldata.isinit);
};

struct node *mlist_add(int handle) {
	assert(mdldata.isinit);
};
struct node *mlist_add_last(int handle) {
	assert(mdldata.isinit);
};
struct node *mlist_add_first(int handle) {
	assert(mdldata.isinit);
};

struct node *mlist_del(int handle) {
	assert(mdldata.isinit);
};
struct node *mlist_del_last(int handle) {
	assert(mdldata.isinit);
};
struct node *mlist_del_first(int handle) {
	assert(mdldata.isinit);
};

struct node *mlist_dsrct(int handle) {
	assert(mdldata.isinit);
};
struct node *mlist_dsrct_last(int handle) {
	assert(mdldata.isinit);
};
struct node *mlist_dsrct_first(int handle) {
	assert(mdldata.isinit);
};

