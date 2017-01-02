/*
 * This file is part of graphene-desktop, the desktop environment of VeltOS.
 * Copyright (C) 2016 Velt Technologies, Aidan Shafran <zelbrium@gmail.com>
 * Licensed under the Apache License 2 <www.apache.org/licenses/LICENSE-2.0>.
 */

#include "style.h"

struct _CMKStyle
{
	ClutterActor parent;
	GHashTable *colors;
	float bevelRadius;
	float padding;
};

enum
{
	SIGNAL_STYLE_CHANGED = 1,
	SIGNAL_LAST
};

static guint signals[SIGNAL_LAST];

static void cmk_style_dispose(GObject *self_);
static void free_table_color(gpointer color);

G_DEFINE_TYPE(CMKStyle, cmk_style, G_TYPE_OBJECT);



CMKStyle * cmk_style_new()
{
	return CMK_STYLE(g_object_new(CMK_TYPE_STYLE, NULL));
}


CMKStyle * cmk_style_get_default()
{
	static CMKStyle *globalStyle = NULL;
	if(CMK_IS_STYLE(globalStyle))
		return g_object_ref(globalStyle);
	globalStyle = cmk_style_new();
	return globalStyle;
}

static void cmk_style_class_init(CMKStyleClass *class)
{
	G_OBJECT_CLASS(class)->dispose = cmk_style_dispose;

	signals[SIGNAL_STYLE_CHANGED] = g_signal_new("style-changed", G_TYPE_FROM_CLASS(class), G_SIGNAL_RUN_FIRST, 0, NULL, NULL, NULL, G_TYPE_NONE, 0);
}

static void cmk_style_init(CMKStyle *self)
{
	self->colors = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, free_table_color);
	
	// Default colors
	CMKColor color;
	cmk_style_set_color(self, "background", cmk_set_color(&color, 1, 1, 1, 1));
	cmk_style_set_color(self, "primary", cmk_set_color(&color, 1, 1, 1, 1));
	cmk_style_set_color(self, "secondary", cmk_set_color(&color, 1, 1, 1, 1));
	cmk_style_set_color(self, "accent", cmk_set_color(&color, 0.5, 0, 0, 1));
	cmk_style_set_color(self, "hover", cmk_set_color(&color, 0, 0, 0, 0.1));
	cmk_style_set_color(self, "activate", cmk_set_color(&color, 0, 0, 0, 0.1));
	self->bevelRadius = 5.0;
	self->padding = 20;
}

static void cmk_style_dispose(GObject *self_)
{
	CMKStyle *self = CMK_STYLE(self_);
	g_clear_pointer(&self->colors, g_hash_table_unref);
	G_OBJECT_CLASS(cmk_style_parent_class)->dispose(self_);
}

static void free_table_color(gpointer color)
{
	g_slice_free(CMKColor, color);
}

CMKColor * cmk_copy_color(CMKColor *dest, const CMKColor *source)
{
	g_return_val_if_fail(dest, dest);
	g_return_val_if_fail(source, dest);
	dest->r = source->r;
	dest->g = source->g;
	dest->b = source->b;
	dest->a = source->a;
	return dest;
}

CMKColor * cmk_set_color(CMKColor *dest, float r, float g, float b, float a)
{
	g_return_val_if_fail(dest, dest);
	dest->r = r;
	dest->g = g;
	dest->b = b;
	dest->a = a;
	return dest;
}

ClutterColor cmk_to_clutter_color(const CMKColor * color)
{
	ClutterColor cc;
	clutter_color_init(&cc, 0, 0, 0, 0);
	g_return_val_if_fail(color, cc);

	clutter_color_init(&cc, color->r*255, color->g*255, color->b*255, color->a*255);
	return cc;
}

void clutter_actor_set_background_cmk_color(ClutterActor *actor, const CMKColor *color)
{
	ClutterColor cc = cmk_to_clutter_color(color);
	clutter_actor_set_background_color(actor, &cc);
}

void cairo_set_source_cmk_color(cairo_t *cr, const CMKColor *color)
{
	g_return_if_fail(color);
	cairo_set_source_rgba(cr, color->r, color->g, color->b, color->a);
}

CMKColor * cmk_overlay_colors(CMKColor *dest, const CMKColor *a, const CMKColor *b)
{
	// TODO
	return dest;
}

const CMKColor * cmk_style_get_color(CMKStyle *self, const gchar *name)
{
	g_return_val_if_fail(CMK_IS_STYLE(self), NULL);
	if(!name)
		return NULL;
	return g_hash_table_lookup(self->colors, name);
}

void cmk_style_set_color(CMKStyle *self, const gchar *name, const CMKColor *color)
{
	g_return_if_fail(CMK_IS_STYLE(self));

	CMKColor *c = g_slice_new(CMKColor);
	cmk_copy_color(c, color);
	g_hash_table_insert(self->colors, g_strdup(name), c);
	g_signal_emit(self, signals[SIGNAL_STYLE_CHANGED], 0);
}

void cmk_style_get_font_color_for_background(CMKStyle *self, const gchar *bgColorName, CMKColor *dest)
{
	cmk_set_color(dest, 0, 0, 0, 1); 
	g_return_if_fail(CMK_IS_STYLE(self));
	gchar *name = g_strdup_printf("%s-font", bgColorName);
	const CMKColor *color = cmk_style_get_color(self, name);
	g_free(name);
	if(color)
		cmk_copy_color(dest, color);
} 

void cmk_style_set_bevel_radius(CMKStyle *self, float radius)
{
	g_return_if_fail(CMK_IS_STYLE(self));
	self->bevelRadius = radius;
	g_signal_emit(self, signals[SIGNAL_STYLE_CHANGED], 0);
}

float cmk_style_get_bevel_radius(CMKStyle *self)
{
	g_return_val_if_fail(CMK_IS_STYLE(self), 0.0);
	return self->bevelRadius;
}

void cmk_style_set_padding(CMKStyle *self, float padding)
{
	g_return_if_fail(CMK_IS_STYLE(self));
	self->padding = padding;
	g_signal_emit(self, signals[SIGNAL_STYLE_CHANGED], 0);
}

float cmk_style_get_padding(CMKStyle *self)
{
	g_return_val_if_fail(CMK_IS_STYLE(self), 0.0);
	return self->padding;
}
