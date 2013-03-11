/***************************************************************************
 *   Copyright (C) 2013 by Michael Ambrus                                  *
 *   ambrmi09@gmail.com                                             *
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

#if !defined( SEMAPHORE_H )
#define SEMAPHORE_H

#include <tk_ansi_dirwrap.h>
#include BUILDCHAIN(semaphore.h)

#if defined(__cplusplus) 
extern "C" {
#endif

int X_sem_init (sem_t * sem, int pshared, unsigned int value );
int X_sem_destroy (sem_t * sem );
int X_sem_trywait (sem_t * sem );
int X_sem_wait (sem_t * sem );
int X_sem_post (sem_t * sem);

#define sem_init        X_sem_init
#define sem_destroy     X_sem_destroy
#define sem_trywait     X_sem_trywait
#define sem_wait        X_sem_wait
#define sem_post		X_sem_post

#if defined(__cplusplus)
}
#endif

#endif
