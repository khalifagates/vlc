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

#include <errno.h>
#include <time.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#include "tiger/tiger.h"

int tiger_util_remap_metric_double(double v0,kate_space_metric metric,double unit,double *v1)
{
  int ret=0;
  switch (metric) {
    case kate_pixel: *v1=v0; break;
    case kate_percentage: *v1=(v0*unit)/100.0; break;
    case kate_millionths: *v1=(v0*unit)/1000000.0; break;
    default: tiger_log(tiger_log_warning,"invalid metric: %d\n",metric); ret=TIGER_E_INVALID_PARAMETER; *v1=v0; break;
  }
  return ret;
}

int tiger_util_remap_metric_int(int v0,kate_space_metric metric,int unit,int *v1)
{
  int ret=0;
  switch (metric) {
    case kate_pixel: *v1=v0; break;
    case kate_percentage: *v1=(v0*unit+50)/100; break;
    case kate_millionths: *v1=(v0*unit+500000)/1000000; break;
    default: tiger_log(tiger_log_warning,"invalid metric: %d\n",metric); ret=TIGER_E_INVALID_PARAMETER; *v1=v0; break;
  }
  return ret;
}

void tiger_util_kate_color_to_tiger_color(tiger_color *tc,const kate_color *kc)
{
  /* TODO: 256 floats LUT would be faster ? */
  tc->r=kc->r/255.0;
  tc->g=kc->g/255.0;
  tc->b=kc->b/255.0;
  tc->a=kc->a/255.0;
}

void tiger_util_usleep(unsigned int microseconds)
{
#if defined HAVE_NANOSLEEP
  struct timespec ts;
  ts.tv_sec=0;
  ts.tv_nsec=microseconds*1000;
  do errno=0; while (nanosleep(&ts,&ts)<0 && errno==EINTR);
#elif defined HAVE_SELECT
  struct timeval tv;
  tv.tv_sec=0;
  tv.tv_usec=microseconds;
  select(0,NULL,NULL,NULL,&tv);
#elif defined HAVE_USLEEP
  usleep(microseconds);
#else
#warning No sleep routine found.
#endif
}

