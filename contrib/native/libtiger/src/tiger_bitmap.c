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

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#include <string.h>

#include "tiger/tiger.h"

struct tiger_bitmap {
  const kate_bitmap *kb;
  const kate_palette *kp;
  cairo_surface_t *cs;
};

struct tiger_png_reader_state {
  const kate_bitmap *kb;
  size_t offset;
};

static cairo_status_t read_from_kate_bitmap(void *closure,unsigned char *data,unsigned int length)
{
  struct tiger_png_reader_state *state=closure;
  if (length>state->kb->size-state->offset) return CAIRO_STATUS_READ_ERROR;
  memcpy(data,state->kb->pixels+state->offset,length);
  state->offset+=length;
  return CAIRO_STATUS_SUCCESS;
}

static void tiger_bitmap_swap_rgb(cairo_surface_t *cs)
{
  /* slow, could there be a better way ? when the PNG is decoded ? use liboil ? */
  unsigned char *data,*ptr;
  int x,y,w,h,stride;

  cairo_surface_flush(cs);
  data=cairo_image_surface_get_data(cs);
  stride=cairo_image_surface_get_stride(cs);
  w=cairo_image_surface_get_width(cs);
  h=cairo_image_surface_get_height(cs);
  ptr=data;
  for (y=0;y<h;++y,ptr+=stride) {
    uint32_t *ptr32=(uint32_t*)ptr;
    for (x=0;x<w;++x) {
      uint32_t r=((*ptr32)&0x00ff0000)>>16;
      uint32_t b=((*ptr32)&0x000000ff)<<16;
      *ptr32=((*ptr32)&0xff00ff00)|r|b;
      ptr32++;
    }
  }
  cairo_surface_mark_dirty(cs);
}

static cairo_surface_t *tiger_bitmap_create_png(const kate_bitmap *kb,int swap_rgb)
{
  cairo_surface_t *cs;
  struct tiger_png_reader_state state;
  state.kb=kb;
  state.offset=0;
  cs=cairo_image_surface_create_from_png_stream(&read_from_kate_bitmap,&state);
  if (cs && swap_rgb) {
    tiger_bitmap_swap_rgb(cs);
  }
  return cs;
}

static cairo_surface_t *tiger_bitmap_create_paletted_bitmap(const kate_bitmap *kb,const kate_palette *kp,int swap_rgb)
{
  cairo_surface_t *cs;
  unsigned char *data,*ptr;
  int w,h,stride;
  int x,y,p;
  kate_color kc;
  kate_color *palette;
  size_t c;

  w=kb->width;
  h=kb->height;
  tiger_log(tiger_log_debug,"creating image surface: %d %d\n",w,h);
  cs=cairo_image_surface_create(CAIRO_FORMAT_ARGB32,w,h);
  tiger_log(tiger_log_debug,"image surface: %p\n",cs);
  if (!cs) return NULL;

  /* safety - as we're going to write direct to the backing store */
  cairo_surface_flush(cs);

  /* get access parameters */
  data=cairo_image_surface_get_data(cs);
  stride=cairo_image_surface_get_stride(cs);
  tiger_log(tiger_log_debug,"image data: %p, stride: %d\n",data,stride);

  /* create a copy of the palette, and force the color of the entries with zero
     alpha to zero, as those colors will bleed when using Cairo antialiasing
     when drawing the bitmap, and Cairo will ignore the "other" pixel's alpha.
     A black color will mean a black border, which is often a good choice. */
  palette=(kate_color*)tiger_malloc(kp->ncolors*sizeof(kate_color));
  for (c=0;c<kp->ncolors;++c) {
    palette[c]=kp->colors[c];
    if (swap_rgb) {
      unsigned char tmp=palette[c].r;
      palette[c].r=palette[c].b;
      palette[c].b=tmp;
    }
    if (palette[c].a==0) {
      palette[c].r=0;
      palette[c].g=0;
      palette[c].b=0;
    }
  }

  /* blit, brute force */
  ptr=data;
  p=0;
  for (y=0;y<h;++y,ptr+=stride) {
    uint32_t *ptr32=(uint32_t*)ptr;
    for (x=0;x<w;++x) {
      kc=palette[kb->pixels[p++]];
      /* Cairo docs say the colors are ARGB in native endianness */
      /* Colors are already swapped in the temporary palette if needed */
      *ptr32++=(kc.a<<24)|(kc.r<<16)|(kc.g<<8)|kc.b;
    }
  }

  tiger_free(palette);

  cairo_surface_mark_dirty(cs);

#if 0 && defined DEBUG
  if (cs) {
    static int i=0; char name[48]; sprintf(name,"/tmp/tiger-bitmap-%d.png",i++);
    if (cairo_surface_write_to_png(cs,name)!=CAIRO_STATUS_SUCCESS) {
      tiger_log(tiger_log_error,"png write trouble\n");
    }
  }
#endif

  return cs;
}

int tiger_bitmap_create(tiger_bitmap **tb,const kate_bitmap *kb,const kate_palette *kp,int swap_rgb)
{
  if (!tb || !kb) return TIGER_E_INVALID_PARAMETER;
  /* kp may be NULL */

  *tb=(tiger_bitmap*)tiger_malloc(sizeof(tiger_bitmap));
  if (!*tb) return TIGER_E_OUT_OF_MEMORY;

  (*tb)->cs=NULL;

  switch (kb->type) {
    case kate_bitmap_type_paletted:
      if (!kp) return TIGER_E_INVALID_PARAMETER;
      (*tb)->cs=tiger_bitmap_create_paletted_bitmap(kb,kp,swap_rgb);
      break;
    case kate_bitmap_type_png:
      (*tb)->cs=tiger_bitmap_create_png(kb,swap_rgb);
      break;
  }
  if (!(*tb)->cs) {
    tiger_log(tiger_log_error,"failed to create Cairo surface from kate_bitmap\n");
    return TIGER_E_CAIRO_ERROR;
  }

  (*tb)->kb=kb;
  (*tb)->kp=kp;

  return 0;
}

static void tiger_bitmap_setup_filter_quality(cairo_pattern_t *cp,double quality)
{
  if (quality>=0.0) {
    cairo_filter_t filter=CAIRO_FILTER_BEST;
    if (quality<=0.1) filter=CAIRO_FILTER_FAST;
    else if (quality<=0.5) filter=CAIRO_FILTER_GOOD;
    else filter=CAIRO_FILTER_BEST;
    cairo_pattern_set_filter(cp,filter);
  }
}

int tiger_bitmap_render(tiger_bitmap *tb,cairo_t *cr,int fill,double quality)
{
  if (!tb || !cr) return TIGER_E_INVALID_PARAMETER;

  if (tb->cs) {
    cairo_pattern_t *cp=cairo_pattern_create_for_surface(tb->cs);
    cairo_set_source(cr,cp);
    tiger_bitmap_setup_filter_quality(cp,quality);
    if (fill) cairo_fill(cr); else cairo_paint(cr);
    cairo_pattern_destroy(cp);
  }

  return 0;
}

int tiger_bitmap_matches(tiger_bitmap *tb,const kate_bitmap *kb,const kate_palette *kp)
{
  if (!tb) return 0;
  return tb->kb==kb && tb->kp==kp;
}

int tiger_bitmap_destroy(tiger_bitmap *tb)
{
  if (!tb) return TIGER_E_INVALID_PARAMETER;

  if (tb->cs) {
    cairo_surface_destroy(tb->cs);
  }

  tiger_free(tb);

  return 0;
}

