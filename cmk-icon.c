/*
 * This file is part of graphene-desktop, the desktop environment of VeltOS.
 * Copyright (C) 2016 Velt Technologies, Aidan Shafran <zelbrium@gmail.com>
 * Licensed under the Apache License 2 <www.apache.org/licenses/LICENSE-2.0>.
 */

#include "cmk-icon.h"
#include "cmk-icon-loader.h"

typedef struct _CmkIconPrivate CmkIconPrivate;
struct _CmkIconPrivate
{
	gchar *iconName;
	gchar *themeName;
	CmkIconLoader *loader;
	cairo_surface_t *iconSurface;
};

enum
{
	PROP_ICON_NAME = 1,
	PROP_ICON_THEME,
	PROP_LAST
};

static GParamSpec *properties[PROP_LAST];

static void cmk_icon_dispose(GObject *self_);
static void cmk_icon_set_property(GObject *self_, guint propertyId, const GValue *value, GParamSpec *pspec);
static void cmk_icon_get_property(GObject *self_, guint propertyId, GValue *value, GParamSpec *pspec);
static void on_background_changed(CmkWidget *self_);
static gboolean on_draw_canvas(ClutterCanvas *canvas, cairo_t *cr, int width, int height, CmkIcon *self);
static void update_canvas(ClutterActor *self_);

G_DEFINE_TYPE_WITH_PRIVATE(CmkIcon, cmk_icon, CMK_TYPE_WIDGET);
#define PRIVATE(icon) ((CmkIconPrivate *)cmk_icon_get_instance_private(icon))

CmkIcon * cmk_icon_new(void)
{
	return CMK_ICON(g_object_new(CMK_TYPE_ICON, NULL));
}

CmkIcon * cmk_icon_new_from_name(const gchar *iconName)
{
	return CMK_ICON(g_object_new(CMK_TYPE_ICON, "icon-name", iconName, NULL));
}

static void cmk_icon_class_init(CmkIconClass *class)
{
	GObjectClass *base = G_OBJECT_CLASS(class);
	base->dispose = cmk_icon_dispose;
	base->get_property = cmk_icon_get_property;
	base->set_property = cmk_icon_set_property;
	
	CMK_WIDGET_CLASS(class)->background_changed = on_background_changed;

	properties[PROP_ICON_NAME] = g_param_spec_string("icon-name", "icon-name", "Icon name", NULL, G_PARAM_READWRITE);
	properties[PROP_ICON_THEME] = g_param_spec_string("icon-theme", "icon-theme", "Icon theme name", NULL, G_PARAM_READWRITE);

	g_object_class_install_properties(base, PROP_LAST, properties);
}

static void cmk_icon_init(CmkIcon *self)
{
	ClutterContent *canvas = clutter_canvas_new();
	clutter_canvas_set_scale_factor(CLUTTER_CANVAS(canvas), 1); // Manual scaling
	g_signal_connect(canvas, "draw", G_CALLBACK(on_draw_canvas), self);
	clutter_actor_set_content_gravity(CLUTTER_ACTOR(self), CLUTTER_CONTENT_GRAVITY_CENTER);
	clutter_actor_set_content(CLUTTER_ACTOR(self), canvas);

	g_signal_connect(CLUTTER_ACTOR(self), "notify::size", G_CALLBACK(update_canvas), NULL);

	PRIVATE(self)->loader = cmk_icon_loader_get_default();
}

static void cmk_icon_dispose(GObject *self_)
{
	CmkIconPrivate *private = PRIVATE(CMK_ICON(self_));
	g_clear_object(&private->loader);
	g_clear_pointer(&private->iconSurface, cairo_surface_destroy);
	g_clear_pointer(&private->iconName, g_free);
	g_clear_pointer(&private->themeName, g_free);
	G_OBJECT_CLASS(cmk_icon_parent_class)->dispose(self_);
}

static void cmk_icon_set_property(GObject *self_, guint propertyId, const GValue *value, GParamSpec *pspec)
{
	g_return_if_fail(CMK_IS_ICON(self_));
	CmkIcon *self = CMK_ICON(self_);
	
	switch(propertyId)
	{
	case PROP_ICON_NAME:
		cmk_icon_set_icon(self, g_value_get_string(value));
		break;
	case PROP_ICON_THEME:
		cmk_icon_set_icon_theme(self, g_value_get_string(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(self, propertyId, pspec);
		break;
	}
}

static void cmk_icon_get_property(GObject *self_, guint propertyId, GValue *value, GParamSpec *pspec)
{
	g_return_if_fail(CMK_IS_ICON(self_));
	CmkIcon *self = CMK_ICON(self_);
	
	switch(propertyId)
	{
	case PROP_ICON_NAME:
		g_value_set_string(value, cmk_icon_get_icon(self));
		break;
	case PROP_ICON_THEME:
		g_value_set_string(value, cmk_icon_get_icon_theme(self));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(self, propertyId, pspec);
		break;
	}
}

static void on_background_changed(CmkWidget *self_)
{
}

static gboolean on_draw_canvas(ClutterCanvas *canvas, cairo_t *cr, int width, int height, CmkIcon *self)
{
	cairo_save(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
	cairo_paint(cr);
	cairo_restore(cr);
	
	if(PRIVATE(self)->iconSurface)
	{
		gdouble factor = (gdouble)height / cairo_image_surface_get_height(PRIVATE(self)->iconSurface);
		cairo_scale(cr, factor, factor);
		cairo_set_source_surface(cr, PRIVATE(self)->iconSurface, 0, 0);
		cairo_paint(cr);
	}
	return TRUE;
}

static void update_canvas(ClutterActor *self_)
{
	ClutterCanvas *canvas = CLUTTER_CANVAS(clutter_actor_get_content(self_));

	CmkIconPrivate *private = PRIVATE(CMK_ICON(self_));
	g_clear_pointer(&private->iconSurface, cairo_surface_destroy);

	gfloat width, height;
	clutter_actor_get_size(CLUTTER_ACTOR(self_), &width, &height);
	gfloat size = MIN(width, height);
	guint scale = cmk_icon_loader_get_scale(private->loader);
	gfloat scaledSize = size / scale;

	if(private->iconName)
		private->iconSurface = cmk_icon_loader_get(private->loader, private->iconName, scaledSize);

	if(!clutter_canvas_set_size(canvas, size, size))
		clutter_content_invalidate(CLUTTER_CONTENT(canvas));
}

void cmk_icon_set_icon(CmkIcon *self, const gchar *iconName)
{
	g_return_if_fail(CMK_IS_ICON(self));

	g_free(PRIVATE(self)->iconName);
	PRIVATE(self)->iconName = g_strdup(iconName);

	update_canvas(CLUTTER_ACTOR(self));
}

const gchar * cmk_icon_get_icon(CmkIcon *self)
{
	g_return_val_if_fail(CMK_IS_ICON(self), NULL);
	return PRIVATE(self)->iconName;
}

void cmk_icon_set_size(CmkIcon *self, gfloat size)
{
	g_return_if_fail(CMK_IS_ICON(self));
	guint scale = cmk_icon_loader_get_scale(PRIVATE(self)->loader);
	size *= scale;
	clutter_actor_set_size(CLUTTER_ACTOR(self), size, size);
}

gfloat cmk_icon_get_size(CmkIcon *self)
{
	g_return_val_if_fail(CMK_IS_ICON(self), 0);
	gfloat width, height;
	clutter_actor_get_size(CLUTTER_ACTOR(self), &width, &height);
	gfloat size = MIN(width, height);
	guint scale = cmk_icon_loader_get_scale(PRIVATE(self)->loader);
	return size / scale;
}

void cmk_icon_set_icon_theme(CmkIcon *self, const gchar *themeName)
{
	g_return_if_fail(CMK_IS_ICON(self));
	g_free(PRIVATE(self)->themeName);
	PRIVATE(self)->themeName = g_strdup(themeName);
}

const gchar * cmk_icon_get_icon_theme(CmkIcon *self)
{
	g_return_val_if_fail(CMK_IS_ICON(self), NULL);
	return PRIVATE(self)->themeName;
}
