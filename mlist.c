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

#include <stdio.h>
#include <assert.h>
#include "mlist.h"

struct modstruct {
	int isinit;
	int hndlpool;
	struct node *mlists;
};

static struct modstruct module = {
	.isinit = 1,
	.hndlpool = 0,
	.mlists = NULL
};


int create_mlist(int sz) {
	assert(module.isinit);
};

int delete_mlist(int handle) {
	assert(module.isinit);
};

struct node *mlist_next(int handle) {
	assert(module.isinit);
};
struct node *mlist_prev(int handle) {
	assert(module.isinit);
};

struct node *mlist_head(int handle) {
	assert(module.isinit);
};
struct node *mlist_tail(int handle) {
	assert(module.isinit);
};

struct node *mlist_new(int handle) {
	assert(module.isinit);
};
struct node *mlist_new_last(int handle) {
	assert(module.isinit);
};
struct node *mlist_new_first(int handle) {
	assert(module.isinit);
};

struct node *mlist_add(int handle) {
	assert(module.isinit);
};
struct node *mlist_add_last(int handle) {
	assert(module.isinit);
};
struct node *mlist_add_first(int handle) {
	assert(module.isinit);
};

struct node *mlist_del(int handle) {
	assert(module.isinit);
};
struct node *mlist_del_last(int handle) {
	assert(module.isinit);
};
struct node *mlist_del_first(int handle) {
	assert(module.isinit);
};

struct node *mlist_dsrct(int handle) {
	assert(module.isinit);
};
struct node *mlist_dsrct_last(int handle) {
	assert(module.isinit);
};
struct node *mlist_dsrct_first(int handle) {
	assert(module.isinit);
};

static void mupp2() {
		fprintf(stderr,"<----------Hoppla\n");
}

static int Elf_Init2(void)
{
  	__asm__ (".section .init \n call Elf_Init2 \n .section .text\n");

	mupp2();

	return 1;
}


/* Module initializers */
void __init mlist_init(void) {
#ifndef NDEBUG	
	printf("==========_init==========\n");
#endif
	//module.isinit=1;
}
void __init mlist_init2(void) {
#ifndef NDEBUG	
	printf("==========_init2==========\n");
#endif
	//module.isinit=1;
}

void __fini mlist_fini(void) {
#ifndef NDEBUG	
	printf("==========_fini==========\n");
#endif
	/* Destroy all lists if not already done. Note: will not take care of
	 * lists containing allocated payloads. This is not a garbage collector
	 * */
	module.isinit=0;
}
