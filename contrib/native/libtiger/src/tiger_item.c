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
#include <math.h>
#include <pango/pangocairo.h>
#include "tiger/tiger.h"

#ifdef DEBUG
#define TIGER_CHECK_CAIRO_STATUS(cr) \
  do { \
    cairo_status_t status=cairo_status(cr); \
    if (status) { \
      tiger_log(tiger_log_error,"Cairo error at %s:%d: %d (%s)\n",__FILE__,__LINE__,status,cairo_status_to_string(status)); \
    } \
  } while(0)
#else
#define TIGER_CHECK_CAIRO_STATUS(cr) ((void)cr)
#endif

static inline int tiger_item_has_flag(const tiger_item *ti,kate_uint32_t flag) __attribute__((pure));
static inline int tiger_item_has_flag(const tiger_item *ti,kate_uint32_t flag)
{
  return ti->flags&flag;
}

static void tiger_item_prime(tiger_item *ti)
{
  size_t n;
  const kate_event *ev=ti->kin.event;
  if (ev) {
    if (ev->bitmap) {
      tiger_bitmap_cache_prime(ti->tbc,ev->bitmap,ev->palette);
    }
    for (n=0;n<ev->nbitmaps;++n) {
      tiger_bitmap_cache_prime(ti->tbc,ev->bitmaps[n],ev->palette);
    }
  }
}

/**
  \ingroup item
  Initializes a Tiger item from a kate event
  \param tiger the Tiger item to initialize
  \param ki the kate info for the stream this event belongs to
  \param ev the kate event to use for this item
  \returns 0 success
  \returns TIGER_E_* error
  */
int tiger_item_init(tiger_item *ti,unsigned int id,kate_uint32_t flags,double quality,const tiger_defaults *defaults,const kate_info *ki,const kate_event *ev)
{
  int ret;

  if (!ti || !ki || !ev || !defaults) return TIGER_E_INVALID_PARAMETER;

  ret=kate_tracker_init(&ti->kin,ki,ev);
  if (ret<0) return ret;

  ti->defaults=defaults;
  ti->id=id;
  ti->flags=flags;
  ti->quality=quality;

  ti->active=0;

  ti->context=NULL;
  ti->font_map=NULL;

  switch (ev->text_directionality) {
    case kate_t2b_l2r:
    case kate_t2b_r2l:
      ti->vertical=1;
      break;
    default:
      ti->vertical=0;
      break;
  }

  ti->tbc=(tiger_bitmap_cache*)tiger_malloc(sizeof(tiger_bitmap_cache));
  if (!ti->tbc) {
    kate_tracker_clear(&ti->kin);
    return TIGER_E_OUT_OF_MEMORY;
  }
  tiger_bitmap_cache_init(ti->tbc,tiger_item_has_flag(ti,TIGER_FLAG_SWAP_RGB));

  tiger_draw_init(&ti->draw,&ti->kin);

  tiger_item_prime(ti);

  ti->cached_render=NULL;

  /* ensure these are set to no offset: if a cache is created, they'll be set to the correct offset */
  ti->cached_render_dx=0;
  ti->cached_render_dy=0;

  /* not dirty yet, as inactive */
  ti->dirty=0;

  return 0;
}

static void tiger_item_extend_rectangle_with_pango_line_extents(tiger_rectangle *r,PangoLayoutLine *line,double font_width_ratio,cairo_t *cr,int vertical)
{
  PangoRectangle ink;
  tiger_rectangle bounds;
  double x,y;

  cairo_get_current_point(cr,&x,&y);
  cairo_user_to_device(cr,&x,&y);

  pango_layout_line_get_pixel_extents(line,&ink,NULL);
  if (vertical) {
    /* Pango puts the baseline in the middle for vertical text */
    bounds.x0=x-ink.height*0.5*font_width_ratio;
    bounds.y0=y+ink.x;
    bounds.x1=bounds.x0+ink.height*font_width_ratio;
    bounds.y1=bounds.y0+ink.width;
  }
  else {
    bounds.x0=x+ink.x*font_width_ratio;
    bounds.y0=y+ink.y;
    bounds.x1=bounds.x0+ink.width*font_width_ratio;
    bounds.y1=bounds.y0+ink.height;
  }

  tiger_rectangle_order(&bounds,&bounds);
  tiger_rectangle_extend(r,&bounds);
}

static void tiger_item_extend_rectangle_with_fill(tiger_rectangle *r,cairo_t *cr)
{
  tiger_rectangle bounds;
  cairo_fill_extents(cr,&bounds.x0,&bounds.y0,&bounds.x1,&bounds.y1);
  cairo_user_to_device(cr,&bounds.x0,&bounds.y0);
  cairo_user_to_device(cr,&bounds.x1,&bounds.y1);
  tiger_rectangle_order(&bounds,&bounds);
  tiger_rectangle_extend(r,&bounds);
}

static void tiger_item_clip_rectangle(tiger_rectangle *r,cairo_t *cr)
{
  tiger_rectangle clip;
  cairo_clip_extents(cr,&clip.x0,&clip.y0,&clip.x1,&clip.y1);
  cairo_user_to_device(cr,&clip.x0,&clip.y0);
  cairo_user_to_device(cr,&clip.x1,&clip.y1);
  tiger_rectangle_clip(r,&clip);
}

static inline double tiger_item_int_align_helper(double x0,double x1,double w,double align) __attribute__((const));
static inline double tiger_item_int_align_helper(double x0,double x1,double w,double align)
{
  return x0+((x1-w)-(x0))*(align+1.0)/2.0;
}

static inline double tiger_item_ext_align_helper(double x0,double x1,double w,double align) __attribute__((const));
static inline double tiger_item_ext_align_helper(double x0,double x1,double w,double align)
{
  return x0-w+(x1-(x0-w))*(align+1.0)/2.0;
}

static int tiger_item_draw_background_bitmap(tiger_item *ti,cairo_t *cr,const kate_bitmap *kb,const kate_palette *kp,int scale_to_region)
{
  tiger_bitmap *tb;
  const kate_tracker *kin=&ti->kin;
  cairo_matrix_t matrix;
  int ret;

  ret=tiger_bitmap_cache_get(ti->tbc,kb,kp,&tb);
  if (ret<0) return ret;

  cairo_matrix_init_identity(&matrix);

  /* offset by the bitmap's logical center */
  cairo_matrix_translate(&matrix,-kb->x_offset,-kb->y_offset);

  /* offset to position */
  if (kin->has.bitmap_pos) {
    cairo_matrix_translate(&matrix,kin->bitmap_x,kin->bitmap_y);
  }
  else {
    cairo_matrix_translate(&matrix,ti->region.x0,ti->region.y0);
  }

  /* scale to appropriate size */
  if (kin->has.bitmap_size) {
    cairo_matrix_scale(&matrix,kin->bitmap_size_x/kb->width,kin->bitmap_size_y/kb->height);
  }
  else if (scale_to_region && ti->explicit_region) {
    /* if we have an explicit region, the bitmap is scaled to it (without margins) */
    cairo_matrix_scale(&matrix,(ti->region.x1-ti->region.x0)/kb->width,(ti->region.y1-ti->region.y0)/kb->height);
  }
  else if (kin->ki->original_canvas_width>0 && kin->ki->original_canvas_height>0) {
    double x_scale=kin->ki->original_canvas_width/(double)ti->frame_width;
    double y_scale=kin->ki->original_canvas_height/(double)ti->frame_height;
    cairo_matrix_scale(&matrix,x_scale,y_scale);
  }

  cairo_transform(cr,&matrix);

  return tiger_bitmap_render(tb,cr,0,ti->quality);
}

static void tiger_item_fill_background_color(tiger_item *ti,cairo_t *cr,const tiger_color *tc)
{
  tiger_pixel_format_set_source_color(cr,tiger_item_has_flag(ti,TIGER_FLAG_SWAP_RGB),tc);
  TIGER_CHECK_CAIRO_STATUS(cr);
  cairo_rectangle(cr,ti->region.x0,ti->region.y0,ti->region.x1-ti->region.x0,ti->region.y1-ti->region.y0);
  TIGER_CHECK_CAIRO_STATUS(cr);
  cairo_fill(cr);
  TIGER_CHECK_CAIRO_STATUS(cr);
}

static void tiger_item_draw_background(tiger_item *ti,cairo_t *cr)
{
  /* fill only if we actually have a region */
  if (ti->explicit_region) {
    /* paint with a background color if available */
    if (ti->background_color.a>0.0) {
      tiger_item_fill_background_color(ti,cr,&ti->background_color);
    }
    else if (ti->defaults->background_fill_color.a>0.0) {
      tiger_item_fill_background_color(ti,cr,&ti->defaults->background_fill_color);
    }
  }

  /* draw a background bitmap if available */
  if (ti->kin.event->bitmap) {
    const kate_bitmap *kb=ti->kin.event->bitmap;
    const kate_palette *kp=ti->kin.event->palette;
    if (!kp && kb->bpp>0) {
      int idx=kb->palette;
      if (idx>=0 && (size_t)idx<ti->kin.event->ki->npalettes) {
        kp=ti->kin.event->ki->palettes[idx];
      }
    }
    if (kp || kb->bpp==0) {
      tiger_item_draw_background_bitmap(ti,cr,kb,kp,1);
    }
  }
}

static void tiger_item_get_font_size(const tiger_item *ti,const kate_style *style,double *font_width,double *font_height)
{
  const kate_tracker *kin=&ti->kin;
  if (kin->has.text_size) {
    *font_width=kin->text_size_x;
    *font_height=kin->text_size_y;
  }
  else if (style) {
    if (style->font_width<0) {
      *font_width=-1;
    }
    else {
      tiger_util_remap_metric_double(style->font_width,style->font_metric,ti->frame_width,font_width);
    }
    if (style->font_height<0) {
      *font_height=-1;
    }
    else {
      tiger_util_remap_metric_double(style->font_height,style->font_metric,ti->frame_height,font_height);
    }
  }
  else {
    *font_width=-1;
    *font_height=-1;
  }
}

static void tiger_item_add_attribute(PangoAttrList **attrs,PangoAttribute *attr,int start,int end)
{
  if (*attrs && attr) {
    attr->start_index=start;
    if (end>=0) attr->end_index=end;
    pango_attr_list_insert(*attrs,attr);
  }
}

static void tiger_item_font_mapping_renderer(cairo_t *cr,PangoAttrShape *attr,gboolean do_path,gpointer data)
{
  tiger_bitmap *tb;
  tiger_item *ti;
  const kate_event *ev;
  const kate_info *ki;
  const kate_bitmap *kb;
  const kate_palette *kp;
  int palette_index;
  double scale_x;
  double scale_y;
  double x,y;
  double rx,ry,rw,rh;
  int ret;

  ti=(tiger_item*)data;
  ev=ti->kin.event;
  ki=ev->ki;
  kb=(const kate_bitmap*)attr->data;
  kp=NULL;

  if (kb->bpp!=0) {
    palette_index=kb->palette;
    if (palette_index<0) palette_index=0;
    if (palette_index>=0 && (size_t)palette_index<ki->npalettes) {
      kp=ki->palettes[palette_index];
    }
    if (!kp) {
      tiger_log(tiger_log_warning,"No valid palette, skipping shape\n");
      return;
    }
  }

  ret=tiger_bitmap_cache_get(ti->tbc,kb,kp,&tb);
  if (ret<0) return;

  scale_x=attr->logical_rect.width/(double)(PANGO_SCALE*kb->width);
  scale_y=attr->logical_rect.height/(double)(PANGO_SCALE*kb->height);

  cairo_get_current_point(cr,&x,&y);
  cairo_translate(cr,x,y);

  rx=attr->logical_rect.x*scale_x/(double)PANGO_SCALE;
  ry=attr->logical_rect.y*scale_y/(double)PANGO_SCALE;
  rw=attr->logical_rect.width*scale_x/(double)PANGO_SCALE;
  rh=attr->logical_rect.height*scale_y/(double)PANGO_SCALE;

  cairo_translate(cr,attr->logical_rect.x/(double)PANGO_SCALE,attr->logical_rect.y/(double)PANGO_SCALE);
  cairo_scale(cr,1.0/scale_x,1.0/scale_y);

  cairo_rectangle(cr,rx,ry,rw,rh);
  if (!do_path) {
    tiger_bitmap_render(tb,cr,1,ti->quality);
  }
}

static PangoAttribute *tiger_item_create_shape_attribute_from_bitmap(const kate_bitmap *kb)
{
  PangoRectangle rect;
  rect.x=-kb->x_offset*PANGO_SCALE;
  rect.y=-kb->y_offset*PANGO_SCALE;
  rect.width=kb->width*PANGO_SCALE;
  rect.height=kb->height*PANGO_SCALE;
  return pango_attr_shape_new_with_data(&rect,&rect,kb,NULL,NULL);
}

static void tiger_item_apply_font_mapping(const tiger_item *ti,const kate_font_mapping *kfm,PangoLayout *layout)
{
  const kate_event *ev;
  const char *text;
  size_t len0;
  int ret;
  int c;
  PangoAttrList *attrs=NULL;

  /* check if there is anything to apply */
  if (!kfm) return;

  ev=ti->kin.event;
  text=ev->text;
  len0=ev->len0;

  while (1) {
    const char *start=text;
    ret=kate_text_get_character(ev->text_encoding,&text,&len0);
    if (ret==0) break;
    if (ret<0) {
      tiger_log(tiger_log_warning,"Failed to get character\n");
      break;
    }
    /* look if this glyph has a mapping */
    c=ret;
    ret=kate_font_get_index_from_code_point(kfm,c);
    if (ret>=0) {
      /* we have found a bitmap */
      size_t bitmap_index=(size_t)ret;
      if (bitmap_index<ev->ki->nbitmaps) {
        /* character baseline is bottom - better way to tell ? */
        const kate_bitmap *kb=ev->ki->bitmaps[bitmap_index];
        PangoAttribute *attr=tiger_item_create_shape_attribute_from_bitmap(kb);
        /* create attribute list if there isn't one already */
        if (!attrs) {
          attrs=pango_layout_get_attributes(layout);
          if (attrs) {
            pango_attr_list_ref(attrs);
          }
          else {
            attrs=pango_attr_list_new();
          }
        }
        tiger_item_add_attribute(&attrs,attr,start-ev->text,text-ev->text);
      }
      else {
        tiger_log(tiger_log_warning,"Bitmap index out of range\n");
      }
    }
    else if (ret!=KATE_E_NOT_FOUND) {
      tiger_log(tiger_log_warning,"Error looking for bitmap for code point 0x%08x: %d\n",c,ret);
    }
  }

  /* if we found at least one mapped glyph, setup a custom renderer */
  if (attrs) {
    pango_cairo_context_set_shape_renderer(pango_layout_get_context(layout),&tiger_item_font_mapping_renderer,ti,NULL);

    /* the attributes list is filled, setup the layout with it */
    pango_layout_set_attributes(layout,attrs);
    pango_attr_list_unref(attrs);
  }
}

static double tiger_item_set_font_style(const tiger_item *ti,const kate_style *ks,PangoLayout *layout,int start,int end)
{
  PangoAttrList *attrs;
  PangoFontDescription *font;
  double font_width,font_height;

  /* find out font size */
  tiger_item_get_font_size(ti,ks,&font_width,&font_height);

  /* negative values mean unset, follow the other one */
  if (font_width<0) font_width=font_height;
  if (font_height<0) font_height=font_width;

  /* nothing to draw for less than 1 point width or height */
  if (font_width>=0.0 && font_width<1.0) return 1.0;
  if (font_height>=0.0 && font_height<1.0) return 1.0;

  if (ti->defaults && ti->defaults->font_desc) {
    font=pango_font_description_copy(ti->defaults->font_desc);
  }
  else {
    font=pango_font_description_new();
  }
  attrs=pango_layout_get_attributes(layout);
  if (attrs) {
    pango_attr_list_ref(attrs);
  }
  else {
    attrs=pango_attr_list_new();
  }

  if (ks && ks->font) {
    pango_font_description_set_family(font,ks->font);
  }

  if (ks) {
    const kate_color *kc=&ks->text_color;
    if (ks->bold) {
      pango_font_description_set_weight(font,PANGO_WEIGHT_BOLD);
    }
    if (ks->italics) {
      pango_font_description_set_style(font,PANGO_STYLE_ITALIC);
    }
    if (ks->underline) {
      tiger_item_add_attribute(&attrs,pango_attr_underline_new(PANGO_UNDERLINE_SINGLE),start,end);
    }
    if (ks->strike) {
      tiger_item_add_attribute(&attrs,pango_attr_strikethrough_new(1),start,end);
    }
    if (tiger_item_has_flag(ti,TIGER_FLAG_SWAP_RGB))
      tiger_item_add_attribute(&attrs,pango_attr_foreground_new(kc->b*257,kc->g*257,kc->r*257),start,end);
    else
      tiger_item_add_attribute(&attrs,pango_attr_foreground_new(kc->r*257,kc->g*257,kc->b*257),start,end);
  }

  if (font_height>0) {
    pango_font_description_set_absolute_size(font,font_height*PANGO_SCALE);
  }
  else {
    /* if we have no font height, nor the font description has a default one,
     try to work out a sensible size based on frame size */
    if (!(pango_font_description_get_set_fields(font)&PANGO_FONT_MASK_SIZE)) {
      double size=ti->frame_width/32.0;
      /* within limits */
      if (size<12.0) size=12.0;
      pango_font_description_set_absolute_size(font,size*PANGO_SCALE);
      font_width=size;
      font_height=size;
    }
  }

  /* add the filled out font description to the attributes list */
  tiger_item_add_attribute(&attrs,pango_attr_font_desc_new(font),start,end);

  /* the attributes list is filled, setup the layout with it */
  pango_layout_set_attributes(layout,attrs);
  pango_attr_list_unref(attrs);

  pango_font_description_free(font);

  if (font_height>0 && font_width>0 && font_height!=font_width) {
    return font_width/font_height;
  }
  else {
    return 1.0;
  }
}

static void map_point(const tiger_item *ti,double x0,double x1,double baseline,double *x,double *y)
{
  kate_float kx,ky;
  const kate_tracker *kin=&ti->kin;
  double t=(*x-x0)/(x1-x0);
  t=kin->path_start+t*(kin->path_end-kin->path_start);
  kate_tracker_update_property_at_duration(kin,1.0,t,kate_motion_semantics_text_path,&kx,&ky);
  if (1) {
    /* we rotate the y coord around x */
    kate_float klx,kly,krx,kry;
    double dx,dy;
    double angle;
    double delta=0.01*(kin->path_end-kin->path_start);
    double tl=t-delta;
    double tr=t+delta;
    if (tl<kin->path_start) tl=kin->path_start;
    if (tr>kin->path_end) tr=kin->path_end;
    kate_tracker_update_property_at_duration(kin,1.0,tl,kate_motion_semantics_text_path,&klx,&kly);
    kate_tracker_update_property_at_duration(kin,1.0,tr,kate_motion_semantics_text_path,&krx,&kry);
    dx=krx-klx;
    dy=kry-kly;
    angle=atan2(dy,dx);
    dx=cos(angle)*(*y-baseline);
    dy=sin(angle)*(*y-baseline);
    *x=kx-dy;
    *y=ky+dx;
  }
  else {
    /* upright */
    *x=kx;
    *y+=ky;
  }
}

static void tiger_item_x_path_extents(const cairo_path_t *path,double *px0,double *px1)
{
  int i,n;
  const cairo_path_data_t *data=path->data;

  *px0=FLT_MAX;
  *px1=-FLT_MAX;

  for (n=0,i=0;i<path->num_data;i+=data[i].header.length,++n) {
    switch (data[i].header.type) {
      case CAIRO_PATH_MOVE_TO:
      case CAIRO_PATH_LINE_TO:
        if (data[i+1].point.x<*px0) *px0=data[i+1].point.x;
        if (data[i+1].point.x>*px1) *px1=data[i+1].point.x;
        break;
      case CAIRO_PATH_CURVE_TO:
        if (data[i+3].point.x<*px0) *px0=data[i+3].point.x;
        if (data[i+3].point.x>*px1) *px1=data[i+3].point.x;
        break;
      case CAIRO_PATH_CLOSE_PATH:
        break;
    }
  }
}

static void tiger_item_draw_mapped_path(const tiger_item *ti,cairo_t *cr,const cairo_path_t *path,double x0,double x1,double baseline)
{
  int i,n;
  double x,y;
  double cp1x,cp1y;
  double cp2x,cp2y;
  const cairo_path_data_t *data=path->data;

  for (n=0,i=0;i<path->num_data;i+=data[i].header.length,++n) {
    switch (data[i].header.type) {
      case CAIRO_PATH_MOVE_TO:
        x=data[i+1].point.x;
        y=data[i+1].point.y;
        map_point(ti,x0,x1,baseline,&x,&y);
        cairo_move_to(cr,x,y);
        TIGER_CHECK_CAIRO_STATUS(cr);
        break;
      case CAIRO_PATH_LINE_TO:
        x=data[i+1].point.x;
        y=data[i+1].point.y;
        map_point(ti,x0,x1,baseline,&x,&y);
        cairo_line_to(cr,x,y);
        TIGER_CHECK_CAIRO_STATUS(cr);
        break;
      case CAIRO_PATH_CURVE_TO:
        x=data[i+1].point.x;
        y=data[i+1].point.y;
        map_point(ti,x0,x1,baseline,&x,&y);
        cp1x=data[i+2].point.x;
        cp1y=data[i+2].point.y;
        map_point(ti,x0,x1,baseline,&cp1x,&cp1y);
        cp2x=data[i+3].point.x;
        cp2y=data[i+3].point.y;
        map_point(ti,x0,x1,baseline,&cp2x,&cp2y);
        cairo_curve_to(cr,cp1x,cp1y,cp2x,cp2y,x,y);
        TIGER_CHECK_CAIRO_STATUS(cr);
        break;
      case CAIRO_PATH_CLOSE_PATH:
        cairo_close_path(cr);
        TIGER_CHECK_CAIRO_STATUS(cr);
        break;
      default:
        tiger_log(tiger_log_error,"Unknown path element type: %d\n",data[i].header.type);
        break;
    }
  }
}

static void tiger_item_map_path(const tiger_item *ti,cairo_t *cr,double baseline)
{
  cairo_path_t *path=cairo_copy_path(cr);
  double x0,x1;

  TIGER_CHECK_CAIRO_STATUS(cr);
  if (path->num_data==0) {
    cairo_path_destroy(path);
    return;
  }

  /* we map the horizontal axis of the path to 0-1, then get a point on the spline at that t */
  tiger_item_x_path_extents(path,&x0,&x1);

  cairo_new_path(cr);
  TIGER_CHECK_CAIRO_STATUS(cr);

  tiger_item_draw_mapped_path(ti,cr,path,x0,x1,baseline);

  cairo_path_destroy(path);
  TIGER_CHECK_CAIRO_STATUS(cr);
}

static void tiger_item_get_text_position_h(const tiger_item *ti,double line_w,double layout_h,double *x,double *y)
{
  const kate_tracker *kin=&ti->kin;
  if (kin->has.path) {
    *x=0.0;
    *y=0.0;
  }
  else if (kin->has.text_pos) {
    *x=kin->text_x;
    *y=kin->text_y;
  }
  else if (kin->has.text_alignment_int) {
    *x=tiger_item_int_align_helper(ti->mregion.x0,ti->mregion.x1,line_w,kin->text_halign);
    *y=tiger_item_int_align_helper(ti->mregion.y0,ti->mregion.y1,layout_h,kin->text_valign);
  }
  else if (kin->has.text_alignment_ext) {
    *x=tiger_item_ext_align_helper(ti->mregion.x0,ti->mregion.x1,line_w,kin->text_halign);
    *y=tiger_item_ext_align_helper(ti->mregion.y0,ti->mregion.y1,layout_h,kin->text_valign);
  }
  else {
    /* no position, nor alignment, default to center in the region */
    *x=tiger_item_int_align_helper(ti->mregion.x0,ti->mregion.x1,line_w,0);
    *y=tiger_item_int_align_helper(ti->mregion.y0,ti->mregion.y1,layout_h,0);
  }

  /* override just x for justification */
  if (kin->event->style && kin->event->style->justify) {
    *x=ti->mregion.x0;
  }
}

static void tiger_item_get_text_position_v(const tiger_item *ti,double line_w,double layout_h,double *x,double *y)
{
  const kate_tracker *kin=&ti->kin;
  if (kin->has.path) {
    *x=0.0;
    *y=0.0;
  }
  else if (kin->has.text_pos) {
    *x=kin->text_x;
    *y=kin->text_y;
  }
  else if (kin->has.text_alignment_int) {
    /* layout_h and line_w are swapped */
    *x=tiger_item_int_align_helper(ti->mregion.x0,ti->mregion.x1,layout_h,kin->text_halign);
    *y=tiger_item_int_align_helper(ti->mregion.y0,ti->mregion.y1,line_w,kin->text_valign);
  }
  else if (kin->has.text_alignment_ext) {
    /* layout_h and line_w are swapped */
    *x=tiger_item_ext_align_helper(ti->mregion.x0,ti->mregion.x1,layout_h,kin->text_halign);
    *y=tiger_item_ext_align_helper(ti->mregion.y0,ti->mregion.y1,line_w,kin->text_valign);
  }
  else {
    /* no position, nor alignment, default to center in the region */
    /* layout_h and line_w are swapped */
    *x=tiger_item_int_align_helper(ti->mregion.x0,ti->mregion.x1,layout_h,0);
    *y=tiger_item_int_align_helper(ti->mregion.y0,ti->mregion.y1,line_w,0);
  }

  /* override just y for justification */
  if (kin->event->style && kin->event->style->justify) {
    *y=ti->mregion.y0;
  }
}

static void tiger_item_get_text_position(const tiger_item *ti,double line_w,double layout_h,double *x,double *y)
{
  if (ti->vertical)
    tiger_item_get_text_position_v(ti,line_w,layout_h,x,y);
  else
    tiger_item_get_text_position_h(ti,line_w,layout_h,x,y);
}

static int tiger_item_glyph_to_byte(kate_text_encoding text_encoding,const char *text,size_t len0,double glyph)
{
  size_t original_len0=len0;
  int c;

  if (glyph<0) return 0;
  while (glyph>0) {
    int ret=kate_text_get_character(text_encoding,&text,&len0);
    if (ret<0) return 0;
    c=ret;
    if (c==0) {
      /* we've reached the end of the string, return the current offset */
      break;
    }
    glyph-=1.0f;
  }
  return original_len0-len0;
}

static int tiger_item_get_style_change(const tiger_item *ti,int *byte_offset)
{
  const kate_tracker *kin=&ti->kin;
  const kate_event *ev=kin->event;
  int n;

  /* if no secondary style, we can't change styles */
  if (!ev->secondary_style)
    return 0;

  /* style change happens for the first (if any) glyph pointer that is present
     and does not have bitmap information */
  for (n=0;n<4;++n) {
    if (kin->has.glyph_pointer&(1<<n) && !(kin->has.glyph_pointer_bitmap&(1<<n))) {
      double glyph_pointer=kin->glyph_pointer[n];
      int cutoff=tiger_item_glyph_to_byte(ev->text_encoding,ev->text,ev->len,glyph_pointer);
      *byte_offset=cutoff;
      return 1;
    }
  }

  /* single style */
  return 0;
}

static int tiger_item_init_context(tiger_item *ti)
{
  if (!ti->font_map) {
    ti->font_map=pango_cairo_font_map_new();
  }
  if (ti->font_map) {
    ti->context=pango_cairo_font_map_create_context((PangoCairoFontMap*)ti->font_map);
  }
  return 0;
}

static PangoLayout* tiger_item_create_layout(tiger_item *ti,cairo_t *cr)
{
  PangoLayout *layout;

  if (!ti->context) {
    /* lazy construction, we might not need a context for all events */
    tiger_item_init_context(ti);
    if (!ti->context) {
      tiger_log(tiger_log_warning,"Failed to create Pango context\n");
      return NULL;
    }
  }

  layout=pango_layout_new(ti->context);
  if (layout) {
    pango_cairo_update_context(cr,ti->context);
  }
  return layout;
}

static void tiger_item_configure_layout(const tiger_item *ti,PangoLayout *layout)
{
  const kate_style *ks;
  kate_wrap_mode wrap_mode;
  PangoWrapMode pango_wrap_mode;
  PangoDirection pango_direction;
  PangoContext *context=pango_layout_get_context(layout);
  int context_changed=0;

  /* find out which wrap mode to use */
  wrap_mode=kate_wrap_word;
  ks=ti->kin.event->style;
  if (ks) wrap_mode=ks->wrap_mode;

  /* tell Pango how to wrap */
  if (wrap_mode!=kate_wrap_none) {
    pango_layout_set_width(layout,(ti->mregion.x1-ti->mregion.x0)*PANGO_SCALE);
    switch (wrap_mode) {
      case kate_wrap_word:
        pango_wrap_mode=PANGO_WRAP_WORD;
        break;
      default:
        tiger_log(tiger_log_warning,"Unknown wrap mode: wrapping at word boundaries\n");
        pango_wrap_mode=PANGO_WRAP_WORD;
        break;
    }
    pango_layout_set_wrap(layout,pango_wrap_mode);
  }

  /* configure the context for this layout */
  if (ti->kin.event->language) {
    PangoLanguage *language=pango_language_from_string(ti->kin.event->language);
    if (language) {
      pango_context_set_language(context,language);
      context_changed=1;
    }
  }

  pango_direction=PANGO_DIRECTION_NEUTRAL;
  switch (ti->kin.event->text_directionality) {
    case kate_l2r_t2b: pango_direction=PANGO_DIRECTION_WEAK_LTR; break;
    case kate_r2l_t2b: pango_direction=PANGO_DIRECTION_WEAK_RTL; break;
    case kate_t2b_r2l: pango_direction=PANGO_DIRECTION_WEAK_RTL; break;
    case kate_t2b_l2r: pango_direction=PANGO_DIRECTION_WEAK_LTR; break;
    default:
      /* unknown, leave as neutral */
      tiger_log(tiger_log_warning,"unknown text directionality: %d\n",ti->kin.event->text_directionality);
      break;
  }
  if (pango_direction!=pango_context_get_base_dir(context)) {
    pango_context_set_base_dir(context,pango_direction);
    context_changed=1;
  }

  if (ks && ks->justify) {
    pango_layout_set_justify(layout,1);
  }
  if (ti->vertical) {
    pango_context_set_base_gravity(context,PANGO_GRAVITY_AUTO);
    context_changed=1;
  }

  if (context_changed) {
    pango_layout_context_changed(layout);
  }
}

static void tiger_item_draw_background_slice(tiger_item *ti,cairo_t *cr,double x,double y,double w,double h,const tiger_color *tc)
{
  /* in order to avoid slight gaps between the slices, or slight overlaps (which would be
     as bad with an alpha background as a slight line with more opacity would show), we
     quantize the coordinates on the device grid, and we always use the floor of those
     values so rectangles should match perfectly */
  double sx0=x;
  double sy0=y;
  double sx1=x+w;
  double sy1=y+h;

  cairo_save(cr);

  cairo_user_to_device(cr,&sx0,&sy0);
  sx0=floor(sx0);
  sy0=floor(sy0);
  cairo_device_to_user(cr,&sx0,&sy0);
  TIGER_CHECK_CAIRO_STATUS(cr);

  cairo_user_to_device(cr,&sx1,&sy1);
  sx1=floor(sx1);
  sy1=floor(sy1);
  cairo_device_to_user(cr,&sx1,&sy1);
  TIGER_CHECK_CAIRO_STATUS(cr);

  tiger_pixel_format_set_source_color(cr,tiger_item_has_flag(ti,TIGER_FLAG_SWAP_RGB),tc);
  TIGER_CHECK_CAIRO_STATUS(cr);
  cairo_rectangle(cr,sx0,sy0,sx1-sx0,sy1-sy0);
  TIGER_CHECK_CAIRO_STATUS(cr);
  tiger_item_extend_rectangle_with_fill(&ti->cached_render_bounds,cr);
  TIGER_CHECK_CAIRO_STATUS(cr);
  cairo_fill(cr);
  TIGER_CHECK_CAIRO_STATUS(cr);

  cairo_restore(cr);
}

static void tiger_item_draw_marker(tiger_item *ti,cairo_t *cr,const kate_bitmap *kb,double x,double y)
{
  int ret;
  tiger_bitmap *frame=NULL;
  const kate_palette *kp=NULL;

  cairo_save(cr);

  if (kb) {
    cairo_translate(cr,x-kb->x_offset,y-kb->y_offset);
    if (kb->bpp!=0) {
      size_t palette=(kb->palette<0)?0:kb->palette;
      if (palette<ti->kin.ki->npalettes) {
        kp=ti->kin.ki->palettes[palette];
      }
    }
    ret=tiger_bitmap_cache_get(ti->tbc,kb,kp,&frame);
    if (ret<0) {
      tiger_log(tiger_log_warning,"No palette for marker frame\n");
    }
    else {
      tiger_bitmap_render(frame,cr,0,ti->quality);
    }
  }
  else {
    tiger_log(tiger_log_warning,"No marker frame, drawing white dot\n");
    cairo_set_source_rgb(cr,1.0,1.0,1.0);
    TIGER_CHECK_CAIRO_STATUS(cr);
    cairo_move_to(cr,x,y);
    cairo_rel_line_to(cr,0.0,0.0);
    cairo_set_line_width(cr,5);
    cairo_set_line_cap(cr,CAIRO_LINE_CAP_ROUND);
    cairo_stroke(cr);
  }

  cairo_restore(cr);
}

static void tiger_item_draw_shadow(tiger_item *ti,cairo_t *cr,cairo_pattern_t *cp,double xsz,double ysz)
{
  tiger_rectangle bounds;

  ti->cached_render_bounds.x1+=xsz;
  ti->cached_render_bounds.y1+=ysz;
  tiger_rectangle_snap(&ti->cached_render_bounds,&bounds);

  cairo_save(cr);
  cairo_rectangle(cr,bounds.x0+ti->cached_render_dx,bounds.y0+ti->cached_render_dy,bounds.x1-bounds.x0,bounds.y1-bounds.y0);
  cairo_clip(cr);
  cairo_translate(cr,xsz,ysz);
  cairo_mask(cr,cp);
  cairo_restore(cr);
}

static void tiger_item_draw_outline(tiger_item *ti,cairo_t *cr,cairo_pattern_t *cp,double xsz,double ysz)
{
  tiger_rectangle bounds;

  tiger_rectangle_grow(&ti->cached_render_bounds,xsz,ysz);
  tiger_rectangle_snap(&ti->cached_render_bounds,&bounds);
  cairo_rectangle(cr,bounds.x0+ti->cached_render_dx,bounds.y0+ti->cached_render_dy,bounds.x1-bounds.x0,bounds.y1-bounds.y0);
  cairo_clip(cr);
  cairo_save(cr); cairo_translate(cr,xsz,ysz); cairo_mask(cr,cp); cairo_restore(cr);
  cairo_save(cr); cairo_translate(cr,xsz,-ysz); cairo_mask(cr,cp); cairo_restore(cr);
  cairo_save(cr); cairo_translate(cr,-xsz,ysz); cairo_mask(cr,cp); cairo_restore(cr);
  cairo_save(cr); cairo_translate(cr,-xsz,-ysz); cairo_mask(cr,cp); cairo_restore(cr);
}

static void tiger_item_draw_half_outline(tiger_item *ti,cairo_t *cr,cairo_pattern_t *cp,double xsz,double ysz)
{
  tiger_rectangle bounds;

  tiger_rectangle_grow(&ti->cached_render_bounds,xsz,ysz);
  tiger_rectangle_snap(&ti->cached_render_bounds,&bounds);
  cairo_rectangle(cr,bounds.x0+ti->cached_render_dx,bounds.y0+ti->cached_render_dy,bounds.x1-bounds.x0,bounds.y1-bounds.y0);
  cairo_clip(cr);
  cairo_save(cr); cairo_translate(cr,xsz,ysz); cairo_mask(cr,cp); cairo_restore(cr);
  cairo_save(cr); cairo_translate(cr,-xsz,-ysz); cairo_mask(cr,cp); cairo_restore(cr);
}

static inline double clamp01(double value) __attribute__((const));
static inline double clamp01(double value)
{
  if (value<0.0) return 0.0;
  if (value>1.0) return 1.0;
  return value;
}

static void tiger_item_add_font_effect(tiger_item *ti,cairo_t *cr,cairo_pattern_t *cp,double line_h,double font_width_ratio)
{
  double quality=ti->quality;
  tiger_font_effect font_effect=ti->defaults->font_effect;
  double strength=clamp01(ti->defaults->font_effect_strength);
  double ysz=line_h*strength*0.05;
  double xsz=ysz*font_width_ratio;

  cairo_save(cr);
  cairo_set_source_rgb(cr,0,0,0);

  switch (font_effect) {
  case tiger_font_plain:
    break;
  case tiger_font_outline:
      if (quality<0.0 || quality>=0.5) {
        tiger_item_draw_outline(ti,cr,cp,xsz,ysz);
      }
      else if (quality>=0.1) {
        tiger_item_draw_half_outline(ti,cr,cp,xsz,ysz);
      }
      else {
        tiger_item_draw_shadow(ti,cr,cp,xsz,ysz);
      }
      break;
  case tiger_font_shadow:
      tiger_item_draw_shadow(ti,cr,cp,xsz,ysz);
      break;
  default:
      tiger_log(tiger_log_warning,"Unknown font effect: %d\n",font_effect);
      break;
  }

  cairo_restore(cr);
}

static void tiger_item_draw_text(tiger_item *ti,cairo_t *cr)
{
  const kate_tracker *kin=&ti->kin;
  const kate_event *ev=kin->event;
  const char *text=ev->text;
  const size_t len=ev->len;
  const kate_style *style=ev->style;
  const kate_style *style2=ev->secondary_style;
  int layout_w,layout_h;
  double x,y;
  double font_width_ratio;
  int pango_baseline;
  double baseline;
  double line_w,line_h;
  double path_baseline;
  PangoLayout *layout;
  PangoRectangle ink,logical;
  PangoLayoutIter *iter;
  int byte_offset;
  int byte_offsets[4];
  int marker_lines[4];
  double marker_x[4];
  double marker_y[4];
  int line_index;
  int n;
  const tiger_color *background_fill_color;
  double previous_last_fill_bottom=-FLT_MAX;
  double leftover_border_height=0;
  double last_border_x0=0;
  double last_border_x1=0;
  int has_alpha=0;
  int has_font_effect=0;

  if (!text || len==0) return;
  if (ev->text_encoding!=kate_utf8) {
    tiger_log(tiger_log_warning,"unknown text encoding: %d\n",ev->text_encoding);
    return;
  }

  // TODO: we might need to draw with secondary text color!
  // or have a marker to draw
  if (ti->text_color.a<=0.001+ti->quality/20) return;

  layout=tiger_item_create_layout(ti,cr);
  if (!layout) {
    tiger_log(tiger_log_error,"Failed to create layout\n");
    return;
  }

  tiger_item_configure_layout(ti,layout);

  tiger_item_apply_font_mapping(ti,ti->kin.event->font_mapping,layout);

  if (tiger_item_get_style_change(ti,&byte_offset)>0) {
    font_width_ratio=tiger_item_set_font_style(ti,style2,layout,0,byte_offset);
    font_width_ratio=tiger_item_set_font_style(ti,style,layout,byte_offset,-1);
  }
  else {
    tiger_pixel_format_set_source_color(cr,tiger_item_has_flag(ti,TIGER_FLAG_SWAP_RGB),&ti->text_color);
    font_width_ratio=tiger_item_set_font_style(ti,ti->kin.event->style,layout,0,-1);
  }
  TIGER_CHECK_CAIRO_STATUS(cr);

  switch (ev->text_markup_type) {
    default: /* default to a sensible setup */
      tiger_log(tiger_log_warning,"unknown markup type: %d\n",ev->text_markup_type);
      /* fall through */
    case kate_markup_none:
      pango_layout_set_text(layout,text,len);
      break;
    case kate_markup_simple:
      pango_layout_set_markup(layout,text,len);
      break;
  }

  TIGER_CHECK_CAIRO_STATUS(cr);
  pango_layout_get_extents(layout,&ink,&logical);
  pango_layout_get_pixel_size(layout,&layout_w,&layout_h);

  /* for vertical text, we want Pango to assign gravity to each script in the text,
     and for this to work we need the text to be rotated first, but we want to rotate
     late so we don't mess up the x/y translations, so we rotate now, tell Pango to
     assign gravity, and rotate back. The final rotation will be done later.
     A better way to do this would be nice, if there is one. */
  if (ti->vertical) {
    cairo_rotate(cr,M_PI/2.0);
    pango_layout_context_changed(layout);
    pango_cairo_update_layout(cr,layout);
    cairo_rotate(cr,-M_PI/2.0);
  }

  /* if we do not have a region, but have a default background color, then we
     render here a rectangle which encompasses the layout */
  background_fill_color=NULL;
  if (!ti->explicit_region) {
    const tiger_color *tc=&ti->defaults->background_fill_color;
    if (tc->a>0.0) {
      background_fill_color=tc;
    }
  }

  /* work out in which lines the markers are located, if any */
  for (n=0;n<4;++n) {
    marker_lines[n]=-1;
    if (kin->has.glyph_pointer&(1<<n) && (kin->has.glyph_pointer_bitmap&(1<<n))) {
      byte_offsets[n]=tiger_item_glyph_to_byte(kin->event->text_encoding,kin->event->text,kin->event->len,kin->glyph_pointer[n]);
      if (byte_offsets[n]>=0) {
        int dummy_x;
        pango_layout_index_to_line_x(layout,byte_offsets[n],0,&marker_lines[n],&dummy_x);
        tiger_log(tiger_log_debug,"marker %d is on line %d (%d lines)\n",n,marker_lines[n],pango_layout_get_line_count(layout));
      }
    }
  }

  iter=pango_layout_get_iter(layout);

  path_baseline=pango_layout_iter_get_baseline(iter)/(double)PANGO_SCALE;

  has_alpha=(ti->text_color.a<0.949+ti->quality/20.0);
  has_font_effect=(ti->defaults->font_effect!=tiger_font_plain);
  if (has_font_effect || has_alpha) {
    cairo_save(cr);
    cairo_push_group(cr);
  }

  line_index=0;
  do {
    PangoLayoutLine *line=pango_layout_iter_get_line_readonly(iter);
    pango_layout_iter_get_line_extents(iter,&ink,&logical);
    line_w=logical.width*font_width_ratio/(double)PANGO_SCALE;
    line_h=logical.height/(double)PANGO_SCALE;
    pango_baseline=pango_layout_iter_get_baseline(iter);
    baseline=pango_baseline/(double)PANGO_SCALE;

    tiger_item_get_text_position(ti,line_w,layout_h,&x,&y);

    cairo_save(cr);
    TIGER_CHECK_CAIRO_STATUS(cr);

    if (!kin->has.path) {
      cairo_translate(cr,x,y);
      if (ti->vertical) {
        cairo_translate(cr,layout_h-baseline,0);
        cairo_rotate(cr,M_PI/2.0);
      }
      else {
        cairo_translate(cr,0,baseline);
      }
      TIGER_CHECK_CAIRO_STATUS(cr);
    }

    // TODO: can't work with attributes
    cairo_scale(cr,font_width_ratio,1.0);
    TIGER_CHECK_CAIRO_STATUS(cr);

    if (kin->has.path) {
      pango_cairo_layout_line_path(cr,line);
      tiger_item_map_path(ti,cr,path_baseline);
      path_baseline-=line_h;
      tiger_item_extend_rectangle_with_fill(&ti->cached_render_bounds,cr);
      cairo_fill(cr);
    }
    else {
      if (background_fill_color) {
        /* compute coordinates to fill - add a border to look better */
        double border=ink.height/PANGO_SCALE*0.35;
        double fill_x=ink.x/PANGO_SCALE-border;
        double fill_y=ink.y/PANGO_SCALE-baseline-border;
        double fill_w=ink.width/PANGO_SCALE+2*border;
        double fill_h=ink.height/PANGO_SCALE+2*border;
        double lowest_ink=(ink.y+ink.height-pango_baseline+PANGO_SCALE-1)/(double)PANGO_SCALE;

        double offset=y+baseline;

        /*
           we want to draw a border below the text, but we will want to draw a border
           above the next line too; if the borders would overlap, a background color
           with non opaque alpha would show the overlap with a different color, so we
           only draw up to the lowest ink, but remember the desired bottom of the
           border, and we'll fill it later, with the maximum horizontal extent for
           this line and the next, so the two borders are merged and drawn with a
           single call.
        */

        /* make sure we can't draw over the previous line, we'd overwrite the bottom of letters with low descent */
        if (fill_y+offset<previous_last_fill_bottom) {
          double dy=previous_last_fill_bottom-(fill_y+offset);
          fill_y+=dy;
          fill_h-=dy;
        }

        /* fill the remainder of the previous line, if any*/
        if (leftover_border_height!=0) {
          double x0=MIN(fill_x,last_border_x0-x);
          double x1=MAX(fill_x+fill_w,last_border_x1-x);
          tiger_item_draw_background_slice(ti,cr,x0,previous_last_fill_bottom-offset-leftover_border_height,x1-x0,leftover_border_height,background_fill_color);
        }

        /* fill the box */
        if (pango_layout_iter_at_last_line(iter)) {
          /* if we're at the last line, we draw down to the bottom as we can't have a larger lower line
             forcing us to draw wider here to accomodate its top part's margin - this will allow us to
             avoid having a last slice at the end */
          tiger_item_draw_background_slice(ti,cr,fill_x,fill_y,fill_w,fill_h,background_fill_color);
        }
        else {
          /* if we're not at the last line, only draw to the bottom ink, as a wider line below will need
             a margin and we'd have to draw a complicated shape - doing it this way keeps us to rectangles */
          tiger_item_draw_background_slice(ti,cr,fill_x,fill_y,fill_w,lowest_ink-fill_y,background_fill_color);
        }

        /* remember the lowest point of the fill */
        previous_last_fill_bottom=fill_y+fill_h+offset;
        leftover_border_height=fill_y+fill_h-lowest_ink;
        last_border_x0=fill_x+x;
        last_border_x1=fill_x+fill_w+x;
      }

      /* would work with the above path/fill method, but it is faster that way */
      tiger_item_extend_rectangle_with_pango_line_extents(&ti->cached_render_bounds,line,font_width_ratio,cr,ti->vertical);
      pango_cairo_show_layout_line(cr,line);
    }

    cairo_restore(cr);
    TIGER_CHECK_CAIRO_STATUS(cr);

    /* if there are markers in this line, calculate their positions now */
    for (n=0;n<4;++n) {
      if (marker_lines[n]==line_index) {
        int x_pos;
        pango_layout_line_index_to_x(line,kin->glyph_pointer[n],0,&x_pos);
        marker_x[n]=x+x_pos/(double)PANGO_SCALE;
        marker_y[n]=y+baseline+kin->glyph_height[n];
      }
    }

    ++line_index;
  } while (pango_layout_iter_next_line(iter));

  if (has_font_effect || has_alpha) {
    cairo_pattern_t *cp=cairo_pop_group(cr);
    tiger_rectangle bounds;

    tiger_rectangle_snap(&ti->cached_render_bounds,&bounds);

    if (has_font_effect) {
      /* if we have alpha, we have to draw the ouline with full opacity below the text
         group, and make this part of the original group */
      if (has_alpha) {
        cairo_push_group(cr);
      }

      tiger_item_add_font_effect(ti,cr,cp,line_h,font_width_ratio);

      if (has_alpha) {
        /* redraw the original text on top */
        cairo_save(cr);
        cairo_set_source(cr,cp);
        cairo_rectangle(cr,bounds.x0+ti->cached_render_dx,bounds.y0+ti->cached_render_dy,bounds.x1-bounds.x0,bounds.y1-bounds.y0);
        cairo_clip(cr);
        cairo_paint(cr);
        /* we now have a group with the text and its effect, with full opacity, replace the text-only group with it */
        cairo_restore(cr);
        cp=cairo_pop_group(cr);
      }
    }

    tiger_rectangle_snap(&ti->cached_render_bounds,&bounds);

    cairo_set_source(cr,cp);
    cairo_rectangle(cr,bounds.x0+ti->cached_render_dx,bounds.y0+ti->cached_render_dy,bounds.x1-bounds.x0,bounds.y1-bounds.y0);
    cairo_clip(cr);
    if (has_alpha) {
      /* performance killer */
      cairo_paint_with_alpha(cr,ti->text_color.a);
    }
    else {
      cairo_paint(cr);
    }

    cairo_pattern_destroy(cp);
    cairo_restore(cr);
  }

  /* render any markers now that positions are calculated */
  for (n=0;n<4;++n) {
    if (marker_lines[n]>=0) {
       tiger_item_draw_marker(ti,cr,kin->glyph_pointer_bitmap[n],marker_x[n],marker_y[n]);
    }
  }

  pango_layout_iter_free(iter);

  g_object_unref(layout);
}

static void tiger_item_draw_markers(tiger_item *ti,cairo_t *cr)
{
  const kate_tracker *kin=&ti->kin;
  size_t n;

  for (n=0;n<4;++n) {
    if (kin->has.marker_pos&(1<<n)) {
      const kate_bitmap *kb=NULL;
      if (kin->has.marker_bitmap&(1<<n)) kb=kin->marker_bitmap[n];
      tiger_item_draw_marker(ti,cr,kb,kin->marker_x[n],kin->marker_y[n]);
    }
  }
}

static void tiger_item_add_margins(double *low_x,double *high_x,kate_space_metric metric,double low_margin,double high_margin,double sz)
{
  double lm,hm;
  tiger_util_remap_metric_double(low_margin,metric,sz,&lm);
  tiger_util_remap_metric_double(high_margin,metric,sz,&hm);
  *low_x+=lm;
  *high_x-=hm;
}

static void tiger_item_setup_region(tiger_item *ti,const kate_tracker *kin)
{
  double rw,rh;
  const kate_style *style=kin->event->style;

  if (kin->has.region) {
    /* motions take priority */
    ti->region.x0=kin->region_x;
    ti->region.y0=kin->region_y;
    ti->region.x1=kin->region_x+kin->region_w;
    ti->region.y1=kin->region_y+kin->region_h;
    ti->explicit_region=1;
  }
  else if (kin->event->region) {
    /* if no motion, a region is used */
    const kate_region *kr=kin->event->region;
    tiger_util_remap_metric_double(kr->x,kr->metric,ti->frame_width,&ti->region.x0);
    tiger_util_remap_metric_double(kr->y,kr->metric,ti->frame_height,&ti->region.y0);
    tiger_util_remap_metric_double(kr->x+kr->w,kr->metric,ti->frame_width,&ti->region.x1);
    tiger_util_remap_metric_double(kr->y+kr->h,kr->metric,ti->frame_height,&ti->region.y1);
    ti->explicit_region=1;
  }
  else {
    /* no region defined, use a sensible default */
    /* TODO: vertical text should default to a vertical column on one side */
    ti->region.x0=ti->frame_width*10/100;
    ti->region.y0=ti->frame_height*80/100;
    ti->region.x1=ti->frame_width*90/100;
    ti->region.y1=ti->frame_height*90/100;
    ti->explicit_region=0;
  }

  /* add margins */
  ti->mregion=ti->region;
  rw=ti->region.x1-ti->region.x0;
  rh=ti->region.y1-ti->region.y0;

  if (kin->has.hmargins) {
    /* animated margins */
    tiger_item_add_margins(&ti->mregion.x0,&ti->mregion.x1,kate_pixel,kin->left_margin,kin->right_margin,rw);
  }
  else if (style) {
    /* from the style */
    tiger_item_add_margins(&ti->mregion.x0,&ti->mregion.x1,style->margin_metric,style->left_margin,style->right_margin,rw);
  }

  if (kin->has.vmargins) {
    /* animated margins */
    tiger_item_add_margins(&ti->mregion.y0,&ti->mregion.y1,kate_pixel,kin->top_margin,kin->bottom_margin,rh);
  }
  else if (style) {
    /* from the style */
    tiger_item_add_margins(&ti->mregion.y0,&ti->mregion.y1,style->margin_metric,style->top_margin,style->bottom_margin,rh);
  }
}

static void tiger_item_set_clip(tiger_item *ti,cairo_t *cr)
{
  /* clip to the region if relevant */
  const kate_region *kr=ti->kin.event->region;
  if (kr && kr->clip) {
    tiger_rectangle clip;
    tiger_rectangle_snap(&ti->region,&clip);
    cairo_rectangle(cr,clip.x0,clip.y0,clip.x1-clip.x0,clip.y1-clip.y0);
    TIGER_CHECK_CAIRO_STATUS(cr);
#if 0
    cairo_set_source_rgb(cr,0,1,0);
    cairo_set_line_width(cr,5);
    cairo_stroke_preserve(cr);
#endif
    cairo_clip(cr);
    TIGER_CHECK_CAIRO_STATUS(cr);
  }
}

static void tiger_item_setup_color(tiger_color *tc,int kin_has,const kate_color *kin_kc,const tiger_color *default_color,double default_alpha)
{
  if (kin_has) {
    /* if there is a style, its color will have been placed into the tracker,
       so checking the style isn't necessary if the tracker has no color info */
    tiger_util_kate_color_to_tiger_color(tc,kin_kc);
  }
  else if (default_color) {
    *tc=*default_color;
  }
  else {
    tc->r=1.0;
    tc->g=1.0;
    tc->b=1.0;
    tc->a=default_alpha;
  }
}

static int tiger_item_can_be_cached(const tiger_item *ti)
{
  const kate_event *ev=ti->kin.event;
  size_t n;

  if (ev->nmotions==0) return 1;
  for (n=0;n<ev->nmotions;++n) {
    switch (ev->motions[n]->semantics) {
      /* any motion causes failure to cache, except a few known to be OK */
      case kate_motion_semantics_time:
      case kate_motion_semantics_z:
      case kate_motion_semantics_background_color_rg:
      case kate_motion_semantics_background_color_ba:
      case kate_motion_semantics_bitmap_position:
      case kate_motion_semantics_bitmap_size:
      case kate_motion_semantics_marker1_position:
      case kate_motion_semantics_marker2_position:
      case kate_motion_semantics_marker3_position:
      case kate_motion_semantics_marker4_position:
      case kate_motion_semantics_marker1_bitmap:
      case kate_motion_semantics_marker2_bitmap:
      case kate_motion_semantics_marker3_bitmap:
      case kate_motion_semantics_marker4_bitmap:
        break;
      /* some that might be OK someday */
      /*
      case kate_motion_semantics_region_position:
      case kate_motion_semantics_text_position:
      case kate_motion_semantics_draw:
      case kate_motion_semantics_draw_color_rg:
      case kate_motion_semantics_draw_color_ba:
        break;
      */
      default:
          return 0;
    }
  }

  /* we haven't found a motion that prevents caching */
  return 1;
}

static int tiger_item_should_be_cached(const tiger_item *ti)
{
  /* not if caching is disabled */
  if (!tiger_item_has_flag(ti,TIGER_FLAG_CACHING)) return 0;
  /* not if we don't have any text */
  if (ti->kin.event->len==0) return 0;
  /* not if it cannot be cached anyway */
  if (!tiger_item_can_be_cached(ti)) return 0;

  return 1;
}

int tiger_item_seek(tiger_item *ti,kate_float t)
{
  const kate_event *ev;

  if (!ti || t<0) return TIGER_E_INVALID_PARAMETER;

  ev=ti->kin.event;

  ti->dirty=1;

  /* if the time is after our end time, we're off */
  if (t>=ev->end_time) return 1;

#if 1
  /* if the time is before our start time, we're off too */
  if (t<ev->start_time) return 1;
#else
  /* if the time is before our start time, set to inactive but stay */
  if (t<ev->start_time) {
    ti->active=0;
    return 0;
  }
#endif

  /* otherwise, t is within our lifetime, we stay */
  return 0;
}

int tiger_item_update(tiger_item *ti,kate_float t,int track,cairo_t *cr)
{
  cairo_surface_t *cs;
  const kate_event *ev;
  const kate_tracker *kin;
  int w,h;

  if (!ti || t<0 || !cr) return TIGER_E_INVALID_PARAMETER;

  /* local access cache */
  kin=&ti->kin;
  ev=kin->event;

  /* early out if we're not within the lifetime of the event */
  if (t<ev->start_time) return 0;
  if (t>=ev->end_time) {
    ti->active=0;
    ti->dirty=1;
    return 1; /* we're done, and will get destroyed */
  }

  /* early out if we don't need to update the kate_tracker */
  if (!track) {
    /* since the tracker would be out of date, make sure we can't render */
    ti->active=0;
    return 0;
  }

  if (!ti->active) {
    ti->active=1;
    ti->dirty=1;
  }

  /* retrieve the surface for the context we got passed */
  cs=cairo_get_target(cr);
  TIGER_CHECK_CAIRO_STATUS(cr);

  /* first test whether the surface is an image or not */
  if (cairo_surface_get_type(cs)!=CAIRO_SURFACE_TYPE_IMAGE) return TIGER_E_BAD_SURFACE_TYPE;

  w=cairo_image_surface_get_width(cs);
  h=cairo_image_surface_get_height(cs);
  TIGER_CHECK_CAIRO_STATUS(cr);
  ti->frame_width=w;
  ti->frame_height=h;

  if (ti->kin.event->nmotions>0) {
    ti->dirty=1;
  }

  return kate_tracker_update(&ti->kin,t-ev->start_time,w,h,0,0,w,h);
}

static void tiger_item_create_cached_surface(tiger_item *ti,cairo_surface_t *cs,cairo_surface_t **cached_cs,cairo_t **cached_cr,double *dx,double *dy)
{
  const kate_region *kr=ti->kin.event->region;
  tiger_rectangle clip;
  tiger_rectangle dimensions;
  
  dimensions.x0=0.0;
  dimensions.y0=0.0;
  dimensions.x1=ti->frame_width;
  dimensions.y1=ti->frame_height;

  if (kr && kr->clip) {
    /* restrict dimensions to the clip region */
    tiger_rectangle_snap(&ti->region,&clip);
    tiger_rectangle_clip(&dimensions,&clip);
  }

  *cached_cs=cairo_surface_create_similar(cs,CAIRO_CONTENT_COLOR_ALPHA,dimensions.x1-dimensions.x0,dimensions.y1-dimensions.y0);
  *cached_cr=cairo_create(*cached_cs);
  *dx=dimensions.x0;
  *dy=dimensions.y0;

  cairo_translate(*cached_cr,-dimensions.x0,-dimensions.y0);
}

int tiger_item_render(tiger_item *ti,cairo_t *cr)
{
  cairo_surface_t *cs;
  const kate_tracker *kin;
  cairo_t *original_cr=cr;
  cairo_surface_t *cached_render=NULL;

  if (!ti || !cr) return TIGER_E_INVALID_PARAMETER;

  if (!ti->active) return 0;
  if (tiger_item_get_z(ti)<0) return 0;

  /* local access cache */
  kin=&ti->kin;

  /* retrieve the surface for the context we got passed */
  cs=cairo_get_target(cr);
  TIGER_CHECK_CAIRO_STATUS(cr);

  /* first test whether the surface is an image or not */
  if (cairo_surface_get_type(cs)!=CAIRO_SURFACE_TYPE_IMAGE) return TIGER_E_BAD_SURFACE_TYPE;

  /* if the image cache has swapped RGB but we don't have anymore (or vice versa),
     we have to clear the cache and let it refill itself as needed */
  if (!!tiger_item_has_flag(ti,TIGER_FLAG_SWAP_RGB)!=!!ti->tbc->swap_rgb) {
    tiger_bitmap_cache_clear(ti->tbc);
    tiger_bitmap_cache_init(ti->tbc,tiger_item_has_flag(ti,TIGER_FLAG_SWAP_RGB));
  }

  cairo_save(cr);
  TIGER_CHECK_CAIRO_STATUS(cr);

  tiger_item_setup_region(ti,kin);
  tiger_item_setup_color(&ti->text_color,kin->has.text_color,&kin->text_color,&ti->defaults->font_color,1.0);
  tiger_item_setup_color(&ti->background_color,kin->has.background_color,&kin->background_color,NULL,0.0);
  tiger_item_set_clip(ti,cr);

  tiger_item_draw_background(ti,cr);

  /* if the event is not animated, we can render its text to a cache instead, and reuse it */
  if (!ti->cached_render && tiger_item_should_be_cached(ti)) {
    cairo_t *cached_cr;
    tiger_item_create_cached_surface(ti,cs,&cached_render,&cached_cr,&ti->cached_render_dx,&ti->cached_render_dy);
    cr=cached_cr;
    TIGER_CHECK_CAIRO_STATUS(cr);
    tiger_item_set_clip(ti,cr);
    /* we're now rendering on the cached surface */
  }

  /* we don't render if we have a cached render */
  if (!ti->cached_render) {
    /* we want to keep track of the render area even if not caching it, because if we need to paint with alpha,
       we'll use that info to clip to the mimimum area for speed
       this is executed both when caching, and when drawing with no caching */
    tiger_rectangle_empty(&ti->cached_render_bounds);
    tiger_item_draw_text(ti,cr);
  }

  /* if we just created a cached render, clip it and save it - after the render test above */
  if (cached_render) {
    tiger_item_clip_rectangle(&ti->cached_render_bounds,cr);
    ti->cached_render=cached_render;
    tiger_rectangle_snap(&ti->cached_render_bounds,&ti->cached_render_bounds);

    /* we don't need the temporary context anymore */
    cairo_destroy(cr);
    cr=original_cr;
  }

  /* markers are never cached as they will most likely move */
  tiger_item_draw_markers(ti,cr);

  cairo_restore(cr);
  TIGER_CHECK_CAIRO_STATUS(cr);

  /* render the cache if appropriate */
  if (ti->cached_render) {
    tiger_rectangle clip=ti->cached_render_bounds;
    cairo_save(cr);
    TIGER_CHECK_CAIRO_STATUS(cr);

    cairo_device_to_user(cr,&clip.x0,&clip.y0);
    cairo_device_to_user(cr,&clip.x1,&clip.y1);
    cairo_rectangle(cr,clip.x0+ti->cached_render_dx,clip.y0+ti->cached_render_dy,clip.x1-clip.x0,clip.y1-clip.y0);
    cairo_clip_preserve(cr);
    cairo_set_source_surface(cr,ti->cached_render,ti->cached_render_dx,ti->cached_render_dy);
    TIGER_CHECK_CAIRO_STATUS(cr);
    cairo_fill(cr);
    TIGER_CHECK_CAIRO_STATUS(cr);

#ifdef DEBUG
    if (tiger_item_has_flag(ti,TIGER_FLAG_DEBUG)) {
      cairo_rectangle(cr,clip.x0+ti->cached_render_dx,clip.y0+ti->cached_render_dy,clip.x1-clip.x0,clip.y1-clip.y0);
      cairo_set_source_rgba(cr,0.5,1,0.5,0.3);
      cairo_stroke(cr);
      TIGER_CHECK_CAIRO_STATUS(cr);
    }
#endif

    cairo_restore(cr);
    TIGER_CHECK_CAIRO_STATUS(cr);
  }

  tiger_draw_render(&ti->draw,cr,kin->t,tiger_item_has_flag(ti,TIGER_FLAG_SWAP_RGB));

  ti->dirty=0;

#ifdef DEBUG
  if (tiger_item_has_flag(ti,TIGER_FLAG_DUMP_CACHED_RENDERS) && cached_render) {
    static int i=0; char name[48]; sprintf(name,"/tmp/tiger-cache-%d.png",i++);
    if (cairo_surface_write_to_png(cached_render,name)!=CAIRO_STATUS_SUCCESS) {
      tiger_log(tiger_log_error,"png write trouble\n");
    }
  }
#endif

  return 0;
}

kate_float tiger_item_get_z(const tiger_item *ti)
{
  if (!ti) return (kate_float)0;
  if (!ti->active) return (kate_float)0;
  if (ti->kin.has.z) return ti->kin.z;
  return (kate_float)0;
}

int tiger_item_is_active(const tiger_item *ti)
{
  if (!ti) return 0;
  return ti->active;
}

int tiger_item_is_dirty(const tiger_item *ti)
{
  if (!ti) return 0;
  return ti->dirty;
}

int tiger_item_invalidate_cache(tiger_item *ti)
{
  if (!ti) return TIGER_E_INVALID_PARAMETER;

  if (ti->cached_render) {
    cairo_surface_destroy(ti->cached_render);
    ti->cached_render=NULL;
    ti->cached_render_dx=0;
    ti->cached_render_dy=0;
  }
  ti->dirty=1;

  return 0;
}

/**
  \ingroup item
  Clears a previously initialized Tiger item
  \param tiger the Tiger item to clear
  \returns 0 success
  \returns TIGER_E_* error
  */
int tiger_item_clear(tiger_item *ti)
{
  if (!ti) return TIGER_E_INVALID_PARAMETER;

  if (ti->cached_render) cairo_surface_destroy(ti->cached_render);

  if (ti->context) g_object_unref(ti->context);
  if (ti->font_map) g_object_unref(ti->font_map);

  tiger_draw_clear(&ti->draw);
  tiger_bitmap_cache_clear(ti->tbc);
  tiger_free(ti->tbc);

  return kate_tracker_clear(&ti->kin);
}

