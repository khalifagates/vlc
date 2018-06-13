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

#include <stdio.h>
#include <stdarg.h>
#ifdef PROFILE
#include <sys/time.h>
#endif
#include "tiger/tiger.h"

#if defined DEBUG || defined PROFILE
void tiger_log(tiger_log_level level,const char *format,...)
{
  va_list ap;
  setvbuf(stderr,NULL,_IONBF,0);
  switch (level) {
#ifdef PROFILE
    case tiger_log_profile: fprintf(stderr,"profile: "); break;
#endif
#ifdef DEBUG
    case tiger_log_error: fprintf(stderr,"error: "); break;
    case tiger_log_warning: fprintf(stderr,"warning: "); break;
    case tiger_log_debug: fprintf(stderr,"debug: "); break;
    case tiger_log_spam: fprintf(stderr,"spam: "); break;
    default: fprintf(stderr," -- UNKNOWN LOG LEVEL -- "); break;
#else
    default: return;
#endif
  }
  va_start(ap,format);
  vfprintf(stderr,format,ap);
  va_end(ap);
}
#endif

#ifdef PROFILE
unsigned long tiger_profile_now(void)
{
  struct timeval tv;
  gettimeofday(&tv,NULL);
  return tv.tv_sec*1000000+tv.tv_usec;
}
#endif

