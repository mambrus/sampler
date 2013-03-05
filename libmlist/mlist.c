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
	struct node *p=NULL;
	assert(mlistmod_data.isinit);

	/*Create a list head in the empty payload*/
	mlistmod_data.mlists->pl = malloc(sizeof(struct listhead));
	assert(mlistmod_data.mlists->pl);
	memset(mlistmod_data.mlists->pl, 0, sizeof(struct listhead));
	/*Create a new empty node in front*/
	mlistmod_data.mlists->prev = malloc(sizeof(struct node));
	assert(mlistmod_data.mlists->prev);
	memset(mlistmod_data.mlists->prev, 0, sizeof(struct node));
	mlistmod_data.mlists->prev->next = mlistmod_data.mlists;
	/*Cotinue initialize our nodes payload*/
	mlistmod_data.mlists->pl->cmpfunc = cmpfunc;
	/* Don't pre-create 1:st node. They are supposed to get sorted on creation
	 * and we don't have that data yet */
	mlistmod_data.mlists->pl->pstart = NULL;
	*hndl=(handle_t)mlistmod_data.mlists;
	mlistmod_data.nlists++;
	/* Prepare for next creation */
	mlistmod_data.mlists=mlistmod_data.mlists->prev;
	return 0;
};

int delete_mlist(handle_t handle) {
	assert(mlistmod_data.isinit);
};
int dstrct_mlist(handle_t handle) {
	assert(mlistmod_data.isinit);
};

struct node *mlist_next(handle_t handle) {
	assert(mlistmod_data.isinit);
};
struct node *mlist_prev(handle_t handle) {
	assert(mlistmod_data.isinit);
};

struct node *mlist_head(handle_t handle) {
	assert(mlistmod_data.isinit);
	return (*(struct listhead*)(handle)).pstart;
};
struct node *mlist_tail(handle_t handle) {
	assert(mlistmod_data.isinit);
	return (*(struct listhead*)(handle)).pend;
};

struct node *mlist_add(handle_t handle, const LDATA *data) {
	assert(mlistmod_data.isinit);
};
struct node *mlist_add_last(handle_t handle, const LDATA *data) {
	assert(mlistmod_data.isinit);
};
struct node *mlist_add_first(handle_t handle, const LDATA *data) {
	assert(mlistmod_data.isinit);
};

struct node *mlist_del(handle_t handle) {
	assert(mlistmod_data.isinit);
};
struct node *mlist_del_last(handle_t handle) {
	assert(mlistmod_data.isinit);
};
struct node *mlist_del_first(handle_t handle) {
	assert(mlistmod_data.isinit);
};

struct node *mlist_dsrct(handle_t handle) {
	assert(mlistmod_data.isinit);
};
struct node *mlist_dsrct_last(handle_t handle) {
	assert(mlistmod_data.isinit);
};
struct node *mlist_dsrct_first(handle_t handle) {
	assert(mlistmod_data.isinit);
};

struct node *mlist_lseek(handle_t handle, off_t offset, int whence) {
	assert(mlistmod_data.isinit);
};

struct node *mlist_search(const LDATA *data) {
	assert(mlistmod_data.isinit);
};

