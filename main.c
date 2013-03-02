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
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#undef  NDEBUG
#include <assert.h>

#include "sampler.h"
#include "mlist.h"

/* TinKer version (generic) */
typedef void* __drv_finit_f(void*);
typedef __drv_finit_f *drv_finit_t;
#define DRV_IO_NAME( x, y ) x ##y
#define DRV_IO( x ) DRV_IO_NAME( mymodule_ , x )

void *DRV_IO(test)(void* inarg) {
		printf("<< Hello >>\n");
}
drv_finit_t DRV_IO(myfunkptr) __attribute__ ((section (".mupp"))) =DRV_IO(test);


/* Simplified version */

#define SECTION( S ) __attribute__ ((section ( S )))
void test(void) {
		printf("<< Hello2 >>\n");
		DRV_IO(test)(NULL);
}
void (*funcptr)(void) SECTION(".dtors") =test;


void __fini tost() {
		fprintf(stderr,"Hoppla----------->\n");
}

void mupp() {
		fprintf(stderr,"<----------Hoppla\n");
}

int Elf_Init(void)
{
  	__asm__ (".section .init \n call Elf_Init \n .section .text\n");

	mupp();

	return 1;
}

int main(int argc, char **argv) {
	if (argc != 2) {
		fprintf(stderr,"Need exactly one argument [%d]: full path of signals description file\n",argc);
		exit(1);
	}

	sampler_init(argv[1]);
	//sampler_init(argv[1], atoi(argv[2]));
	//
	delete_mlist(1);
	return 0;
}

