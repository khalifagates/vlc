/* Copyright (C) 2008-2010 Vincent Penquerc'h.
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
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#include <string.h>
#include "tiger/tiger.h"

static void tiger_renderer_propagate_flags(tiger_renderer *tr)
{
  size_t n;

  for (n=0; n<tr->nitems; ++n) {
    tr->items[n].flags=tr->flags;
  }
}

static int tiger_renderer_set_flag(tiger_renderer *tr,kate_uint32_t flag,int on)
{
  if (!tr) return TIGER_E_INVALID_PARAMETER;
  if (on)
    tr->flags|=flag;
  else
    tr->flags&=~flag;
  tiger_renderer_propagate_flags(tr);
  return 0;
}

static int tiger_renderer_invalidate_caches(tiger_renderer *tr)
{
  size_t n;
  int inv_ret;

  if (!tr) return TIGER_E_INVALID_PARAMETER;

  inv_ret=0;
  for (n=0; n<tr->nitems; ++n) {
    int ret=tiger_item_invalidate_cache(&tr->items[n]);
    if (ret<0) inv_ret=ret;
  }

  return inv_ret;
}

static void tiger_renderer_set_color(tiger_color *tc,double r,double g,double b,double a)
{
  tc->r=r<0?0:r>1?1:r;
  tc->g=g<0?0:g>1?1:g;
  tc->b=b<0?0:b>1?1:b;
  tc->a=a<0?0:a>1?1:a;
}

static void tiger_renderer_apply_quality(cairo_t *cr,double quality)
{
  if (quality>=0) {
    cairo_set_antialias(cr,quality<0.1f?CAIRO_ANTIALIAS_NONE:CAIRO_ANTIALIAS_DEFAULT);
    cairo_set_tolerance(cr,0.55-quality/2.0);
  }
}

static void tiger_renderer_init_defaults(tiger_defaults *defaults)
{
  defaults->font_desc=pango_font_description_new();
  tiger_renderer_set_color(&defaults->font_color,1.0,1.0,1.0,1.0);
  tiger_renderer_set_color(&defaults->background_fill_color,0.0,0.0,0.0,0.0);
  defaults->font_effect=tiger_font_outline;
  defaults->font_effect_strength=0.5;
}

static void tiger_renderer_destroy_defaults(tiger_defaults *defaults)
{
  if (defaults->font_desc) {
    pango_font_description_free(defaults->font_desc);
    defaults->font_desc=NULL;
  }
}

static int tiger_renderer_init(tiger_renderer *tr)
{
  if (!tr) return TIGER_E_INVALID_PARAMETER;

  tr->nitems=0;
  tr->items=NULL;

  tr->cs=NULL;
  tr->cr=NULL;

  tr->id_generator=0;

  tr->flags=TIGER_FLAG_CACHING;
#ifdef DEBUG
  tr->flags|=TIGER_FLAG_DEBUG;
#endif

  tr->quality=-1.0;

  tr->clear=0;
  tiger_renderer_set_color(&tr->clear_color,0,0,0,0);

  /* defaults */
  tiger_renderer_init_defaults(&tr->defaults);

  tr->dirty=1;

  return 0;
}

/**
  \ingroup renderer
  Creates a new Tiger renderer
  \param tr where to store a pointer to the newly created renderer
  \returns 0 success
  \returns TIGER_E_* error
  */
int tiger_renderer_create(tiger_renderer **tr)
{
  int ret;

  if (!tr) return TIGER_E_INVALID_PARAMETER;

  *tr=(tiger_renderer*)tiger_malloc(sizeof(tiger_renderer));
  if (!*tr) return TIGER_E_OUT_OF_MEMORY;
  ret=tiger_renderer_init(*tr);
  if (ret<0) {
    tiger_free(*tr);
    return ret;
  }

  return 0;
}

static int tiger_renderer_set_cairo_surface(tiger_renderer *tr,cairo_surface_t *cs)
{
  if (!tr || !cs) return TIGER_E_INVALID_PARAMETER;

  /* first test whether the surface is an image or not */
  if (cairo_surface_get_type(cs)!=CAIRO_SURFACE_TYPE_IMAGE) return TIGER_E_BAD_SURFACE_TYPE;

  /* destroy any previous cairo surface */
  if (tr->cr) {
    cairo_destroy(tr->cr);
    tr->cr=NULL;
  }
  if (tr->cs) {
    tr->cs=NULL;
  }

  /* set the new cairo surface */
  tr->cr=cairo_create(cs);
  if (!tr->cr) return TIGER_E_CAIRO_ERROR;

  tr->cs=cs;
  tiger_renderer_set_flag(tr,TIGER_FLAG_SWAP_RGB,0);
  tiger_renderer_apply_quality(tr->cr,tr->quality);

  tr->dirty=1;

  return 0;
}

/**
  \ingroup renderer
  Sets a buffer where to render the Kate stream.
  This buffer should be a linear buffer with 32 bits per pixel, with 8 bits
  per color component in ARGB order (or ABGR is swap_rgb is non zero) in
  host endianness.
  This buffer will be used as backing store for a Cairo surface, so any other
  requirement by Cairo also applies here.
  \param tr the Tiger renderer for which to set the buffer
  \param ptr the data to use as backing store for the Cairo surface
  \param width the width of the Cairo surface to create
  \param height the height of the Cairo surface to create
  \param stride the stride of the Cairo surface to create
  \param swap_rgb if non zero, red and blue components are swapped
  \returns 0 success
  \returns TIGER_E_* error
  */
int tiger_renderer_set_buffer(tiger_renderer *tr,unsigned char *ptr,int width,int height,int stride,int swap_rgb)
{
  int ret;
  cairo_surface_t *cs;

  if (!tr) return TIGER_E_INVALID_PARAMETER;
  if (!ptr || width<0 || height<0 || stride<0) return TIGER_E_INVALID_PARAMETER;

  cs=cairo_image_surface_create_for_data(ptr,CAIRO_FORMAT_ARGB32,width,height,stride);
  if (!cs) return TIGER_E_CAIRO_ERROR;

  ret=tiger_renderer_set_cairo_surface(tr,cs);
  if (ret<0) {
    cairo_surface_destroy(cs);
    return ret;
  }

  /* the cairo_t keeps a reference to the new surface, so we can unref it now */
  cairo_surface_destroy(cs);

  tiger_renderer_set_flag(tr,TIGER_FLAG_SWAP_RGB,swap_rgb);

  tr->dirty=1;

  return 0;
}

/**
  \ingroup renderer
  Sets a relative quality for the rendering
  \param tr the Tiger renderer for which to set the quality
  \param quality rendering quality, between 0 (lowest, fastest) and 1 (highest, slowest), or negative for default quality
  \returns 0 success
  \returns TIGER_E_* error
  */
int tiger_renderer_set_quality(tiger_renderer *tr,double quality)
{
  if (!tr) return TIGER_E_INVALID_PARAMETER;

  if (quality>1) quality=1;
  tr->quality=quality;

  if (tr->cr)
    tiger_renderer_apply_quality(tr->cr,quality);

  tr->dirty=1;
  tiger_renderer_invalidate_caches(tr);

  return 0;
}

static int tiger_renderer_destroy_item(tiger_renderer *tr,size_t idx)
{
  size_t n;
  int ret;

  if (!tr) return TIGER_E_INVALID_PARAMETER;
  if (idx>=tr->nitems) return TIGER_E_NOT_FOUND;

  ret=tiger_item_clear(tr->items+idx);

  /* keep the ordering */
  --tr->nitems;
  for (n=idx;n<tr->nitems;++n) {
    tr->items[n]=tr->items[n+1];
  }

  tr->dirty=1;

  return ret;
}

static int tiger_renderer_clear(tiger_renderer *tr)
{
  if (!tr) return TIGER_E_INVALID_PARAMETER;

  while (tr->nitems>0) {
    tiger_renderer_destroy_item(tr,tr->nitems-1);
  }
  tiger_free(tr->items);

  cairo_destroy(tr->cr);

  tiger_renderer_destroy_defaults(&tr->defaults);

  return 0;
}

/**
  \ingroup renderer
  Destroys a previously initialized Tiger renderer
  \param tr the Tiger renderer to clear
  \returns 0 success
  \returns TIGER_E_* error
  */
int tiger_renderer_destroy(tiger_renderer *tr)
{
  int ret;

  if (!tr) return TIGER_E_INVALID_PARAMETER;

  ret=tiger_renderer_clear(tr);

  tiger_free(tr);

  return ret;
}

/**
  \ingroup renderer
  Sets whether to clear the surface before rendering, and the clearing color
  \param tr the Tiger renderer to modify
  \param clear whether to clear before rendering
  \param r the red value of the clear color
  \param g the green value of the clear color
  \param b the blue value of the clear color
  \param a the alpha value of the clear color
  \returns 0 success
  \returns TIGER_E_* error
  */
int tiger_renderer_set_surface_clear_color(tiger_renderer *tr,int clear,double r,double g,double b,double a)
{
  if (!tr) return TIGER_E_INVALID_PARAMETER;

  tr->clear=clear;
  tiger_renderer_set_color(&tr->clear_color,r,g,b,a);

  tr->dirty=1;

  return 0;
}

/**
  \ingroup renderer
  Adds an event to the renderer
  \param tr the Tiger renderer to add the event to
  \param ki the kate_info for the stream this events belongs to
  \param ev the event to add to the renderer
  \returns 0 success
  \returns TIGER_E_* error
  */
int tiger_renderer_add_event(tiger_renderer *tr,const kate_info *ki,const kate_event *ev)
{
  tiger_item *items;
  int ret;

  if (!tr || !ev) return TIGER_E_INVALID_PARAMETER;

  items=(tiger_item*)tiger_realloc(tr->items,(tr->nitems+1)*sizeof(tiger_item));
  if (!items) return TIGER_E_OUT_OF_MEMORY;

  ret=tiger_item_init(items+tr->nitems,tr->id_generator++,tr->flags,tr->quality,&tr->defaults,ki,ev);
  if (ret<0) return ret;

  tr->items=items;
  ++tr->nitems;

  tr->dirty=1;

  return 0;
}

static int tiger_renderer_sort_items(const void *e1,const void *e2)
{
  const tiger_item *ti1=(const tiger_item*)e1;
  const tiger_item *ti2=(const tiger_item*)e2;
  double z1=tiger_item_get_z(ti1);
  double z2=tiger_item_get_z(ti2);
  if (z1>z2) {
    return -1;
  }
  else if (z1<z2) {
    return 1;
  }
  else {
    /* same depth, we want to stay stable, compare the ids */
    return ti1->id-ti2->id;
  }
}

/**
  \ingroup renderer
  Updates the renderer at the new time
  \param tr the Tiger renderer to update
  \param t the time to update to renderer to
  \param track whether to update item trackers
  \returns 0 success
  \returns 1 success, but no Kate events are currently active
  \returns TIGER_E_* error
  \note Even if 0 is returned, a call to draw may not actually change the rendering surface,
        for instance if all active Kate events have a text with fully transparent color and
        no background, etc.
  */
int tiger_renderer_update(tiger_renderer *tr,kate_float t,int track)
{
  size_t n;
  cairo_t *cr;
  int ret=0;
  int nactive=0;

  if (!tr || t<0) return TIGER_E_INVALID_PARAMETER;

  cr=tr->cr;

  tiger_log(tiger_log_debug,"tiger_renderer_update: %f, %d items\n",t,tr->nitems);

  /* update all items, removing any that are obsolete */
  for (n=0; n<tr->nitems; ++n) {
    int update_ret=tiger_item_update(tr->items+n,t,track,cr);
    if (update_ret>0) {
      /* positive indicates the item should be destroyed */
      tr->dirty=1;
      tiger_renderer_destroy_item(tr,n);
      --n;
    }
    else if (update_ret<0) {
      /* record the error, but continue, to avoid leaving items accumulate */
      ret=update_ret;
    }
    else {
      if (tiger_item_is_active(tr->items+n))
        ++nactive;
      if (tiger_item_is_dirty(tr->items+n))
        tr->dirty=1;
    }
  }

  if (ret==0) {
    if (nactive==0) ret=1;
  }

  return ret;
}

/**
  \ingroup renderer
  Registers that a seek occured
  \param tr the Tiger renderer on which the seek occured
  \param target the target time of the seek
  \returns 0 success
  \returns TIGER_E_* error
  */
int tiger_renderer_seek(tiger_renderer *tr,kate_float target)
{
  size_t n;

  if (!tr) return TIGER_E_INVALID_PARAMETER;

  /* if we're going back in time, we need to kill everything that starts after
     the target time
     if we're going forward in time, we need to kill everything that stops before
     the target time, as in a normal update; force tracking, as a seek is going
     to be a bit slow anyway, and we'll be sure we don't have a frame where things
     are badly rendered if render is called before update
     so we kill anything that is not live at the time of the week target - we
     could avoid killing those not started yet, and make them inactive, this would
     avoid possibly losing them if we don't get them again, but we'd risk getting
     duplicates and we have no way to tell
  */
  for (n=0; n<tr->nitems; ++n) {
    int ret=tiger_item_seek(tr->items+n,target);
    if (ret<0 || ret>0) {
      /* destroy items throwing an error as well as those that are off */
      tiger_renderer_destroy_item(tr,n);
      --n;
    }
  }

  /* just in case */
  tr->dirty=1;

  return 0;
}

/**
  \ingroup renderer
  Renders the current state of the renderer
  \param tr the Tiger renderer to update
  \returns 0 success
  \returns TIGER_E_* error
  */
int tiger_renderer_render(tiger_renderer *tr)
{
  size_t n;
  cairo_t *cr;
  int ret=0;

  if (!tr) return TIGER_E_INVALID_PARAMETER;

  cr=tr->cr;

  tiger_log(tiger_log_debug,"tiger_renderer_render: %d items\n",tr->nitems);

  cairo_save(cr);

#if 0
  /* test: move everything to the top right corner */
  cairo_translate(cr,480,0);
  cairo_scale(cr,0.25,0.25);
#endif

#if 0
  /* test: matrix stuff */
  {
    cairo_matrix_t m;
    cairo_matrix_init(
      &m,
      1.0,  0.0,
      0.3,  0.7,
      0.0,  0.0
    );
    cairo_transform(cr,&m);
  }
#endif

  if (tr->clear) {
    cairo_save(cr);
    cairo_set_operator(cr,CAIRO_OPERATOR_SOURCE);
    tiger_pixel_format_set_source_color(cr,tr->flags&TIGER_FLAG_SWAP_RGB,&tr->clear_color);
    cairo_paint(cr);
    cairo_restore(cr);
  }

  /* sort the items by depth */
  qsort(tr->items,tr->nitems,sizeof(*tr->items),&tiger_renderer_sort_items);

  /* render the items */
  if (ret==0) {
    for (n=0; n<tr->nitems; ++n) {
      ret=tiger_item_render(tr->items+n,cr);
      if (ret<0) break;
    }
  }

#if 0
  /* test event counter */
  cairo_set_source_rgba(cr,1,0.5,1,0.6);
  for (n=0;n<tr->nitems;++n) {
    cairo_rectangle(cr,10+20*n,10,10,20);
  }
  cairo_fill(cr);
#endif

  cairo_restore(cr);

  cairo_surface_flush(tr->cs);

  tr->dirty=0;

#ifdef DEBUG
  if (tr->flags&TIGER_FLAG_DUMP_FRAMES) {
    static int i=0; char name[48]; sprintf(name,"/tmp/tiger-%d.png",i++);
    if (cairo_surface_write_to_png(tr->cs,name)!=CAIRO_STATUS_SUCCESS) {
      tiger_log(tiger_log_error,"png write trouble\n");
    }
  }
#endif

  return ret;
}

/**
  \ingroup renderer
  Checks whether the Tiger renderer is dirty or not (eg, whether rendering a frame now
  would yield a different frame from the previous frame).
  \param tr the Tiger renderer to query
  \returns 0 success, and the Tiger renderer is not dirrty
  \returns !0 success, the Tiger renderer is dirty
  \note The client code may choose not to call tiger_renderer_render if the renderer is not dirty
        as an optimization. However, care should be taken if the buffer the frame was rendered
        onto was changed (eg, if a new video frame was drawn onto it); in this case, a new frame
        should be rendered on top of the video, even if the Tiger frame would be the same as the
        previous frame, as it would have been erased by the video.
  */
int tiger_renderer_is_dirty(const tiger_renderer *tr)
{
  if (!tr) return 0;
  return tr->dirty;
}

/**
  \ingroup renderer
  Sets the default font description to use if no override is specified
  \param tr the Tiger renderer for which to set the default font
  \param desc the font description to use as default
  \returns 0 success
  \returns TIGER_E_* error
  */
int tiger_renderer_set_default_font_description(tiger_renderer *tr,const char *desc)
{
  PangoFontDescription *default_font_desc=NULL;

  if (!tr) return TIGER_E_INVALID_PARAMETER;

  if (desc) {
    default_font_desc=pango_font_description_from_string(desc);
  }
  else {
    default_font_desc=pango_font_description_new();
  }
  if (!default_font_desc) return TIGER_E_OUT_OF_MEMORY;
  if (tr->defaults.font_desc) pango_font_description_free(tr->defaults.font_desc);
  tr->defaults.font_desc=default_font_desc;

  tr->dirty=1;
  tiger_renderer_invalidate_caches(tr);

  return 0;
}

/**
  \ingroup renderer
  Sets the default font to use if none is specified
  \param tr the Tiger renderer for which to set the default font
  \param font the font to use as default, a copy of which is made
  \returns 0 success
  \returns TIGER_E_* error
  */
int tiger_renderer_set_default_font(tiger_renderer *tr,const char *font)
{
  if (!tr) return TIGER_E_INVALID_PARAMETER;

  /* ensure we have a font desc */
  if (!tr->defaults.font_desc) {
    tr->defaults.font_desc=pango_font_description_new();
    if (!tr->defaults.font_desc) return TIGER_E_OUT_OF_MEMORY;
  }

  if (font) {
    pango_font_description_set_family(tr->defaults.font_desc,font);
  }
  else {
    pango_font_description_unset_fields(tr->defaults.font_desc,PANGO_FONT_MASK_FAMILY);
  }

  tr->dirty=1;
  tiger_renderer_invalidate_caches(tr);

  return 0;
}

/**
  \ingroup renderer
  Sets the default font size to use if none is specified
  \param tr the Tiger renderer for which to set the default font size
  \param size the font size to use as default
  \returns 0 success
  \returns TIGER_E_* error
  */
int tiger_renderer_set_default_font_size(tiger_renderer *tr,double size)
{
  if (!tr) return TIGER_E_INVALID_PARAMETER;

  /* ensure we have a font desc */
  if (!tr->defaults.font_desc) {
    tr->defaults.font_desc=pango_font_description_new();
    if (!tr->defaults.font_desc) return TIGER_E_OUT_OF_MEMORY;
  }

  pango_font_description_set_absolute_size(tr->defaults.font_desc,size*PANGO_SCALE);

  tr->dirty=1;
  tiger_renderer_invalidate_caches(tr);

  return 0;
}

/**
  \ingroup renderer
  Sets the default font color to use if none is specified
  \param tr the Tiger renderer for which to set the default font color
  \param r the red component of the default color, in the 0-1 range
  \param g the green component of the default color, in the 0-1 range
  \param b the blue component of the default color, in the 0-1 range
  \param a the alpha component of the default color, in the 0-1 range
  \returns 0 success
  \returns TIGER_E_* error
  */
int tiger_renderer_set_default_font_color(tiger_renderer *tr,double r,double g, double b,double a)
{
  if (!tr) return TIGER_E_INVALID_PARAMETER;

  tiger_renderer_set_color(&tr->defaults.font_color,r,g,b,a);

  tr->dirty=1;
  tiger_renderer_invalidate_caches(tr);

  return 0;
}

/**
  \ingroup renderer
  Sets the default font color to use for background. The default is to not fill
  the background (though a Kate event may specify a background), but in cases
  where the text may be hard to read over a video, specifying a background fill
  color will allow the text to stand out more.
  \param tr the Tiger renderer for which to set the background fill color
  \param r the red component of the background fill color, in the 0-1 range
  \param g the green component of the background fill color, in the 0-1 range
  \param b the blue component of the background fill color, in the 0-1 range
  \param a the alpha component of the background fill color, in the 0-1 range
  \returns 0 success
  \returns TIGER_E_* error
  */
int tiger_renderer_set_default_background_fill_color(tiger_renderer *tr,double r,double g,double b,double a)
{
  if (!tr) return TIGER_E_INVALID_PARAMETER;

  tiger_renderer_set_color(&tr->defaults.background_fill_color,r,g,b,a);

  tr->dirty=1;
  tiger_renderer_invalidate_caches(tr);

  return 0;
}

/**
  \ingroup renderer
  Sets the default font effect. The default is to have no effect (tiger_font_plain).
  \param tr the Tiger renderer for which to set which font effect to use
  \param effect the font effect to use
  \param strength how pronounced to make the effect (effect dependent)
  \returns 0 success
  \returns TIGER_E_* error
  */
int tiger_renderer_set_default_font_effect(tiger_renderer *tr,tiger_font_effect effect,double strength)
{
  if (!tr) return TIGER_E_INVALID_PARAMETER;

  tr->defaults.font_effect=effect;
  tr->defaults.font_effect_strength=strength;

  tr->dirty=1;
  tiger_renderer_invalidate_caches(tr);

  return 0;
}

/**
  \ingroup renderer
  Enables or disables internal caching of text layouts
  \param tr the Tiger renderer for which to enable or disable caching
  \param enable whether to enable or disable caching
  \returns 0 success
  \returns TIGER_E_* error
  */
int tiger_renderer_enable_caching(tiger_renderer *tr,int enable)
{
  return tiger_renderer_set_flag(tr,TIGER_FLAG_CACHING,enable);
}

#ifdef DEBUG
int tiger_renderer_enable_debug(tiger_renderer *tr,int debug)
{
  return tiger_renderer_set_flag(tr,TIGER_FLAG_DEBUG,debug);
}
#endif

