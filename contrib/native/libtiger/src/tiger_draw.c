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

static int tiger_draw_find_motion(const kate_event *ev,kate_motion_semantics semantics,size_t *motion_index)
{
  size_t n;

  if (!ev) return TIGER_E_INVALID_PARAMETER;

  for (n=0;n<ev->nmotions;++n) {
    const kate_motion *km=ev->motions[n];
    if (km->semantics==semantics) {
      if (motion_index) *motion_index=n;
      return 0;
    }
  }

  return TIGER_E_NOT_FOUND;
}

static void tiger_draw_setup_stroke(const tiger_draw *td,cairo_t *cr,kate_float duration,kate_float t)
{
  int ret;
  kate_float r,g,b,a;
  tiger_color color;
  kate_float draw_width;
  kate_float dummy;

  color=td->base_draw_color;

  if (td->draw_color_rg_motion) {
    ret=kate_tracker_update_property_at_duration(td->kin,duration,t,kate_motion_semantics_draw_color_rg,&r,&g);
    if (ret==0) {
      color.r=r/255.0;
      color.g=g/255.0;
    }
  }
  if (td->draw_color_ba_motion) {
    ret=kate_tracker_update_property_at_duration(td->kin,duration,t,kate_motion_semantics_draw_color_ba,&b,&a);
    if (ret==0) {
      color.b=b/255.0;
      color.a=a/255.0;
    }
  }

  tiger_pixel_format_set_source_color(cr,td->swap_rgb,&color);

  draw_width=(kate_float)1;
  if (td->draw_width_motion) {
    kate_tracker_update_property_at_duration(td->kin,duration,t,kate_motion_semantics_draw_width,&draw_width,&dummy);
  }

  cairo_set_line_width(cr,draw_width);
}

static int tiger_draw_get_point(const tiger_draw *td,const kate_motion *km,const kate_curve *kc,size_t n,kate_float *x,kate_float *y)
{
  *x=kc->pts[n*2];
  *y=kc->pts[n*2+1];
  return kate_tracker_remap(td->kin,km->x_mapping,km->y_mapping,x,y);
}

static kate_float tiger_draw_get_sampling_step(const kate_curve *kc,cairo_t *cr)
{
  static const struct { kate_float multiplier; int use_tolerance; } settings[]={
    {1,0}, /* none */
    {1,0}, /* static */
    {2,0}, /* linear */
    {8,1}, /* catmull rom */
    {8,1}, /* cubic bezier */
    {8,1}, /* B spline */
  };
  double tolerance=cairo_get_tolerance(cr);
  kate_float npts=(kate_float)1;
  if ((size_t)kc->type<sizeof(settings)/sizeof(settings[0])) {
    npts*=settings[kc->type].multiplier;
    if (settings[kc->type].use_tolerance) {
      double clamped_tolerance=tolerance>10?10:tolerance<0?0:tolerance;
      npts*=1+(10-clamped_tolerance*10);
    }
  }
  return 1/npts;
}

static int tiger_draw_draw_curve_sample(const tiger_draw *td,const kate_motion *km,size_t curve,kate_float target,cairo_t *cr)
{
  const kate_curve *kc=km->curves[curve];
  kate_float t,dt;
  kate_float x,y;
  int ret;
  int first=1;
  kate_float event_duration=td->kin->event->end_time-td->kin->event->start_time;

  dt=tiger_draw_get_sampling_step(kc,cr);
  for (t=(kate_float)0;t<=target;t+=dt) {
    ret=kate_curve_get_point(kc,t,&x,&y);
    if (ret<0) return ret;
    if (ret==0) {
      ret=kate_tracker_remap(td->kin,km->x_mapping,km->y_mapping,&x,&y);
      if (ret<0) return ret;
      if (first) {
        cairo_move_to(cr,x,y);
        first=0;
      }
      cairo_line_to(cr,x,y);
      if (td->needs_stepped_strokes) {
        tiger_draw_setup_stroke(td,cr,event_duration,td->curve_start_t+t*td->curve_duration);
        cairo_stroke(cr);
        cairo_move_to(cr,x,y);
      }
    }
  }

  return 0;
}

static int tiger_draw_draw_curve_static(const tiger_draw *td,const kate_motion *km,size_t curve,cairo_t *cr)
{
  const kate_curve *kc=km->curves[curve];
  kate_float x,y;
  int ret;

  ret=tiger_draw_get_point(td,km,kc,0,&x,&y);
  if (ret<0) return ret;
  cairo_move_to(cr,x,y);
  cairo_line_to(cr,x,y);
  return 0;
}

static int tiger_draw_draw_curve_linear(const tiger_draw *td,const kate_motion *km,size_t curve,cairo_t *cr)
{
  const kate_curve *kc=km->curves[curve];
  kate_float x,y;
  size_t n;
  int ret;

  for (n=0;n<kc->npts;++n) {
    ret=tiger_draw_get_point(td,km,kc,n,&x,&y);
    if (ret<0) return ret;
    if (n==0) cairo_move_to(cr,x,y);
    cairo_line_to(cr,x,y);
  }
  return 0;
}

static int tiger_draw_draw_curve_cubic_bezier(const tiger_draw *td,const kate_motion *km,size_t curve,cairo_t *cr)
{
  const kate_curve *kc=km->curves[curve];
  kate_float x1,y1,x2,y2,x3,y3;
  size_t n;
  int ret;

  if (kc->npts>0) {
    ret=tiger_draw_get_point(td,km,kc,0,&x1,&y1);
    if (ret<0) return ret;
    cairo_move_to(cr,x1,y1);
  }
  for (n=1;n+2<kc->npts;n+=3) {
    ret=tiger_draw_get_point(td,km,kc,n,&x1,&y1);
    if (ret<0) return ret;
    ret=tiger_draw_get_point(td,km,kc,n+1,&x2,&y2);
    if (ret<0) return ret;
    ret=tiger_draw_get_point(td,km,kc,n+2,&x3,&y3);
    if (ret<0) return ret;
    cairo_curve_to(cr,x1,y1,x2,y2,x3,y3);
  }
  return 0;
}

static int tiger_draw_draw_curve(const tiger_draw *td,const kate_motion *km,size_t curve,cairo_t *cr)
{
  const kate_curve *kc=km->curves[curve];
  static int (* const drawers[])(const tiger_draw*,const kate_motion*,size_t,cairo_t*)={
    NULL, /* none */
    &tiger_draw_draw_curve_static,
    &tiger_draw_draw_curve_linear,
    NULL, /* catmull rom */
    &tiger_draw_draw_curve_cubic_bezier,
    NULL, /* B spline */
  };
  if (!td->needs_stepped_strokes) {
    if (((size_t)kc->type)<sizeof(drawers)/sizeof(drawers[0])) {
      if (drawers[kc->type]) return (*drawers[kc->type])(td,km,curve,cr);
    }
  }
  /* not known, doesn't map well to cairo, or using draw color etc - let Kate handle it via sampling */
  return tiger_draw_draw_curve_sample(td,km,curve,(kate_float)1,cr);
}

int tiger_draw_render(tiger_draw *td,cairo_t *cr,kate_float t,int swap_rgb)
{
  const kate_tracker *kin;
  const kate_event *ev;
  const kate_motion *km;
  kate_float event_duration;
  size_t n;
  int ret;

  if (!td || !cr) return TIGER_E_INVALID_PARAMETER;

  /* most events won't have a draw motion */
  if (!td->draw_motion) return 0;

  kin=td->kin;
  ev=kin->event;
  km=td->draw_motion;

  /* we now have a draw motion */
  event_duration=ev->end_time-ev->start_time;
  td->curve_start_t=(kate_float)0;
  td->swap_rgb=swap_rgb;

  cairo_save(cr);
  tiger_pixel_format_set_source_color(cr,swap_rgb,&td->base_draw_color);
  cairo_set_line_width(cr,1.0);

  /* find out all the curves that are fully done at the given time, and draw them without sampling */
  for (n=0;n<km->ncurves;++n) {
    /* find out the duration of that curve */
    td->curve_duration=km->durations[n];
    /* negative durations are relative to the event's duration */
    if (td->curve_duration<0) td->curve_duration=-td->curve_duration*event_duration;
    if (td->curve_start_t+td->curve_duration>t) {
      /* the curve spans further than our target, render it partway with sampling */
      kate_float ratio=(t-td->curve_start_t)/td->curve_duration;
      ret=tiger_draw_draw_curve_sample(td,km,n,ratio,cr);
      if (ret<0) return ret;
      break;
    }
    /* draw the full curve */
    ret=tiger_draw_draw_curve(td,km,n,cr);
    if (ret<0) return ret;
    td->curve_start_t+=td->curve_duration;
  }

  /* if we have a draw color, the path will have been stroked realtime to track
     changes in color, so only do it if we don't */
  if (!td->needs_stepped_strokes) {
    cairo_stroke(cr);
  }

  cairo_restore(cr);

  return 0;
}

int tiger_draw_init(tiger_draw *td,const kate_tracker *kin)
{
  const kate_style *ks;
  size_t motion_index;
  int ret;

  if (!td || !kin) return TIGER_E_INVALID_PARAMETER;

  td->kin=kin;
  td->draw_motion=NULL;
  td->draw_color_rg_motion=NULL;
  td->draw_color_ba_motion=NULL;
  td->draw_width_motion=NULL;
  td->needs_stepped_strokes=0;
  td->swap_rgb=0;

  /* look for a motion with draw semantics */
  ret=tiger_draw_find_motion(kin->event,kate_motion_semantics_draw,&motion_index);
  if (ret==TIGER_E_NOT_FOUND) return 0;
  if (ret<0) return ret;

  /* we have found one */
  td->draw_motion=kin->event->motions[motion_index];

  /* look for draw color motions, since we have a draw motion */
  ret=tiger_draw_find_motion(kin->event,kate_motion_semantics_draw_color_rg,&motion_index);
  if (ret!=TIGER_E_NOT_FOUND) {
    if (ret<0) return ret;
    td->draw_color_rg_motion=kin->event->motions[motion_index];
    td->needs_stepped_strokes=1;
  }
  ret=tiger_draw_find_motion(kin->event,kate_motion_semantics_draw_color_ba,&motion_index);
  if (ret!=TIGER_E_NOT_FOUND) {
    if (ret<0) return ret;
    td->draw_color_ba_motion=kin->event->motions[motion_index];
    td->needs_stepped_strokes=1;
  }

  /* look for draw width motions, since we have a draw motion */
  ret=tiger_draw_find_motion(kin->event,kate_motion_semantics_draw_width,&motion_index);
  if (ret!=TIGER_E_NOT_FOUND) {
    if (ret<0) return ret;
    td->draw_width_motion=kin->event->motions[motion_index];
    td->needs_stepped_strokes=1;
  }

  /* if no motion, or a gap in a motion, we'll default on the style's draw color, if any */
  ks=td->kin->event->style;
  if (!ks) {
    const kate_region *kr=td->kin->event->region;
    if (kr && kr->style>=0) ks=td->kin->ki->styles[kr->style]; /* regions can have a default style */
  }
  if (ks) {
    tiger_util_kate_color_to_tiger_color(&td->base_draw_color,&ks->draw_color);
  }
  else {
    td->base_draw_color.r=1.0;
    td->base_draw_color.g=1.0;
    td->base_draw_color.b=1.0;
    td->base_draw_color.a=1.0;
  }

  return 0;
}

int tiger_draw_clear(tiger_draw *td)
{
  if (!td) return TIGER_E_INVALID_PARAMETER;
  return 0;
}

