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


#define TIGER_INTERNAL
#include "tiger_internal.h"

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#include <string.h>

#include "tiger/tiger.h"

typedef struct tiger_bitmap_cache_prime_item {
  const kate_bitmap *kb;
  const kate_palette *kp;
} tiger_bitmap_cache_prime_item;

typedef struct tiger_bitmap_cache_prime_queue {
  size_t nitems;
  tiger_bitmap_cache_prime_item *items;
} tiger_bitmap_cache_prime_queue;

struct tiger_bitmap_cache_primer {
  int stop;
  tiger_mutex_t mutex;
  tiger_thread_t thread;
  tiger_bitmap_cache_prime_queue queue;
};

static int tiger_bitmap_cache_expand(tiger_bitmap_cache *tbc)
{
  size_t size,n;
  tiger_bitmap **tb;

  if (!tbc) return TIGER_E_INVALID_PARAMETER;

  size=tbc->size?tbc->size*2:8;
  tb=(tiger_bitmap**)tiger_realloc(tbc->tb,size*sizeof(tiger_bitmap*));
  if (!tb) return TIGER_E_OUT_OF_MEMORY;

  for (n=tbc->size;n<size;++n) {
    tb[n]=NULL;
  }

  tbc->size=size;
  tbc->tb=tb;

  return 0;
}

static int tiger_bitmap_cache_get_unlocked(tiger_bitmap_cache *tbc,const kate_bitmap *kb,const kate_palette *kp,tiger_bitmap **tb)
{
  size_t n;
  tiger_bitmap **available=NULL;
  int ret;

  if (!tbc || !kb || !tb) return TIGER_E_INVALID_PARAMETER;
  /* kp may be NULL */

  for (n=0;n<tbc->size;++n) {
    if (tbc->tb[n]) {
      if (tiger_bitmap_matches(tbc->tb[n],kb,kp)) {
        *tb=tbc->tb[n];
        return 0;
      }
    }
    else {
      if (!available) available=&tbc->tb[n];
    }
  }

  if (!available) {
    size_t last=tbc->size;
    ret=tiger_bitmap_cache_expand(tbc);
    if (ret<0) return ret;
    available=&tbc->tb[last];
  }

  ret=tiger_bitmap_create(available,kb,kp,tbc->swap_rgb);
  if (ret<0) return ret;

  *tb=*available;
  return 0;
}

int tiger_bitmap_cache_get(tiger_bitmap_cache *tbc,const kate_bitmap *kb,const kate_palette *kp,tiger_bitmap **tb)
{
  int ret;
  if (tbc->primer) tiger_mutex_lock(&tbc->primer->mutex);
  ret=tiger_bitmap_cache_get_unlocked(tbc,kb,kp,tb);
  if (tbc->primer) tiger_mutex_unlock(&tbc->primer->mutex);
  return ret;
}

#ifdef HAVE_PTHREAD
static void *tiger_bitmap_cache_thread(void *arg)
{
  tiger_bitmap_cache *tbc=(tiger_bitmap_cache*)arg;
  tiger_bitmap_cache_primer *primer=tbc->primer;
  while (!primer->stop) {
    if (!tiger_mutex_trylock(&primer->mutex)) {
      if (primer->queue.nitems>0) {
        const kate_bitmap *kb=primer->queue.items[0].kb;
        const kate_palette *kp=primer->queue.items[0].kp;
        tiger_bitmap *tb;
        tiger_bitmap_cache_get_unlocked(tbc,kb,kp,&tb);
        --primer->queue.nitems;
        memmove(primer->queue.items,primer->queue.items+1,primer->queue.nitems*sizeof(tiger_bitmap_cache_prime_item));
      }
      tiger_mutex_unlock(&primer->mutex);
    }
    tiger_util_usleep(1000);
  }
  return NULL;
}
#endif

int tiger_bitmap_cache_init(tiger_bitmap_cache *tbc,int swap_rgb)
{
  int ret;

  if (!tbc) return TIGER_E_INVALID_PARAMETER;

  tbc->tb=NULL;
  tbc->size=0;
  tbc->swap_rgb=swap_rgb;
  tbc->primer=NULL;

#ifdef HAVE_PTHREAD
  tbc->primer=(tiger_bitmap_cache_primer*)tiger_malloc(sizeof(tiger_bitmap_cache_primer));
  if (!tbc->primer) return TIGER_E_OUT_OF_MEMORY;

  ret=tiger_mutex_init(&tbc->primer->mutex);
  if (ret==0) {
    tbc->primer->stop=0; /* needs to be set before the thread starts */
    tbc->primer->queue.nitems=0;
    tbc->primer->queue.items=NULL;
    /* the thread will start running, everything must be initialized by now, and locks are needed from now on */
    ret=tiger_thread_create(&tbc->primer->thread,&tiger_bitmap_cache_thread,tbc);
    if (ret<0) {
      tiger_mutex_destroy(&tbc->primer->mutex);
      tiger_free(tbc->primer);
      tbc->primer=NULL;
    }
  }
  else {
    tiger_free(tbc->primer);
    tbc->primer=NULL;
  }
#endif

  return 0;
}

int tiger_bitmap_cache_clear(tiger_bitmap_cache *tbc)
{
  size_t n;
  int ret;

  if (!tbc) return TIGER_E_INVALID_PARAMETER;

#ifdef HAVE_PTHREAD
  if (tbc->primer) {
    tiger_mutex_lock(&tbc->primer->mutex);
    tbc->primer->stop=1;
    ret=tiger_thread_join(tbc->primer->thread);
    if (ret<0) {
      tiger_log(tiger_log_warning,"failed to join thread: %d\n",ret);
    }
    tiger_mutex_unlock(&tbc->primer->mutex);
    tiger_mutex_destroy(&tbc->primer->mutex);
    if (tbc->primer->queue.items) tiger_free(tbc->primer->queue.items);
    tiger_free(tbc->primer);
  }
#endif

  if (tbc->tb) {
    for (n=0;n<tbc->size;++n) {
      if (tbc->tb[n]) {
        tiger_bitmap_destroy(tbc->tb[n]);
      }
    }
    tiger_free(tbc->tb);
  }

  return 0;
}

void tiger_bitmap_cache_prime(tiger_bitmap_cache *tbc,const kate_bitmap *kb,const kate_palette *kp)
{
  tiger_bitmap_cache_prime_item *items;
  size_t nitems;

  if (tbc->primer) {
    tiger_mutex_lock(&tbc->primer->mutex);
    nitems=tbc->primer->queue.nitems+1;
    items=(tiger_bitmap_cache_prime_item*)tiger_realloc(tbc->primer->queue.items,nitems*sizeof(tiger_bitmap_cache_prime_item));
    if (items) {
      items[nitems-1].kb=kb;
      items[nitems-1].kp=kp;
      tbc->primer->queue.items=items;
      tbc->primer->queue.nitems=nitems;
    }
    tiger_mutex_unlock(&tbc->primer->mutex);
  }
}

