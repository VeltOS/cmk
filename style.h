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
G_DECLARE_FINAL_TYPE(CMKStyle, cmk_style, CMK, STYLE, GObject);

typedef struct {
	float r,g,b,a;
} CMKColor;

CMKColor * cmk_copy_color(CMKColor *dest, const CMKColor * source);
CMKColor * cmk_set_color(CMKColor *dest, float r, float g, float b, float a);
ClutterColor cmk_to_clutter_color(const CMKColor *color);
void clutter_actor_set_background_cmk_color(ClutterActor *actor, const CMKColor *color);
void cairo_set_source_cmk_color(cairo_t *cr, const CMKColor *color);
CMKColor * cmk_overlay_colors(CMKColor *dest, const CMKColor *a, const CMKColor *b);

CMKStyle * cmk_style_new();

/*
 * Returns a ref to the default style. You should unref with with
 * g_object_unref when you're done with it.
 */
CMKStyle * cmk_style_get_default();

void  cmk_style_load_file(CMKStyle *style, const gchar *filePath, GError **error);

const CMKColor * cmk_style_get_color(CMKStyle *style, const gchar *name);
void  cmk_style_set_color(CMKStyle *style, const gchar *name, const CMKColor *color);

/*
 * "Background" colors are the same as colors set with cmk_style_set_color.
 * This is a convenience function for calling cmk_style_get_color with
 * 'name' as 'bgColorName-font', except that if no font color is found for
 * that background, solid black is returned instead of NULL.
 */
void cmk_style_get_font_color_for_background(CMKStyle *style, const gchar *bgColorName, CMKColor *dest);

void  cmk_style_set_bevel_radius(CMKStyle *style, float radius);
float cmk_style_get_bevel_radius(CMKStyle *style);

void  cmk_style_set_padding(CMKStyle *style, float padding);
float cmk_style_get_padding(CMKStyle *style);

G_END_DECLS

#endif

