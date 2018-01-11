/*
 * libcmk, Copyright 2018 Aidan Shafran <zelbrium@gmail.com>
 * Apache License 2.0 <www.apache.org/licenses/LICENSE-2.0>.
 */

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include "cmk-icon.h"
#include "cmk-icon-loader.h"

#define CMK_ICON_EVENT_MASK 0

typedef struct _CmkIconPrivate CmkIconPrivate;
struct _CmkIconPrivate
{
	CmkIconLoader *loader;

	char *iconName;
	char *themeName;
	float size;
	bool useForegroundColor;

	cairo_surface_t *surface;
	// scale used at time of surface creation
	unsigned int surfaceScale;
	unsigned int surfaceSize; // not scaled

	const CmkColor *foregroundColor;
};

enum
{
	PROP_ICON_NAME = 1,
	PROP_THEME_NAME,
	PROP_SIZE,
	PROP_USE_FOREGROUND_COLOR,
	PROP_LAST,
	PROP_OVERRIDE_EVENT_MASK
};

static GParamSpec *properties[PROP_LAST];

G_DEFINE_TYPE_WITH_PRIVATE(CmkIcon, cmk_icon, CMK_TYPE_WIDGET);
#define PRIV(icon) ((CmkIconPrivate *)cmk_icon_get_instance_private(icon))

static void on_dispose(GObject *self_);
static void set_property(GObject *self_, guint id, const GValue *value, GParamSpec *pspec);
static void get_property(GObject *self_, guint id, GValue *value, GParamSpec *pspec);
static void on_draw(CmkWidget *self_, cairo_t *cr);
static void get_preferred_width(CmkWidget *self_, float forHeight, float *min, float *nat);
static void get_preferred_height(CmkWidget *self_, float forWidth, float *min, float *nat);
static void get_draw_rect(CmkWidget *self_, CmkRect *rect);
static void on_default_icon_theme_changed(CmkIcon *self);
static void on_palette_changed(CmkIcon *self);


CmkIcon * cmk_icon_new(const char *iconName, float size)
{
	return CMK_ICON(g_object_new(CMK_TYPE_ICON,
		"icon-name", iconName,
		"size", size,
		NULL));
}

CmkIcon * cmk_icon_new_full(const char *iconName, const char *theme, float size, bool useForeground)
{
	return CMK_ICON(g_object_new(CMK_TYPE_ICON,
		"icon-name", iconName,
		"theme-name", theme,
		"size", size,
		"use-foreground-color", useForeground,
		NULL));
}

static void cmk_icon_class_init(CmkIconClass *class)
{
	GObjectClass *base = G_OBJECT_CLASS(class);
	base->dispose = on_dispose;
	base->get_property = get_property;
	base->set_property = set_property;

	CmkWidgetClass *widgetClass = CMK_WIDGET_CLASS(class);
	widgetClass->draw = on_draw;
	widgetClass->get_preferred_width = get_preferred_width;
	widgetClass->get_preferred_height = get_preferred_height;
	widgetClass->get_draw_rect = get_draw_rect;

	/**
	 * CmkIcon:icon-name:
	 *
	 * The name of the icon.
	 */
	properties[PROP_ICON_NAME] =
		g_param_spec_string("icon-name", "icon name", "icon name",
		                    NULL,
		                    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

	/**
	 * CmkIcon:theme-name:
	 *
	 * The name of the icon theme.
	 */
	properties[PROP_THEME_NAME] =
		g_param_spec_string("theme-name", "theme name", "theme name",
		                    NULL,
		                    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

	/**
	 * CmkIcon:size:
	 *
	 * Size of the icon in Cairo units.
	 */
	properties[PROP_SIZE] =
		g_param_spec_float("size", "size", "size",
		                   0, G_MAXFLOAT, 0,
		                   G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

	/**
	 * CmkIcon:use-foreground-color:
	 *
	 * Use the icon as a mask for the foreground color.
	 */
	properties[PROP_USE_FOREGROUND_COLOR] =
		g_param_spec_boolean("use-foreground-color", "use foreground", "use foreground",
		                     false,
		                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

	g_object_class_install_properties(base, PROP_LAST, properties);

	g_object_class_override_property(base, PROP_OVERRIDE_EVENT_MASK, "event-mask");
}

static void cmk_icon_init(CmkIcon *self)
{
	CmkIconPrivate *priv = PRIV(self);

	priv->loader = cmk_icon_loader_get_default();
	g_signal_connect_swapped(priv->loader,
	                 "notify::default-theme",
	                 G_CALLBACK(on_default_icon_theme_changed),
	                 self);

	g_signal_connect(self,
	                 "notify::palette",
	                 G_CALLBACK(on_palette_changed),
	                 NULL);
}

static void on_dispose(GObject *self_)
{
	CmkIconPrivate *priv = PRIV(CMK_ICON(self_));
	g_clear_pointer(&priv->iconName, g_free);
	g_clear_pointer(&priv->themeName, g_free);
	G_OBJECT_CLASS(cmk_icon_parent_class)->dispose(self_);
}

static void get_property(GObject *self_, guint id, GValue *value, GParamSpec *pspec)
{
	CmkIcon *self = CMK_ICON(self_);
	
	switch(id)
	{
	case PROP_ICON_NAME:
		g_value_set_string(value, PRIV(self)->iconName);
		break;
	case PROP_THEME_NAME:
		g_value_set_string(value, PRIV(self)->themeName);
		break;
	case PROP_SIZE:
		g_value_set_float(value, PRIV(self)->size);
		break;
	case PROP_USE_FOREGROUND_COLOR:
		g_value_set_boolean(value, PRIV(self)->useForegroundColor);
		break;
	case PROP_OVERRIDE_EVENT_MASK:
		g_value_set_uint(value, CMK_ICON_EVENT_MASK);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(self, id, pspec);
		break;
	}
}

static void set_property(GObject *self_, guint id, const GValue *value, GParamSpec *pspec)
{
	CmkIcon *self = CMK_ICON(self_);
	
	switch(id)
	{
	case PROP_ICON_NAME:
		cmk_icon_set_icon(self, g_value_get_string(value));
		break;
	case PROP_THEME_NAME:
		cmk_icon_set_theme(self, g_value_get_string(value));
		break;
	case PROP_SIZE:
		cmk_icon_set_size(self, g_value_get_float(value));
		break;
	case PROP_USE_FOREGROUND_COLOR:
		cmk_icon_set_use_foreground_color(self, g_value_get_boolean(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(self, id, pspec);
		break;
	}
}

static void clear_cache(CmkIcon *self)
{
	CmkIconPrivate *priv = PRIV(self);
	g_clear_pointer(&priv->surface, cairo_surface_destroy);
}

static void ensure_surface(CmkIcon *self, cairo_t *cr)
{
	CmkIconPrivate *priv = PRIV(self);

	// Use the device scale as the scale for the icon,
	// as device scale is usually set based on system
	// DPI scale.
	cairo_surface_t *target = cairo_get_target(cr);
	double xdscale, ydscale;
	cairo_surface_get_device_scale(target, &xdscale, &ydscale);
	float dscale = MAX(fabs(xdscale), fabs(ydscale));
	unsigned int iconScale = round(dscale);

	// Modify icon size based on user scale (and any
	// non-integer part of the device scale)
	cairo_matrix_t mat;
	cairo_get_matrix(cr, &mat);
	float xuscale = sqrt(mat.xx * mat.xx + mat.xy * mat.xy);
	float yuscale = sqrt(mat.yx * mat.yx + mat.yy * mat.yy);
	float uscale = MAX(xuscale, yuscale) * (dscale / iconScale);

	unsigned int size = round(priv->size * uscale);

	// If these are the same, the icon is already loaded.
	// No need to check name and theme because priv->surface
	// is cleared when those change.
	if(priv->surface
	&& priv->surfaceSize == size
	&& priv->surfaceScale == iconScale)
		return;

	clear_cache(self);

	char *path = cmk_icon_loader_lookup_full(priv->loader,
		priv->iconName, true,
		priv->themeName, true,
		size, iconScale);

	if(!path)
	{
		path = cmk_icon_loader_lookup_full(priv->loader,
			"gtk-missing-image", true,
			priv->themeName, true,
			size, iconScale);
	}

	if(path)
	{
		priv->surface = cmk_icon_loader_load(priv->loader,
			path, size, iconScale, true);
		g_free(path);
	}
}

static void on_draw(CmkWidget *self_, cairo_t *cr)
{
	CmkIconPrivate *priv = PRIV(CMK_ICON(self_));

	// Make sure the icon (priv->surace) is loaded
	ensure_surface(CMK_ICON(self_), cr);

	if(!priv->surface)
		return;

	float width, height;
	cmk_widget_get_size(self_, &width, &height);

	// Center icon
	cairo_translate(cr,
		width / 2 - priv->size / 2,
		height / 2 - priv->size / 2);

	// Scale the icon to priv->size, as it may be larger or smaller
	// based on the loaded icon scale and what icons were available.
	float scale = priv->size / cairo_image_surface_get_height(priv->surface);
	cairo_scale(cr, scale, scale);

	if(priv->useForegroundColor)
	{
		cmk_cairo_set_source_color(cr, priv->foregroundColor);
		cairo_mask_surface(cr, priv->surface, 0, 0);
	}
	else
	{
		cairo_set_source_surface(cr, priv->surface, 0, 0);
		cairo_paint(cr);
	}
}

static void get_preferred_width(CmkWidget *self_, UNUSED float forHeight, float *min, float *nat)
{
	CmkIconPrivate *priv = PRIV(CMK_ICON(self_));
	*min = *nat = priv->size;
}

static void get_preferred_height(CmkWidget *self_, UNUSED float forWidth, float *min, float *nat)
{
	CmkIconPrivate *priv = PRIV(CMK_ICON(self_));
	*min = *nat = priv->size;
}

static void get_draw_rect(CmkWidget *self_, CmkRect *rect)
{
	CmkIconPrivate *priv = PRIV(CMK_ICON(self_));

	float width, height;
	cmk_widget_get_size(self_, &width, &height);

	rect->x = width / 2 - priv->size / 2;
	rect->y = height / 2 - priv->size / 2;
	rect->width = priv->size;
	rect->height = priv->size;
}

static void on_default_icon_theme_changed(CmkIcon *self)
{
	if(PRIV(self)->themeName == NULL)
		cmk_widget_invalidate(CMK_WIDGET(self), NULL);
}

static void on_palette_changed(CmkIcon *self)
{
	PRIV(self)->foregroundColor = cmk_widget_get_color(CMK_WIDGET(self), "foreground");
}

void cmk_icon_set_icon(CmkIcon *self, const char *iconName)
{
	g_return_if_fail(CMK_IS_ICON(self));
	CmkIconPrivate *priv = PRIV(self);

	g_free(priv->iconName);
	priv->iconName = g_strdup(iconName);
	clear_cache(self);
	cmk_widget_invalidate(CMK_WIDGET(self), NULL);
}

const char * cmk_icon_get_icon(CmkIcon *self)
{
	g_return_val_if_fail(CMK_IS_ICON(self), NULL);
	return PRIV(self)->iconName;
}

void cmk_icon_set_pixmap(CmkIcon *self, unsigned char *data, cairo_format_t format, unsigned int size, unsigned int frames, unsigned int fps)
{
	// TODO
}

void cmk_icon_set_size(CmkIcon *self, float size)
{
	g_return_if_fail(CMK_IS_ICON(self));
	CmkIconPrivate *priv = PRIV(self);

	size = MAX(size, 0);
	if(priv->size != size)
	{
		priv->size = size;
		clear_cache(self);
		cmk_widget_relayout(CMK_WIDGET(self));
	}
}

float cmk_icon_get_size(CmkIcon *self)
{
	g_return_val_if_fail(CMK_IS_ICON(self), 0);
	return PRIV(self)->size;
}

void cmk_icon_set_use_foreground_color(CmkIcon *self, bool useForeground)
{
	g_return_if_fail(CMK_IS_ICON(self));
	if(PRIV(self)->useForegroundColor != useForeground)
	{
		PRIV(self)->useForegroundColor = useForeground;
		cmk_widget_invalidate(CMK_WIDGET(self), NULL);
	}
}

bool cmk_icon_get_use_foreground_color(CmkIcon *self)
{
	g_return_val_if_fail(CMK_IS_ICON(self), FALSE);
	return PRIV(self)->useForegroundColor;
}

void cmk_icon_set_theme(CmkIcon *self, const char *theme)
{
	g_return_if_fail(CMK_IS_ICON(self));
	CmkIconPrivate *priv = PRIV(self);

	if(g_strcmp0(priv->themeName, theme) != 0)
	{
		g_free(PRIV(self)->themeName);
		PRIV(self)->themeName = g_strdup(theme);
		clear_cache(self);
		cmk_widget_invalidate(CMK_WIDGET(self), NULL);
	}
}

const char * cmk_icon_get_icon_theme(CmkIcon *self)
{
	g_return_val_if_fail(CMK_IS_ICON(self), NULL);
	return PRIV(self)->themeName;
}
