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
#include "local.h"

struct listhead;
#define LDATA struct listhead
/* Administrative keeper of all lists */
struct modstruct {
	int isinit;           /* Is this module initialized? I.e. does mlist
							 below contain a valid pointer e.t.a.*/
	int nlists;           /* Current number of lists in  list */
	struct node *mlists;  /* List-head of lists. No need to sort to find.
							 Handle is hash-key*/
} moddata = {
	.isinit = 0,
	.nlists = 0,
	.mlists = NULL,
};

/* Data of this struct is the payload for the mlist variable in modstruct.
 * It's the administrative keeper of each list */
struct listhead {
	int iindx;            /* Iterator index. File-pointer so to speak */
	int nelem;            /* Current size of this list */
	int pl_sz;            /* pay-load size */

	/* Caller provided function used to search & sort list. Can be NULL if
	 * search and sort is not supported */
	int (*cmpfunc)(LDATA *lval, LDATA *rval);
	struct node *mlist;  /* The real list */
};

#include <mlist.h>
/* "Constructor" / "Destructor" */
/*----------------------------------------------------------------------*/
/* Module initializers */
void __init mlist_init(void) {
#ifndef NDEBUG
	printf("==========_init==========\n");
#endif
	assert(!moddata.isinit);
	moddata.nlists = 0;
	moddata.mlists = malloc(sizeof(struct node));
	assert(moddata.mlists);
	memset(moddata.mlists, 0, sizeof(struct node));
	moddata.isinit=1;
}

void __fini mlist_fini(void) {
	struct node *tnext;   /* Needed because race could happen */
#ifndef NDEBUG
	printf("==========_fini==========\n");
#endif
	assert(moddata.isinit);
	/* Destroy all lists if not already done. Note: will not take care of
	 * lists containing allocated payloads. This is not a garbage collector
	 * */
	for (
		;
		moddata.mlists;
		moddata.nlists--, moddata.mlists=tnext
	) {
		fprintf(stderr,
			"WARNING: Destructing un-freed list [%p]. "
				"Possible leak\n",
			moddata.mlists
		);
		if ((struct listhead*)(moddata.mlists->pl)){
			struct node *tlist=((struct listhead*)(moddata.mlists->pl))->mlist;
			int rc=0;

			fprintf(stderr,
				"WARNING: Destructing un-freed pay-load [%p]. "
					"List id: [%p]\n",
				moddata.mlists->pl , tlist
			);
			rc=dstrct_mlist((handle_t)tlist);
			assert(rc==0);
		}

		tnext=moddata.mlists->next;
		free(moddata.mlists);
		moddata.mlists = NULL;
	}

	moddata.nlists = 0;
	moddata.isinit=0;
}
/*----------------------------------------------------------------------*/
int create_mlist(
		int sz,
		int (*cmpfunc)(LDATA *lval, LDATA *rval),
		handle_t *hndl
) {
	struct node *p=NULL;
	assert(moddata.isinit);

	/*Create a list head in the empty payload*/
	moddata.mlists->pl = malloc(sizeof(struct listhead));
	assert(moddata.mlists->pl);
	memset(moddata.mlists->pl, 0, sizeof(struct listhead));
	/*Create a new empty node in front*/
	moddata.mlists->prev = malloc(sizeof(struct node));
	assert(moddata.mlists->prev);
	memset(moddata.mlists->prev, 0, sizeof(struct node));
	moddata.mlists->prev->next = moddata.mlists;
	/*Cotinue initialize our nodes payload*/
	moddata.mlists->pl->cmpfunc = cmpfunc;
	/* Don't pre-create 1:st node. They are supposed to get sorted on creation
	 * and we don't have that data yet */
	moddata.mlists->pl->mlist = NULL;
	*hndl=(handle_t)moddata.mlists;
	moddata.nlists++;
	/* Prepare for next creation */
	moddata.mlists=moddata.mlists->prev;
	return 0;
};

int delete_mlist(handle_t handle) {
	assert(moddata.isinit);
};
int dstrct_mlist(handle_t handle) {
	assert(moddata.isinit);
};

struct node *mlist_next(handle_t handle) {
	assert(moddata.isinit);
};
struct node *mlist_prev(handle_t handle) {
	assert(moddata.isinit);
};

struct node *mlist_head(handle_t handle) {
	assert(moddata.isinit);
	return (*(struct listhead*)(handle)).mlist;
};
struct node *mlist_tail(handle_t handle) {
	assert(moddata.isinit);
};

struct node *mlist_new(handle_t handle) {
	assert(moddata.isinit);
};
struct node *mlist_new_last(handle_t handle) {
//	struct listhead *p;
	assert(moddata.isinit);
//(*(struct listhead*)(handle))
};
struct node *mlist_new_first(handle_t handle) {
	assert(moddata.isinit);
};

struct node *mlist_add(handle_t handle, const LDATA *data) {
	assert(moddata.isinit);
};
struct node *mlist_add_last(handle_t handle) {
	assert(moddata.isinit);
};
struct node *mlist_add_first(handle_t handle) {
	assert(moddata.isinit);
};

struct node *mlist_del(handle_t handle) {
	assert(moddata.isinit);
};
struct node *mlist_del_last(handle_t handle) {
	assert(moddata.isinit);
};
struct node *mlist_del_first(handle_t handle) {
	assert(moddata.isinit);
};

struct node *mlist_dsrct(handle_t handle) {
	assert(moddata.isinit);
};
struct node *mlist_dsrct_last(handle_t handle) {
	assert(moddata.isinit);
};
struct node *mlist_dsrct_first(handle_t handle) {
	assert(moddata.isinit);
};

