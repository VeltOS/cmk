/*
 * This file is part of graphene-desktop, the desktop environment of VeltOS.
 * Copyright (C) 2016 Velt Technologies, Aidan Shafran <zelbrium@gmail.com>
 * Licensed under the Apache License 2 <www.apache.org/licenses/LICENSE-2.0>.
 */

#include "style.h"

struct _CmkStyle
{
	ClutterActor parent;
	GHashTable *colors;
	float bevelRadius;
	float padding;
};

enum
{
	SIGNAL_CHANGED = 1,
	SIGNAL_LAST
};

static guint signals[SIGNAL_LAST];

static void cmk_style_dispose(GObject *self_);
static void free_table_color(gpointer color);

G_DEFINE_TYPE(CmkStyle, cmk_style, G_TYPE_OBJECT);



CmkStyle * cmk_style_new()
{
	return CMK_STYLE(g_object_new(CMK_TYPE_STYLE, NULL));
}


CmkStyle * cmk_style_get_default()
{
	static CmkStyle *globalStyle = NULL;
	if(CMK_IS_STYLE(globalStyle))
		return g_object_ref(globalStyle);
	globalStyle = cmk_style_new();
	return globalStyle;
}

static void cmk_style_class_init(CmkStyleClass *class)
{
	G_OBJECT_CLASS(class)->dispose = cmk_style_dispose;

	signals[SIGNAL_CHANGED] = g_signal_new("changed", G_TYPE_FROM_CLASS(class), G_SIGNAL_RUN_FIRST, 0, NULL, NULL, NULL, G_TYPE_NONE, 0);
}

static void cmk_style_init(CmkStyle *self)
{
	self->colors = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, free_table_color);
	
	// Default colors
	CmkColor color;
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
	CmkStyle *self = CMK_STYLE(self_);
	g_clear_pointer(&self->colors, g_hash_table_unref);
	G_OBJECT_CLASS(cmk_style_parent_class)->dispose(self_);
}

static void free_table_color(gpointer color)
{
	g_slice_free(CmkColor, color);
}

CmkColor * cmk_copy_color(CmkColor *dest, const CmkColor *source)
{
	g_return_val_if_fail(dest, dest);
	g_return_val_if_fail(source, dest);
	dest->r = source->r;
	dest->g = source->g;
	dest->b = source->b;
	dest->a = source->a;
	return dest;
}

CmkColor * cmk_set_color(CmkColor *dest, float r, float g, float b, float a)
{
	g_return_val_if_fail(dest, dest);
	dest->r = r;
	dest->g = g;
	dest->b = b;
	dest->a = a;
	return dest;
}

ClutterColor cmk_to_clutter_color(const CmkColor * color)
{
	ClutterColor cc;
	clutter_color_init(&cc, 0, 0, 0, 0);
	g_return_val_if_fail(color, cc);

	clutter_color_init(&cc, color->r*255, color->g*255, color->b*255, color->a*255);
	return cc;
}

void clutter_actor_set_background_cmk_color(ClutterActor *actor, const CmkColor *color)
{
	ClutterColor cc = cmk_to_clutter_color(color);
	clutter_actor_set_background_color(actor, &cc);
}

void cairo_set_source_cmk_color(cairo_t *cr, const CmkColor *color)
{
	g_return_if_fail(color);
	cairo_set_source_rgba(cr, color->r, color->g, color->b, color->a);
}

CmkColor * cmk_overlay_colors(CmkColor *dest, const CmkColor *a, const CmkColor *b)
{
	// TODO
	return dest;
}

const CmkColor * cmk_style_get_color(CmkStyle *self, const gchar *name)
{
	g_return_val_if_fail(CMK_IS_STYLE(self), NULL);
	if(!name)
		return NULL;
	return g_hash_table_lookup(self->colors, name);
}

void cmk_style_set_color(CmkStyle *self, const gchar *name, const CmkColor *color)
{
	g_return_if_fail(CMK_IS_STYLE(self));

	CmkColor *c = g_slice_new(CmkColor);
	cmk_copy_color(c, color);
	g_hash_table_insert(self->colors, g_strdup(name), c);
	g_signal_emit(self, signals[SIGNAL_CHANGED], 0);
}

void cmk_style_get_font_color_for_background(CmkStyle *self, const gchar *bgColorName, CmkColor *dest)
{
	cmk_set_color(dest, 0, 0, 0, 1); 
	g_return_if_fail(CMK_IS_STYLE(self));
	const CmkColor *color = NULL;
	if(bgColorName)
	{
		gchar *name = g_strdup_printf("%s-font", bgColorName);
		color = cmk_style_get_color(self, name);
		g_free(name);
	}
	else
		color = cmk_style_get_color(self, "font");
		
	if(color)
		cmk_copy_color(dest, color);
} 

void cmk_style_set_bevel_radius(CmkStyle *self, float radius)
{
	g_return_if_fail(CMK_IS_STYLE(self));
	self->bevelRadius = radius;
	g_signal_emit(self, signals[SIGNAL_CHANGED], 0);
}

float cmk_style_get_bevel_radius(CmkStyle *self)
{
	g_return_val_if_fail(CMK_IS_STYLE(self), 0.0);
	return self->bevelRadius;
}

void cmk_style_set_padding(CmkStyle *self, float padding)
{
	g_return_if_fail(CMK_IS_STYLE(self));
	self->padding = padding;
	g_signal_emit(self, signals[SIGNAL_CHANGED], 0);
}

float cmk_style_get_padding(CmkStyle *self)
{
	g_return_val_if_fail(CMK_IS_STYLE(self), 0.0);
	return self->padding;
}
