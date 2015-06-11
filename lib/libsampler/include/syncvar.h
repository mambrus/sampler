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

#ifndef syncvar_h
#define syncvar_h
#include <pthread.h>

/* RW-lock macros*/
#define BEGIN_RD( L ) pthread_rwlock_rdlock(L); {
#define END_RD( L ) } pthread_rwlock_unlock(L);
#define BEGIN_WR( L ) pthread_rwlock_wrlock(L); {
#define END_WR( L ) } pthread_rwlock_unlock(L);

static inline void sync_set(pthread_rwlock_t *lock, int *var, int val) {
	BEGIN_WR(lock)
		*var = val;
	END_WR(lock)
};

static inline void sync_add(pthread_rwlock_t *lock, int *var, int val) {
	BEGIN_WR(lock)
		*var += val;
	END_WR(lock)
};

static inline void sync_sub(pthread_rwlock_t *lock, int *var, int val) {
	BEGIN_WR(lock)
		*var -= val;
	END_WR(lock)
};

static inline int sync_get(pthread_rwlock_t *lock, int *var) {
	int tmp;

	BEGIN_RD(lock)
		tmp = *var;
	END_RD(lock)
	return tmp;
};

#define sync_inc( L , V ) \
	sync_add(L, V, 1);

#define sync_dec( L , V ) \
	sync_sub(L, V, 1);


#endif /* syncvar_h */
