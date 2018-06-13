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

#include <math.h>
#include "tiger/tiger.h"

void tiger_rectangle_empty(tiger_rectangle *r)
{
  r->x0=DBL_MAX;
  r->y0=DBL_MAX;
  r->x1=-DBL_MAX;
  r->y1=-DBL_MAX;
}

void tiger_rectangle_snap(const tiger_rectangle *r,tiger_rectangle *snapped)
{
  snapped->x0=floor(r->x0);
  snapped->y0=floor(r->y0);
  snapped->x1=ceil(r->x1);
  snapped->y1=ceil(r->y1);
}

void tiger_rectangle_extend(tiger_rectangle *r,const tiger_rectangle *extend)
{
  if (extend->x0<r->x0) r->x0=extend->x0;
  if (extend->y0<r->y0) r->y0=extend->y0;
  if (extend->x1>r->x1) r->x1=extend->x1;
  if (extend->y1>r->y1) r->y1=extend->y1;
}

void tiger_rectangle_grow(tiger_rectangle *r,double x,double y)
{
  /* if growing by negative amounts (shrinking), ensure we can't shrink to less than empty */
  if (x<0.0) {
    if (-x>r->x1-r->x0) x=-(r->x1-r->x0)/2.0;
  }
  r->x0-=x;
  r->x1+=x;

  if (y<0.0) {
    if (-y>r->y1-r->y0) y=-(r->y1-r->y0)/2.0;
  }
  r->y0-=y;
  r->y1+=y;
}

void tiger_rectangle_clip(tiger_rectangle *r,const tiger_rectangle *clip)
{
  if (clip->x0>r->x0) r->x0=clip->x0;
  if (clip->y0>r->y0) r->y0=clip->y0;
  if (clip->x1<r->x1) r->x1=clip->x1;
  if (clip->y1<r->y1) r->y1=clip->y1;
}

void tiger_rectangle_order(const tiger_rectangle *r,tiger_rectangle *ordered)
{
  if (r->x0<r->x1) {
    ordered->x0=r->x0;
    ordered->x1=r->x1;
  }
  else {
    double tmp=ordered->x0; /* in case r and ordered are the same */
    ordered->x0=r->x1;
    ordered->x1=tmp;
  }
  if (r->y0<r->y1) {
    ordered->y0=r->y0;
    ordered->y1=r->y1;
  }
  else {
    double tmp=ordered->y0; /* in case r and ordered are the same */
    ordered->y0=r->y1;
    ordered->y1=tmp;
  }
}

