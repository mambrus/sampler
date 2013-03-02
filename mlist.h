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

#ifndef mlist_h
#define mlist_h

/* This a linked list library which is C++ inspired. It is meant to be
 * fairly simple and fast, therefore it's (currently) not thread-safe. Nodes
 * are accessed directly as is without regards to any concurrent thread
 * possibly destroying it */

#ifndef LDATA
/* Optional. Set this if prior including to get GDB show more info. */
#define LDATA void
#endif

#ifndef NULL
#define NULL ((void*)0)
#endif

struct node {
	struct node* prev;
	struct node* next;
	LDATA *payload;
};

/* Create a new mlist with payload size sz. Returns handle */
int create_mlist(int sz);

/* Deletes mlist */
int delete_mlist(int handle);

/* Simple iterators */
struct node *mlist_next(int handle);
struct node *mlist_prev(int handle);

/* Go-to's */
struct node *mlist_head(int handle);
struct node *mlist_tail(int handle);

/* New node creation:
 * Note: Both new node and payload will be allocated on heap,
 * payload will *not* be initialized (this is not C++) */
struct node *mlist_new(int handle);
struct node *mlist_new_last(int handle);
struct node *mlist_new_first(int handle);

/* Node insert:
 * Note: Node will be allocated on heap and inserted in list at iterator
 * position, or at position indicated by name */
struct node *mlist_add(int handle);
struct node *mlist_add_last(int handle);
struct node *mlist_add_first(int handle);


/* Delete a node. Deletes a node at iterator position. Assumes payload is
 * already empty. Iterator position is shifted to node just before in list
 * */
struct node *mlist_del(int handle);
struct node *mlist_del_last(int handle);
struct node *mlist_del_first(int handle);

/* Destruct a node. As delete API, but also frees payload. Note, any
 * sub-elements in payload has to be destroyed separately first (this is not
 * C++) */
struct node *mlist_dsrct(int handle);
struct node *mlist_dsrct_last(int handle);
struct node *mlist_dsrct_first(int handle);

#endif /* list_h */

