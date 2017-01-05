/*
 * This file is part of graphene-desktop, the desktop environment of VeltOS.
 * Copyright (C) 2016 Velt Technologies, Aidan Shafran <zelbrium@gmail.com>
 * Licensed under the Apache License 2 <www.apache.org/licenses/LICENSE-2.0>.
 */

#ifndef __CMK_STYLE_H__
#define __CMK_STYLE_H__

#include <clutter/clutter.h>

G_BEGIN_DECLS

#define CMK_TYPE_STYLE cmk_style_get_type()
G_DECLARE_FINAL_TYPE(CmkStyle, cmk_style, CMK, STYLE, GObject);

typedef struct {
	float r,g,b,a;
} CmkColor;

CmkColor * cmk_copy_color(CmkColor *dest, const CmkColor * source);
CmkColor * cmk_set_color(CmkColor *dest, float r, float g, float b, float a);
ClutterColor cmk_to_clutter_color(const CmkColor *color);
void clutter_actor_set_background_cmk_color(ClutterActor *actor, const CmkColor *color);
void cairo_set_source_cmk_color(cairo_t *cr, const CmkColor *color);
CmkColor * cmk_overlay_colors(CmkColor *dest, const CmkColor *a, const CmkColor *b);

CmkStyle * cmk_style_new();

/*
 * Returns a ref to the default style. You should unref with with
 * g_object_unref when you're done with it.
 */
CmkStyle * cmk_style_get_default();

void  cmk_style_load_file(CmkStyle *style, const gchar *filePath, GError **error);

const CmkColor * cmk_style_get_color(CmkStyle *style, const gchar *name);
void  cmk_style_set_color(CmkStyle *style, const gchar *name, const CmkColor *color);

/*
 * "Background" colors are the same as colors set with cmk_style_set_color.
 * This is a convenience function for calling cmk_style_get_color with
 * 'name' as 'bgColorName-font', except that if no font color is found for
 * that background, solid black is returned instead of NULL.
 */
void cmk_style_get_font_color_for_background(CmkStyle *style, const gchar *bgColorName, CmkColor *dest);

void  cmk_style_set_bevel_radius(CmkStyle *style, float radius);
float cmk_style_get_bevel_radius(CmkStyle *style);

void  cmk_style_set_padding(CmkStyle *style, float padding);
float cmk_style_get_padding(CmkStyle *style);

G_END_DECLS

#endif

