/*
 * This file is part of graphene-desktop, the desktop environment of VeltOS.
 * Copyright (C) 2016 Velt Technologies, Aidan Shafran <zelbrium@gmail.com>
 * Licensed under the Apache License 2 <www.apache.org/licenses/LICENSE-2.0>.
 */

#include "cmk-icon.h"
#include "cmk-icon-loader.h"
#include <math.h>

typedef struct _CmkIconPrivate CmkIconPrivate;
struct _CmkIconPrivate
{
	gchar *iconName;
	gchar *themeName;
	gboolean useForegroundColor;
	CmkIconLoader *loader;
	cairo_surface_t *iconSurface;
	gboolean setPixmap;
	gboolean dirty;

	// A size "request" for the actor. Can be scaled by the style scale
	// factor. If this is <=0, the actor's standard allocated size is used.
	gfloat size;
};

enum
{
	PROP_ICON_NAME = 1,
	PROP_ICON_THEME,
	PROP_ICON_SIZE,
	PROP_USE_FOREGROUND_COLOR,
	PROP_LAST
};

static GParamSpec *properties[PROP_LAST];

static void cmk_icon_dispose(GObject *self_);
static void cmk_icon_set_property(GObject *self_, guint propertyId, const GValue *value, GParamSpec *pspec);
static void cmk_icon_get_property(GObject *self_, guint propertyId, GValue *value, GParamSpec *pspec);
static void get_preferred_width(ClutterActor *self_, gfloat forHeight, gfloat *minWidth, gfloat *natWidth);
static void get_preferred_height(ClutterActor *self_, gfloat forWidth, gfloat *minHeight, gfloat *natHeight);
static void on_styles_changed(CmkWidget *self_, guint flags);
static void on_default_icon_theme_changed(CmkIcon *self);
static gboolean on_draw_canvas(ClutterCanvas *canvas, cairo_t *cr, int width, int height, CmkIcon *self);

G_DEFINE_TYPE_WITH_PRIVATE(CmkIcon, cmk_icon, CMK_TYPE_WIDGET);
#define PRIVATE(icon) ((CmkIconPrivate *)cmk_icon_get_instance_private(icon))

CmkIcon * cmk_icon_new(gfloat size)
{
	return CMK_ICON(g_object_new(CMK_TYPE_ICON, "icon-size", size, NULL));
}

CmkIcon * cmk_icon_new_from_name(const gchar *iconName, gfloat size)
{
	return CMK_ICON(g_object_new(CMK_TYPE_ICON,
		"icon-name", iconName,
		"icon-size", size,
		NULL));
}

CmkIcon * cmk_icon_new_full(const gchar *iconName, const gchar *themeName, gfloat size, gboolean useForeground)
{
	return CMK_ICON(g_object_new(CMK_TYPE_ICON,
		"icon-name", iconName,
		"icon-theme", themeName,
		"icon-size", size,
		"use-foreground-color", useForeground,
		NULL));
}

static void queue_update_canvas(CmkIcon *self)
{
	PRIVATE(self)->dirty = TRUE;
	clutter_actor_queue_redraw(CLUTTER_ACTOR(self));
}

static void on_paint(ClutterActor *self_)
{
	CmkIconPrivate *private = PRIVATE(CMK_ICON(self_));
	if(!private->dirty)
		return;
	private->dirty = FALSE;
	
	gfloat fscale = cmk_widget_get_dp_scale(CMK_WIDGET(self_));
	guint scale = roundf(fscale);
	
	if(!private->setPixmap)
	{
		g_clear_pointer(&private->iconSurface, cairo_surface_destroy);
		if(private->iconName)
		{
			gchar *path = cmk_icon_loader_lookup_full(private->loader, private->iconName, TRUE, private->themeName, TRUE, private->size, scale);
			if(!path)
				path = cmk_icon_loader_lookup_full(private->loader, "gtk-missing-image", TRUE, private->themeName, TRUE, private->size, scale);
			
			private->iconSurface = cmk_icon_loader_load(private->loader, path, private->size, scale, TRUE);
			g_free(path);
		}
	}
	
	ClutterCanvas *canvas = CLUTTER_CANVAS(clutter_actor_get_content(self_));
	if(!clutter_canvas_set_size(canvas, private->size*scale, private->size*scale))
		clutter_content_invalidate(CLUTTER_CONTENT(canvas));
}

static void cmk_icon_class_init(CmkIconClass *class)
{
	GObjectClass *base = G_OBJECT_CLASS(class);
	base->dispose = cmk_icon_dispose;
	base->get_property = cmk_icon_get_property;
	base->set_property = cmk_icon_set_property;

	ClutterActorClass *actorClass = CLUTTER_ACTOR_CLASS(class);
	actorClass->get_preferred_width = get_preferred_width;
	actorClass->get_preferred_height = get_preferred_height;
	actorClass->paint = on_paint;
	
	CMK_WIDGET_CLASS(class)->styles_changed = on_styles_changed;

	properties[PROP_ICON_NAME] = g_param_spec_string("icon-name", "icon-name", "Icon name", NULL, G_PARAM_READWRITE);
	properties[PROP_ICON_THEME] = g_param_spec_string("icon-theme", "icon-theme", "Icon theme name", NULL, G_PARAM_READWRITE);
	properties[PROP_ICON_SIZE] = g_param_spec_float("icon-size", "icon-size", "Icon size reqest", 0, 1024, 0, G_PARAM_READWRITE);
	properties[PROP_USE_FOREGROUND_COLOR] = g_param_spec_boolean("use-foreground-color", "use foreground color", "use foreground color to color the icon", FALSE, G_PARAM_READWRITE);

	g_object_class_install_properties(base, PROP_LAST, properties);
}

static void cmk_icon_init(CmkIcon *self)
{
	ClutterContent *canvas = clutter_canvas_new();
	clutter_canvas_set_scale_factor(CLUTTER_CANVAS(canvas), 1); // Manual scaling
	g_signal_connect(canvas, "draw", G_CALLBACK(on_draw_canvas), self);
	clutter_actor_set_content_gravity(CLUTTER_ACTOR(self), CLUTTER_CONTENT_GRAVITY_CENTER);
	clutter_actor_set_content(CLUTTER_ACTOR(self), canvas);
	
	PRIVATE(self)->loader = cmk_icon_loader_get_default();
	g_signal_connect_swapped(PRIVATE(self)->loader, "notify::default-theme", G_CALLBACK(on_default_icon_theme_changed), self);
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
	case PROP_ICON_SIZE:
		cmk_icon_set_size(self, g_value_get_float(value));
		break;
	case PROP_USE_FOREGROUND_COLOR:
		cmk_icon_set_use_foreground_color(self, g_value_get_boolean(value));
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
	case PROP_ICON_SIZE:
		g_value_set_float(value, cmk_icon_get_size(self));
		break;
	case PROP_USE_FOREGROUND_COLOR:
		g_value_set_boolean(value, cmk_icon_get_use_foreground_color(self));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(self, propertyId, pspec);
		break;
	}
}

static void get_preferred_width(ClutterActor *self_, gfloat forHeight, gfloat *minWidth, gfloat *natWidth)
{
	gfloat scale = cmk_widget_get_dp_scale(CMK_WIDGET(self_));
	*minWidth = *natWidth = scale * PRIVATE(CMK_ICON(self_))->size;
}

static void get_preferred_height(ClutterActor *self_, gfloat forWidth, gfloat *minHeight, gfloat *natHeight)
{
	gfloat scale = cmk_widget_get_dp_scale(CMK_WIDGET(self_));
	*minHeight = *natHeight = scale * PRIVATE(CMK_ICON(self_))->size;
}

static void on_styles_changed(CmkWidget *self_, guint flags)
{
	CMK_WIDGET_CLASS(cmk_icon_parent_class)->styles_changed(self_, flags);
	if((flags & CMK_STYLE_FLAG_COLORS)
	|| (flags & CMK_STYLE_FLAG_BACKGROUND_NAME)
	|| (flags & CMK_STYLE_FLAG_DP))
		queue_update_canvas(CMK_ICON(self_));
}

static void on_default_icon_theme_changed(CmkIcon *self)
{
	if(PRIVATE(self)->themeName == NULL)
		queue_update_canvas(self);
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
		if(PRIVATE(self)->useForegroundColor)
		{
			cairo_set_source_clutter_color(cr, cmk_widget_get_foreground_clutter_color(CMK_WIDGET(self)));
			cairo_mask_surface(cr, PRIVATE(self)->iconSurface, 0, 0);
		}
		else
		{
			cairo_set_source_surface(cr, PRIVATE(self)->iconSurface, 0, 0);
			cairo_paint(cr);
		}
	}
	return TRUE;
}

void cmk_icon_set_icon(CmkIcon *self, const gchar *iconName)
{
	g_return_if_fail(CMK_IS_ICON(self));
	g_free(PRIVATE(self)->iconName);
	PRIVATE(self)->iconName = g_strdup(iconName);
	PRIVATE(self)->setPixmap = FALSE;
	queue_update_canvas(self);
}

const gchar * cmk_icon_get_icon(CmkIcon *self)
{
	g_return_val_if_fail(CMK_IS_ICON(self), NULL);
	if(PRIVATE(self)->setPixmap)
		return NULL;
	return PRIVATE(self)->iconName;
}

void cmk_icon_set_pixmap(CmkIcon *self, guchar *data, cairo_format_t format, guint size, guint frames, guint fps)
{
	CmkIconPrivate *private = PRIVATE(self);
	g_clear_pointer(&private->iconSurface, cairo_surface_destroy);
	cairo_surface_t *surface = cairo_image_surface_create_for_data(data, format, size, size, cairo_format_stride_for_width(format, size));
	private->iconSurface = surface;
	private->setPixmap = TRUE;
	queue_update_canvas(self);
}

void cmk_icon_set_size(CmkIcon *self, gfloat size)
{
	g_return_if_fail(CMK_IS_ICON(self));
	if(PRIVATE(self)->size != size)
	{
		if(size <= 0)
			size = 0;
		PRIVATE(self)->size = size;
		queue_update_canvas(self);
		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));
	}
}

gfloat cmk_icon_get_size(CmkIcon *self)
{
	g_return_val_if_fail(CMK_IS_ICON(self), 0);
	return PRIVATE(self)->size;
}

void cmk_icon_set_use_foreground_color(CmkIcon *self, gboolean useForeground)
{
	g_return_if_fail(CMK_IS_ICON(self));
	if(PRIVATE(self)->useForegroundColor != useForeground)
	{
		PRIVATE(self)->useForegroundColor = useForeground;
		queue_update_canvas(self);
	}
}

gboolean cmk_icon_get_use_foreground_color(CmkIcon *self)
{
	g_return_val_if_fail(CMK_IS_ICON(self), FALSE);
	return PRIVATE(self)->useForegroundColor;
}

void cmk_icon_set_icon_theme(CmkIcon *self, const gchar *themeName)
{
	g_return_if_fail(CMK_IS_ICON(self));
	g_free(PRIVATE(self)->themeName);
	PRIVATE(self)->themeName = g_strdup(themeName);
	queue_update_canvas(self);
}

const gchar * cmk_icon_get_icon_theme(CmkIcon *self)
{
	g_return_val_if_fail(CMK_IS_ICON(self), NULL);
	return PRIVATE(self)->themeName;
}
