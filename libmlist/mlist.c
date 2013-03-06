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

static inline struct node *forward(off_t n) {
};

static inline struct node *reverse(off_t n) {
};

int create_mlist(
		int sz,
		int (*cmpfunc)(LDATA *lval, LDATA *rval),
		handle_t *hndl
) {
	struct listheader *L=NULL;
	assert(mlistmod_data.isinit);
	
	if (!mlistmod_data.mlists) {
		/* If list of lists is all empty, create also first empty node  */
		mlistmod_data.mlists = malloc(sizeof(struct node));
		assert(mlistmod_data.mlists);
		memset(mlistmod_data.mlists, 0, sizeof(struct node));
		/* nodes prev/next left zero (NULL) on purpose */
	} else {
		/*Create a new empty node at tail*/
		mlistmod_data.mlists->next = malloc(sizeof(struct node));
		assert(mlistmod_data.mlists->next);
		memset(mlistmod_data.mlists->next, 0, sizeof(struct node));
		mlistmod_data.mlists->next->prev = mlistmod_data.mlists;
		/*Last one created becomes list head (i.e. order reversed)*/
		mlistmod_data.mlists = mlistmod_data.mlists->next;
	}
	
	mlistmod_data.mlists->pl = malloc(sizeof(struct listheader));
	assert(mlistmod_data.mlists->pl);

	/*Cotinue initialize our list headers payload*/
	L=mlistmod_data.mlists->pl;
	memset(L, 0, sizeof(struct listheader));
	/* Not really needed to to the following 3*/
	/* L->p       = mlistmod_data.mlists; WRONG*/
	L->p       = NULL;
	L->phead   = NULL;
	L->ptail   = NULL;
	L->pl_sz   = sz;
	L->cmpfunc = cmpfunc;
	mlistmod_data.nlists++;
	*hndl=(handle_t)L;
	return 0;
};

int delete_mlist(const handle_t handle) {
	assert(mlistmod_data.isinit);
	struct listheader *L=(struct listheader *)handle;
	assert(!TBD_UNFINISHED);
};
int dstrct_mlist(const handle_t handle) {
	assert(mlistmod_data.isinit);
	struct listheader *L=(struct listheader *)handle;
	assert(!TBD_UNFINISHED);
};

struct node *mlist_next(const handle_t handle) {
	assert(mlistmod_data.isinit);
	struct listheader *L=(struct listheader *)handle;
	if (!L->p->next)
		return(NULL);

	L->p = L->p->next;
	return(L->p);
};

struct node *mlist_prev(const handle_t handle) {
	assert(mlistmod_data.isinit);
	struct listheader *L=(struct listheader *)handle;
	if (!L->p->prev)
		return(NULL);

	L->p = L->p->prev;
	return(L->p);
};

struct node *mlist_head(const handle_t handle) {
	assert(mlistmod_data.isinit);
	struct listheader *L=(struct listheader *)handle;
	return L->p = L->phead;
};
struct node *mlist_tail(const handle_t handle) {
	assert(mlistmod_data.isinit);
	struct listheader *L=(struct listheader *)handle;
	return L->p = L->ptail;
};

struct node *mlist_add(const handle_t handle, const LDATA *data) {
	assert(mlistmod_data.isinit);
	struct listheader *L=(struct listheader *)handle;
	if (!L->cmpfunc)
		return mlist_add_last(handle, data);

	
	assert(!TBD_UNFINISHED);
};
struct node *mlist_add_last(const handle_t handle, const LDATA *data) {
	assert(mlistmod_data.isinit);
	struct listheader *L=(struct listheader *)handle;
	
	
	if (!L->nelem) {
		/* If list is all empty, create also first empty node  */
		L->p = malloc(sizeof(struct node));
		assert(L->p);
		memset(L->p, 0, sizeof(struct node));
		/*All 3 element pointers point at the same */
		L->phead = L->ptail = L->p;
		/* nodes prev/next left zero (NULL) on purpose */
	} else {
		/*Create a new empty node at tail*/
		L->ptail->next = malloc(sizeof(struct node));
		assert(L->ptail->next);
		memset(L->ptail->next, 0, sizeof(struct node));
		L->ptail->next->prev = L->ptail;
		L->ptail = L->ptail->next;
		/* Note. No need to save/restore list header temporary as it's intact. */
	}
	
	L->ptail->pl = (LDATA *)data;
	L->nelem++;
	assert(L != L->p->pl);
	return L->ptail;
};
struct node *mlist_add_first(const handle_t handle, const LDATA *data) {
	assert(mlistmod_data.isinit);
	struct listheader *L=(struct listheader *)handle;
	assert(!TBD_UNFINISHED);
};

struct node *mlist_del(const handle_t handle) {
	assert(mlistmod_data.isinit);
	struct listheader *L=(struct listheader *)handle;
	assert(!TBD_UNFINISHED);
};
struct node *mlist_del_last(const handle_t handle) {
	assert(mlistmod_data.isinit);
	struct listheader *L=(struct listheader *)handle;
	assert(!TBD_UNFINISHED);
};
struct node *mlist_del_first(const handle_t handle) {
	assert(mlistmod_data.isinit);
	struct listheader *L=(struct listheader *)handle;
	assert(!TBD_UNFINISHED);
};

struct node *mlist_dsrct(const handle_t handle) {
	assert(mlistmod_data.isinit);
	struct listheader *L=(struct listheader *)handle;
	assert(!TBD_UNFINISHED);
};
struct node *mlist_dsrct_last(const handle_t handle) {
	assert(mlistmod_data.isinit);
	struct listheader *L=(struct listheader *)handle;
	assert(!TBD_UNFINISHED);
};
struct node *mlist_dsrct_first(const handle_t handle) {
	assert(mlistmod_data.isinit);
	struct listheader *L=(struct listheader *)handle;
	assert(!TBD_UNFINISHED);
};

struct node *mlist_lseek(const handle_t handle, off_t offset, int whence) {
	assert(mlistmod_data.isinit);
	struct listheader *L=(struct listheader *)handle;
	assert(!TBD_UNFINISHED);
};

struct node *mlist_search(const handle_t handle, const LDATA *data) {
	assert(mlistmod_data.isinit);
	struct listheader *L=(struct listheader *)handle;
	assert(!TBD_UNFINISHED);
};
 
