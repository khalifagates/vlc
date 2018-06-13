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


#ifndef TIGER_tiger_h_GUARD
#define TIGER_tiger_h_GUARD

/** \file tiger.h
  The libtiger public API.
  */

#include <stddef.h>
#include <kate/kate.h>
#include "tiger/tiger_config.h"

/** \name API version */
/** @{ */
#define TIGER_VERSION_MAJOR 0            /**< major version number of the libtiger API */
#define TIGER_VERSION_MINOR 3            /**< minor version number of the libtiger API */
#define TIGER_VERSION_PATCH 4            /**< patch version number of the libtiger API */
/** @} */

typedef struct tiger_renderer tiger_renderer;

typedef enum {
  tiger_font_plain,
  tiger_font_shadow,
  tiger_font_outline
} tiger_font_effect;

#ifdef __cplusplus
extern "C" {
#endif

/** \defgroup version Version information */
extern int tiger_get_version(void);
extern const char *tiger_get_version_string(void);

/** \defgroup renderer */
extern int tiger_renderer_create(tiger_renderer **tr);
extern int tiger_renderer_set_buffer(tiger_renderer *tr,unsigned char *ptr,int width,int height,int stride,int swap_rgb);
extern int tiger_renderer_set_surface_clear_color(tiger_renderer *tr,int clear,double r,double g,double b,double a);
extern int tiger_renderer_set_quality(tiger_renderer *tr,double quality);
extern int tiger_renderer_add_event(tiger_renderer *tr,const kate_info *ki,const kate_event *ev);
extern int tiger_renderer_update(tiger_renderer *tr,kate_float t,int track);
extern int tiger_renderer_seek(tiger_renderer *tr, kate_float target);
extern int tiger_renderer_render(tiger_renderer *tr);
extern int tiger_renderer_destroy(tiger_renderer *tr);
extern int tiger_renderer_is_dirty(const tiger_renderer *tr);
extern int tiger_renderer_enable_caching(tiger_renderer *tr,int enable);

extern int tiger_renderer_set_default_font_description(tiger_renderer *tr,const char *desc);
extern int tiger_renderer_set_default_font(tiger_renderer *tr,const char *font);
extern int tiger_renderer_set_default_font_size(tiger_renderer *tr,double size);
extern int tiger_renderer_set_default_font_color(tiger_renderer *tr,double r,double g, double b,double a);

extern int tiger_renderer_set_default_background_fill_color(tiger_renderer *tr,double r,double g,double b,double a);
extern int tiger_renderer_set_default_font_effect(tiger_renderer *tr,tiger_font_effect effect,double strength);

#ifdef DEBUG
/** \defgroup debug */
extern int tiger_renderer_enable_debug(tiger_renderer *tr,int debug);
#endif

#ifdef __cplusplus
}
#endif

/** \name Error codes */
/** @{ */
/* note that libkate errors may be returned by libtiger - see libkate API for more details */
#define TIGER_E_NOT_FOUND (-1001)            /**< whatever was requested was not found */
#define TIGER_E_INVALID_PARAMETER (-1002)    /**< a bogus parameter was passed (usually NULL) */
#define TIGER_E_OUT_OF_MEMORY (-1003)        /**< we're running out of cheese, bring some more */
#define TIGER_E_CAIRO_ERROR (-1004)          /**< the Cairo API returned an error */
#define TIGER_E_BAD_SURFACE_TYPE (-1005)     /**< this surface type is unsupported */
/** @} */

#endif

