/* Copyright (C) 2008 Vincent Penquerc'h.
   This file is part of the Tiger rendering library.
   Written by Vincent Penquerc'h.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.
  
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
   Lesser General Public License for more details.
  
   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the
   Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA. */


#ifndef TIGER_tiger_mutex_h_GUARD
#define TIGER_tiger_mutex_h_GUARD

#include "tiger/tiger.h"

#ifdef HAVE_PTHREAD

#include <pthread.h>

typedef pthread_mutex_t tiger_mutex_t;
#define TIGER_MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER
#define tiger_mutex_init(mutex) pthread_mutex_init(mutex,NULL)
#define tiger_mutex_destroy pthread_mutex_destroy
#define tiger_mutex_lock pthread_mutex_lock
#define tiger_mutex_trylock pthread_mutex_trylock
#define tiger_mutex_unlock pthread_mutex_unlock

#else

/* unsafe stubs, not built by default */
typedef int tiger_mutex_t;
#define TIGER_MUTEX_INITIALIZER 0
static inline int tiger_mutex_init(tiger_mutex_t *lock) { (void)lock; return 0; }
static inline int tiger_mutex_destroy(tiger_mutex_t *lock) { (void)lock; return 0; }
static inline int tiger_mutex_lock(tiger_mutex_t *lock) { (void)lock; return 0; }
static inline int tiger_mutex_unlock(tiger_mutex_t *lock) { (void)lock; return 0; }
static inline int tiger_mutex_trylock(tiger_mutex_t *lock) { (void)lock; return 0; }
#endif

#endif

