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

void tiger_pixel_format_set_source_color(cairo_t *cr,int swap,const tiger_color *tc)
{
  if (swap)
    cairo_set_source_rgba(cr,tc->b,tc->g,tc->r,tc->a);
  else
    cairo_set_source_rgba(cr,tc->r,tc->g,tc->b,tc->a);
}

