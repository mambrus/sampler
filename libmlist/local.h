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

#ifndef local_h
#define local_h

#define LDATA struct listhead
#include <mlist.h>

#define __init __attribute__((constructor))
#define __fini __attribute__((destructor))

#define TBD_UNFINISHED "Code not finished!!! (TBD)"

/* Administrative keeper of all lists */
struct mlistmod_struct {
	int isinit;           /* Is this module initialized? I.e. does mlist
							 below contain a valid pointer e.t.a.*/
	int nlists;           /* Current number of lists in  list */
	struct node *mlists;  /* List-head of lists. No need to sort to find.
							 Handle is hash-key*/
};

/* Indicate to others that someone is keeping this as global module data
 * (easier to find with GDB) */
extern struct mlistmod_struct mlistmod_data;

/* Data of this struct is the payload for the mlist variable in mlistmod_struct.
 * It's the administrative keeper of each list. */
struct listhead {
	struct node *p;	      /* Current (file) pointer */
	off_t o;              /* Offset from start (in jumps jumps) */
	int iindx;            /* Iterator index. File-pointer so to speak */
	int nelem;            /* Current size of this list */
	int pl_sz;            /* pay-load size */

	/* Caller provided function used to search & sort list. Can be NULL if
	 * search and sort is not supported */
	int (*cmpfunc)(LDATA *lval, LDATA *rval);
	struct node *pstart;  /* List-star */
	struct node *plast;    /* List-end */
};

#endif /* local_h */

