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


#ifndef TIGER_tiger_internal_h_GUARD
#define TIGER_tiger_internal_h_GUARD

#include <pango/pango.h>
#include <cairo/cairo.h>
#include <kate/kate.h>
#include "tiger/tiger.h"
#include "tiger_thread.h"
#include "tiger_mutex.h"

#if !defined __GNUC__ || (((__GNUC__*0x100)+(__GNUC_MINOR__+0))<0x0303)
#define __attribute__(x)
#endif

#ifdef __ELF__
#define tiger_internal __attribute__((visibility("internal")))
#else
#define tiger_internal
#endif

#define TIGER_FLAG_DEBUG (1<<0)
#define TIGER_FLAG_CACHING (1<<1)
#define TIGER_FLAG_DUMP_FRAMES (1<<2)
#define TIGER_FLAG_DUMP_CACHED_RENDERS (1<<3)
#define TIGER_FLAG_SWAP_RGB (1<<4)

typedef struct tiger_color {
  double r;
  double g;
  double b;
  double a;
} tiger_color;

typedef struct tiger_rectangle {
  double x0;
  double y0;
  double x1;
  double y1;
} tiger_rectangle;

typedef struct tiger_defaults {
  PangoFontDescription *font_desc;
  tiger_color font_color;
  tiger_color background_fill_color;
  tiger_font_effect font_effect;
  double font_effect_strength;
} tiger_defaults;

typedef struct tiger_draw {
  const kate_tracker *kin;
  const kate_motion *draw_motion;
  const kate_motion *draw_color_rg_motion;
  const kate_motion *draw_color_ba_motion;
  const kate_motion *draw_width_motion;
  tiger_color base_draw_color;
  int needs_stepped_strokes;

  /* local cache */
  kate_float curve_start_t;
  kate_float curve_duration;
  int swap_rgb;
} tiger_draw;

typedef struct tiger_bitmap_cache_primer tiger_bitmap_cache_primer;
typedef struct tiger_bitmap tiger_bitmap;
typedef struct tiger_bitmap_cache {
  size_t size;
  struct tiger_bitmap **tb;
  int swap_rgb;

  tiger_bitmap_cache_primer *primer;
} tiger_bitmap_cache;

typedef struct tiger_item {
  kate_tracker kin;

  unsigned int id;

  double quality;

  /* defaults */
  const tiger_defaults *defaults;

  /* output setup */
  double frame_width;
  double frame_height;

  int active;

  /* Pango */
  PangoFontMap *font_map;
  PangoContext *context;

  int vertical;

  /* region */
  int explicit_region;
  tiger_rectangle region;
  tiger_rectangle mregion;

  int justify;

  /* color */
  tiger_color text_color;
  tiger_color background_color;

  /* draw */
  tiger_draw draw;

  /* cache */
  cairo_surface_t *cached_render;
  tiger_rectangle cached_render_bounds;
  double cached_render_dx;
  double cached_render_dy;

  tiger_bitmap_cache *tbc;

  int dirty;

  kate_uint32_t flags;
} tiger_item;

struct tiger_renderer {
  size_t nitems;
  struct tiger_item *items;

  cairo_surface_t *cs;
  cairo_t *cr;

  double quality;

  int clear;
  tiger_color clear_color;

  unsigned int id_generator;

  tiger_defaults defaults;

  int dirty;

  kate_uint32_t flags;
};

typedef enum {
  tiger_log_error,
  tiger_log_warning,
  tiger_log_debug,
  tiger_log_profile,
  tiger_log_spam
} tiger_log_level;

/* item */
extern int tiger_item_init(tiger_item *ti,unsigned int id,kate_uint32_t flags,double quality,const tiger_defaults *defaults,const kate_info *ki,const kate_event *ev) tiger_internal;
extern int tiger_item_clear(tiger_item *ti) tiger_internal;
extern int tiger_item_seek(tiger_item *ti,kate_float t) tiger_internal;
extern int tiger_item_update(tiger_item *ti,kate_float t,int track,cairo_t *cr) tiger_internal;
extern int tiger_item_render(tiger_item *ti,cairo_t *cr) tiger_internal;
extern kate_float tiger_item_get_z(const tiger_item *ti) tiger_internal;
extern int tiger_item_is_active(const tiger_item *ti) tiger_internal;
extern int tiger_item_is_dirty(const tiger_item *ti) tiger_internal;
extern int tiger_item_invalidate_cache(tiger_item *ti) tiger_internal;

/* draw */
extern int tiger_draw_init(tiger_draw *td,const kate_tracker *kin) tiger_internal;
extern int tiger_draw_clear(tiger_draw *td) tiger_internal;
extern int tiger_draw_render(tiger_draw *td,cairo_t *cr,kate_float t,int swap_rgb) tiger_internal;

/* util */
extern int tiger_util_remap_metric_double(double v0,kate_space_metric metric,double unit,double *v1) tiger_internal;
extern int tiger_util_remap_metric_int(int v0,kate_space_metric metric,int unit,int *v1) tiger_internal;
extern void tiger_util_kate_color_to_tiger_color(tiger_color *tc,const kate_color *kc);
extern void tiger_util_usleep(unsigned int microseconds) tiger_internal;

/* bitmap */
extern int tiger_bitmap_create(tiger_bitmap **tb,const kate_bitmap *kb,const kate_palette *kp,int swap_rgb) tiger_internal;
extern int tiger_bitmap_matches(tiger_bitmap *tb,const kate_bitmap *kb,const kate_palette *kp) tiger_internal;
extern int tiger_bitmap_destroy(tiger_bitmap *tb) tiger_internal;
extern int tiger_bitmap_render(tiger_bitmap *tb,cairo_t *cr,int fill,double quality) tiger_internal;

/* bitmap cache */
extern int tiger_bitmap_cache_init(tiger_bitmap_cache *tbc,int swap_rgb) tiger_internal;
extern int tiger_bitmap_cache_clear(tiger_bitmap_cache *tbc) tiger_internal;
extern int tiger_bitmap_cache_get(tiger_bitmap_cache *tbc,const kate_bitmap *kb,const kate_palette *kp,tiger_bitmap **tb) tiger_internal;
extern void tiger_bitmap_cache_prime(tiger_bitmap_cache *tbc,const kate_bitmap *kb,const kate_palette *kp) tiger_internal;

/* rectangle */
extern void tiger_rectangle_empty(tiger_rectangle *r) tiger_internal;
extern void tiger_rectangle_snap(const tiger_rectangle *r,tiger_rectangle *snapped) tiger_internal;
extern void tiger_rectangle_extend(tiger_rectangle *r,const tiger_rectangle *extend) tiger_internal;
extern void tiger_rectangle_grow(tiger_rectangle *r,double x,double y) tiger_internal;
extern void tiger_rectangle_clip(tiger_rectangle *r,const tiger_rectangle *clip) tiger_internal;
extern void tiger_rectangle_order(const tiger_rectangle *r,tiger_rectangle *ordered) tiger_internal;

/* pixel format */
extern void tiger_pixel_format_set_source_color(cairo_t *cr,int swap,const tiger_color *tc) tiger_internal;

/* debug */
#if defined DEBUG || defined PROFILE
extern void tiger_log(tiger_log_level level,const char *format,...) tiger_internal;
#else
static inline void tiger_log(tiger_log_level level,const char *format,...) { (void)level;(void)format; }
#endif

/* profile */
#ifdef PROFILE
extern unsigned long tiger_profile_now(void) tiger_internal;
#define PROFILE_START(name) unsigned long profile_##name=tiger_profile_now()
#define PROFILE_STOP(name) tiger_log(tiger_log_profile,#name ": %f ms\n",(tiger_profile_now()-profile_##name)/1000.0)
#else
#define PROFILE_START(name) ((void)0)
#define PROFILE_STOP(name) ((void)0)
#endif

#endif

