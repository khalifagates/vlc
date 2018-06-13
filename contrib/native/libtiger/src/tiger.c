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

#include "tiger/tiger.h"
#include "tiger_mutex.h"

#define MKVER1(x) #x
#define MKVER2(x,y) MKVER1(x) "." MKVER1(y)
#define MKVER3(x,y,z) MKVER1(x)"."MKVER1(y)"."MKVER1(z)

/**
  \ingroup version
  Returns the version number of this version of libtiger, in 8.8.8 major.minor.patch format
  \returns the version number of libtiger
  */
int tiger_get_version(void)
{
  return (TIGER_VERSION_MAJOR<<16)|(TIGER_VERSION_MINOR<<8)|TIGER_VERSION_PATCH;
}

/**
  \ingroup version
  Returns a human readable string representing the version number of this version of libtiger
  \returns the version number of libtiger as a string
  */
const char *tiger_get_version_string(void)
{
  return "libtiger " MKVER3(TIGER_VERSION_MAJOR,TIGER_VERSION_MINOR,TIGER_VERSION_PATCH);
}

