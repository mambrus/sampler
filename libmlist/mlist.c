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
	struct listhead *L=NULL;
	assert(mlistmod_data.isinit);
	
	if (!mlistmod_data.mlists) {
		/* If list is all empty, create also first empty node  */
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
	}
	
	mlistmod_data.mlists->pl = malloc(sizeof(struct listhead));
	assert(mlistmod_data.mlists->pl);

	/*Cotinue initialize our nodes payload*/
	L=mlistmod_data.mlists->pl;
	memset(L, 0, sizeof(struct listhead));
	L->p       = mlistmod_data.mlists;
	L->pstart  = mlistmod_data.mlists;
	L->plast    = mlistmod_data.mlists;
	L->pl_sz   = sz;
	L->cmpfunc = cmpfunc;
	mlistmod_data.nlists++;
	*hndl=(handle_t)L;
	return 0;
};

int delete_mlist(const handle_t handle) {
	assert(mlistmod_data.isinit);
	struct listhead *L=(struct listhead *)handle;
	assert(!TBD_UNFINISHED);
};
int dstrct_mlist(const handle_t handle) {
	assert(mlistmod_data.isinit);
	struct listhead *L=(struct listhead *)handle;
	assert(!TBD_UNFINISHED);
};

struct node *mlist_next(const handle_t handle) {
	assert(mlistmod_data.isinit);
	struct listhead *L=(struct listhead *)handle;
	assert(!TBD_UNFINISHED);
};
struct node *mlist_prev(const handle_t handle) {
	assert(mlistmod_data.isinit);
	struct listhead *L=(struct listhead *)handle;
	assert(!TBD_UNFINISHED);
};

struct node *mlist_head(const handle_t handle) {
	assert(mlistmod_data.isinit);
	struct listhead *L=(struct listhead *)handle;
	assert(!TBD_UNFINISHED);
	return (*(struct listhead*)(handle)).pstart;
};
struct node *mlist_tail(const handle_t handle) {
	assert(mlistmod_data.isinit);
	struct listhead *L=(struct listhead *)handle;
	assert(!TBD_UNFINISHED);
	return (*(struct listhead*)(handle)).plast;
};

struct node *mlist_add(const handle_t handle, const LDATA *data) {
	assert(mlistmod_data.isinit);
	struct listhead *L=(struct listhead *)handle;
	if (!L->cmpfunc)
		return mlist_add_last(handle, data);

	
	assert(!TBD_UNFINISHED);
};
struct node *mlist_add_last(const handle_t handle, const LDATA *data) {
	assert(mlistmod_data.isinit);
	struct listhead *L=(struct listhead *)handle;
	
	L->plast->next = malloc(sizeof(struct node));
	assert(L->plast->next);
	memset(L->plast->next, 0, sizeof(struct node));
	L->plast->next->prev = L->plast;
	L->plast = L->plast->next->prev;
	L->plast->pl = (LDATA *)data;
	return L->plast;
};
struct node *mlist_add_first(const handle_t handle, const LDATA *data) {
	assert(mlistmod_data.isinit);
	struct listhead *L=(struct listhead *)handle;
	assert(!TBD_UNFINISHED);
};

struct node *mlist_del(const handle_t handle) {
	assert(mlistmod_data.isinit);
	struct listhead *L=(struct listhead *)handle;
	assert(!TBD_UNFINISHED);
};
struct node *mlist_del_last(const handle_t handle) {
	assert(mlistmod_data.isinit);
	struct listhead *L=(struct listhead *)handle;
	assert(!TBD_UNFINISHED);
};
struct node *mlist_del_first(const handle_t handle) {
	assert(mlistmod_data.isinit);
	struct listhead *L=(struct listhead *)handle;
	assert(!TBD_UNFINISHED);
};

struct node *mlist_dsrct(const handle_t handle) {
	assert(mlistmod_data.isinit);
	struct listhead *L=(struct listhead *)handle;
	assert(!TBD_UNFINISHED);
};
struct node *mlist_dsrct_last(const handle_t handle) {
	assert(mlistmod_data.isinit);
	struct listhead *L=(struct listhead *)handle;
	assert(!TBD_UNFINISHED);
};
struct node *mlist_dsrct_first(const handle_t handle) {
	assert(mlistmod_data.isinit);
	struct listhead *L=(struct listhead *)handle;
	assert(!TBD_UNFINISHED);
};

struct node *mlist_lseek(const handle_t handle, off_t offset, int whence) {
	assert(mlistmod_data.isinit);
	struct listhead *L=(struct listhead *)handle;
	assert(!TBD_UNFINISHED);
};

struct node *mlist_search(const handle_t handle, const LDATA *data) {
	assert(mlistmod_data.isinit);
	struct listhead *L=(struct listhead *)handle;
	assert(!TBD_UNFINISHED);
};

