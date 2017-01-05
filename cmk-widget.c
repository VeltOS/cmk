/*
 * This file is part of graphene-desktop, the desktop environment of VeltOS.
 * Copyright (C) 2016 Velt Technologies, Aidan Shafran <zelbrium@gmail.com>
 * Licensed under the Apache License 2 <www.apache.org/licenses/LICENSE-2.0>.
 */

#include "cmk-widget.h"

typedef struct _CmkWidgetPrivate CmkWidgetPrivate;
struct _CmkWidgetPrivate
{
	CmkStyle *style;
	CmkStyle *actualStyle;
	CmkWidget *styleParent; // Not ref'd
	CmkWidget *actualStyleParent; // styleParent, real parent, or NULL (not ref'd)
	gchar *backgroundColorName;
	gboolean drawBackground;
	gboolean disposed;
};

enum
{
	PROP_STYLE = 1,
	PROP_BACKGROUND_COLOR_NAME,
	PROP_LAST
};

enum
{
	SIGNAL_STYLE_CHANGED = 1,
	SIGNAL_BACKGROUND_CHANGED,
	SIGNAL_LAST
};

static GParamSpec *properties[PROP_LAST];
static guint signals[SIGNAL_LAST];

static void cmk_widget_dispose(GObject *self_);
static void cmk_widget_set_property(GObject *self_, guint propertyId, const GValue *value, GParamSpec *pspec);
static void cmk_widget_get_property(GObject *self_, guint propertyId, GValue *value, GParamSpec *pspec);
static void emit_style_changed(CmkWidget *self);
static void update_actual_style(CmkWidget *self);
static void on_parent_background_changed(CmkWidget *self, CmkWidget *parent);
static void on_parent_set(ClutterActor *self_, ClutterActor *prevParent);
static void on_style_changed(CmkWidget *self, CmkStyle *style);
static void on_style_parent_style_object_changed(CmkWidget *self, GParamSpec *spec, CmkWidget *parent);
static void on_style_parent_background_changed(CmkWidget *self, CmkWidget *parent);
static void on_style_parent_destroy(CmkWidget *self, CmkWidget *parent);
static void on_background_changed(CmkWidget *self);
static void update_named_background_color(CmkWidget *self);

G_DEFINE_TYPE_WITH_PRIVATE(CmkWidget, cmk_widget, CLUTTER_TYPE_ACTOR);
#define PRIVATE(widget) ((CmkWidgetPrivate *)cmk_widget_get_instance_private(widget))

CmkWidget * cmk_widget_new()
{
	return CMK_WIDGET(g_object_new(CMK_TYPE_WIDGET, NULL));
}

static void cmk_widget_class_init(CmkWidgetClass *class)
{
	GObjectClass *base = G_OBJECT_CLASS(class);
	base->dispose = cmk_widget_dispose;
	base->get_property = cmk_widget_get_property;
	base->set_property = cmk_widget_set_property;
	
	CLUTTER_ACTOR_CLASS(class)->parent_set = on_parent_set;

	class->style_changed = on_style_changed;
	class->background_changed = on_background_changed;

	properties[PROP_STYLE] = g_param_spec_object("style", "style", "style", CMK_TYPE_STYLE, G_PARAM_READWRITE|G_PARAM_CONSTRUCT);
	properties[PROP_BACKGROUND_COLOR_NAME] = g_param_spec_string("background-color-name", "background-color-name", "Named background color using CmkStyle", NULL, G_PARAM_READWRITE);

	g_object_class_install_properties(base, PROP_LAST, properties);
	
	signals[SIGNAL_STYLE_CHANGED] = g_signal_new("style-changed", G_TYPE_FROM_CLASS(class), G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(CmkWidgetClass, style_changed), NULL, NULL, NULL, G_TYPE_NONE, 1, CMK_TYPE_STYLE);

	signals[SIGNAL_BACKGROUND_CHANGED] = g_signal_new("background-changed", G_TYPE_FROM_CLASS(class), G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(CmkWidgetClass, background_changed), NULL, NULL, NULL, G_TYPE_NONE, 0);
}

static void cmk_widget_init(CmkWidget *self)
{
}

static void cmk_widget_dispose(GObject *self_)
{
	CmkWidgetPrivate *private = PRIVATE(CMK_WIDGET(self_));
	
	ClutterActor *parent = clutter_actor_get_parent(CLUTTER_ACTOR(self_));	
	if(CMK_IS_WIDGET(parent))
		g_signal_handlers_disconnect_by_func(parent, G_CALLBACK(on_parent_background_changed), self_);

	if(private->actualStyle)
		g_signal_handlers_disconnect_by_func(private->actualStyle, G_CALLBACK(emit_style_changed), self_);
	if(private->actualStyleParent)
		g_signal_handlers_disconnect_by_func(private->actualStyleParent, G_CALLBACK(emit_style_changed), self_);
	
	if(private->styleParent)
	{
		g_signal_handlers_disconnect_by_func(private->styleParent, G_CALLBACK(on_style_parent_style_object_changed), self_);
		g_signal_handlers_disconnect_by_func(private->styleParent, G_CALLBACK(on_style_parent_background_changed), self_);
		g_signal_handlers_disconnect_by_func(private->styleParent, G_CALLBACK(on_style_parent_destroy), self_);
		private->styleParent = NULL;
	}

	g_clear_object(&private->style);
	g_clear_object(&private->actualStyle);
	g_clear_pointer(&private->backgroundColorName, g_free);
	private->disposed = TRUE;
	G_OBJECT_CLASS(cmk_widget_parent_class)->dispose(self_);
}

static void cmk_widget_set_property(GObject *self_, guint propertyId, const GValue *value, GParamSpec *pspec)
{
	g_return_if_fail(CMK_IS_WIDGET(self_));
	CmkWidget *self = CMK_WIDGET(self_);
	
	switch(propertyId)
	{
	case PROP_STYLE:
		cmk_widget_set_style(self, g_value_get_object(value));
		break;
	case PROP_BACKGROUND_COLOR_NAME:
		cmk_widget_set_background_color(self, g_value_get_string(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(self, propertyId, pspec);
		break;
	}
}

static void cmk_widget_get_property(GObject *self_, guint propertyId, GValue *value, GParamSpec *pspec)
{
	g_return_if_fail(CMK_IS_WIDGET(self_));
	CmkWidget *self = CMK_WIDGET(self_);
	
	switch(propertyId)
	{
	case PROP_STYLE:
		g_value_set_object(value, cmk_widget_get_actual_style(self));
		break;
	case PROP_BACKGROUND_COLOR_NAME:
		g_value_set_string(value, cmk_widget_get_background_color(self));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(self, propertyId, pspec);
		break;
	}
}

static void emit_style_changed(CmkWidget *self)
{
	if(!PRIVATE(self)->disposed)
	{
		g_signal_emit(self, signals[SIGNAL_STYLE_CHANGED], 0, PRIVATE(self)->actualStyle);
		g_signal_emit(self, signals[SIGNAL_BACKGROUND_CHANGED], 0);
	}
}

static void update_actual_style(CmkWidget *self)
{
	CmkWidgetPrivate *private = PRIVATE(self);
	if(private->disposed)
		return;	

	// Disconnect last update's handlers
	if(private->actualStyle)
		g_signal_handlers_disconnect_by_func(private->actualStyle, G_CALLBACK(emit_style_changed), self);
	if(private->actualStyleParent)
		g_signal_handlers_disconnect_by_func(private->actualStyleParent, G_CALLBACK(emit_style_changed), self);

	CmkStyle *prevActualStyle = private->actualStyle; // Keep pointer for comparison
	private->actualStyleParent = NULL;
	g_clear_object(&private->actualStyle);
	ClutterActor *parent = clutter_actor_get_parent(CLUTTER_ACTOR(self));

	// If the style has been explicitly set, use that
	if(private->style)
	{
		private->actualStyle = g_object_ref(private->style);
	}
	// Use the style parent's style if one exists
	else if(private->styleParent)
	{
		private->actualStyle = cmk_widget_get_actual_style(private->styleParent);
		if(private->actualStyle)
		{
			g_object_ref(private->actualStyle);
			private->actualStyleParent = private->styleParent;
		}
	}
	// Use the real parent's style if it is a widget
	else if(CMK_IS_WIDGET(parent))
	{
		private->actualStyle = cmk_widget_get_actual_style(CMK_WIDGET(parent));
		if(private->actualStyle)
		{
			g_object_ref(private->actualStyle);
			private->actualStyleParent = CMK_WIDGET(parent);
		}
	}

	// Otherwise, just use the default style
	if(!private->actualStyle)
		private->actualStyle = cmk_style_get_default();
	
	// Connect signal to watch for style changes
	if(private->actualStyleParent)
		g_signal_connect_swapped(private->actualStyleParent, "style-changed", G_CALLBACK(emit_style_changed), self);
	else
		g_signal_connect_swapped(private->actualStyle, "changed", G_CALLBACK(emit_style_changed), self);
	
	if(prevActualStyle != private->actualStyle)
	{
		g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_STYLE]);
		emit_style_changed(self);
	}
}

static void on_parent_background_changed(CmkWidget *self, CmkWidget *parent)
{
	g_return_if_fail(CMK_IS_WIDGET(self));
	// Only say we've changed colors if we're not drawing our own background
	// and there is no style parent to inherit background from
	if(!PRIVATE(self)->backgroundColorName && !PRIVATE(self)->styleParent)
		g_signal_emit(self, signals[SIGNAL_BACKGROUND_CHANGED], 0);
}

static void on_parent_set(ClutterActor *self_, ClutterActor *prevParent)
{
	g_return_if_fail(CMK_IS_WIDGET(self_));
	if(PRIVATE(CMK_WIDGET(self_))->disposed)
		return;

	update_actual_style(CMK_WIDGET(self_));

	if(CMK_IS_WIDGET(prevParent))
		g_signal_handlers_disconnect_by_func(prevParent, G_CALLBACK(on_parent_background_changed), self_);
	
	ClutterActor *newParent = clutter_actor_get_parent(self_);
	if(CMK_IS_WIDGET(newParent))
		g_signal_connect_swapped(newParent, "background-changed", G_CALLBACK(on_parent_background_changed), self_);
}

static void on_style_changed(CmkWidget *self, CmkStyle *style)
{
	g_return_if_fail(CMK_IS_WIDGET(self));
	g_return_if_fail(CMK_IS_STYLE(style));
	update_named_background_color(self);
}

void cmk_widget_set_style(CmkWidget *self, CmkStyle *style)
{
	g_return_if_fail(CMK_IS_WIDGET(self));
	g_return_if_fail(style == NULL || CMK_IS_STYLE(style));
	if(style)
		g_object_ref(style);
	g_clear_object(&(PRIVATE(self)->style));
	PRIVATE(self)->style = style;
	update_actual_style(self);
}

CmkStyle * cmk_widget_get_actual_style(CmkWidget *self)
{
	g_return_val_if_fail(CMK_IS_WIDGET(self), NULL);
	return PRIVATE(self)->actualStyle;
}

CmkStyle * cmk_widget_get_style(CmkWidget *self)
{
	g_return_val_if_fail(CMK_IS_WIDGET(self), NULL);
	return PRIVATE(self)->style;
}

static void on_style_parent_style_object_changed(CmkWidget *self, GParamSpec *spec, CmkWidget *parent)
{
	update_actual_style(self);
}

static void on_style_parent_background_changed(CmkWidget *self, CmkWidget *parent)
{
	g_return_if_fail(CMK_IS_WIDGET(self));
	// Only say we've changed colors if we're not drawing our own background
	if(!PRIVATE(self)->backgroundColorName)
		g_signal_emit(self, signals[SIGNAL_BACKGROUND_CHANGED], 0);
}

static void on_style_parent_destroy(CmkWidget *self, CmkWidget *parent)
{
	g_return_if_fail(CMK_IS_WIDGET(self));
	if(CMK_IS_WIDGET(parent))
	{
		g_signal_handlers_disconnect_by_func(parent, G_CALLBACK(on_style_parent_style_object_changed), self);
		g_signal_handlers_disconnect_by_func(parent, G_CALLBACK(on_style_parent_destroy), self);
	}
	PRIVATE(self)->styleParent = NULL;
	on_style_parent_style_object_changed(self, NULL, parent);
}

void cmk_widget_set_style_parent(CmkWidget *self, CmkWidget *parent)
{
	g_return_if_fail(CMK_IS_WIDGET(self));
	g_return_if_fail(CMK_IS_WIDGET(self) || parent == NULL);
	if(PRIVATE(self)->styleParent == parent)
		return;

	if(PRIVATE(self)->styleParent)
	{
		g_signal_handlers_disconnect_by_func(PRIVATE(self)->styleParent, G_CALLBACK(on_style_parent_style_object_changed), self);
		g_signal_handlers_disconnect_by_func(PRIVATE(self)->styleParent, G_CALLBACK(on_style_parent_background_changed), self);
		g_signal_handlers_disconnect_by_func(PRIVATE(self)->styleParent, G_CALLBACK(on_style_parent_destroy), self);
	}

	PRIVATE(self)->styleParent = parent;
	update_actual_style(self);

	if(parent)
	{
		g_signal_emit(self, signals[SIGNAL_BACKGROUND_CHANGED], 0);
		g_signal_connect_swapped(parent, "notify::style", G_CALLBACK(on_style_parent_style_object_changed), self);
		g_signal_connect_swapped(parent, "background-changed", G_CALLBACK(on_style_parent_background_changed), self);
		g_signal_connect_swapped(parent, "destroy", G_CALLBACK(on_style_parent_destroy), self);
	}
}

CmkWidget * cmk_widget_get_style_parent(CmkWidget *self)
{
	g_return_val_if_fail(CMK_IS_WIDGET(self), NULL);
	return PRIVATE(self)->styleParent;
}

void cmk_widget_set_background_color(CmkWidget *self, const gchar *namedColor)
{
	g_return_if_fail(CMK_IS_WIDGET(self));
	if(g_strcmp0(PRIVATE(self)->backgroundColorName, namedColor) == 0)
		return;
	g_clear_pointer(&(PRIVATE(self)->backgroundColorName), g_free);
	PRIVATE(self)->backgroundColorName = g_strdup(namedColor);
	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_BACKGROUND_COLOR_NAME]);
	g_signal_emit(self, signals[SIGNAL_BACKGROUND_CHANGED], 0);
}

/*
 * If this is set to not draw, then the widget should be drawing its own
 * background with the set color. So this shouldn't emit a background color
 * changed signal.
 */
void cmk_widget_set_draw_background_color(CmkWidget *self, gboolean draw)
{
	g_return_if_fail(CMK_IS_WIDGET(self));
	if(PRIVATE(self)->drawBackground == draw)
		return;
	PRIVATE(self)->drawBackground = draw;
	update_named_background_color(self);
}

const gchar * cmk_widget_get_background_color(CmkWidget *self)
{
	g_return_val_if_fail(CMK_IS_WIDGET(self), NULL);
	CmkWidgetPrivate *private = PRIVATE(self);
	
	if(private->backgroundColorName)
		return private->backgroundColorName;

	if(private->styleParent)
		return cmk_widget_get_background_color(private->styleParent);

	ClutterActor *parent = clutter_actor_get_parent(CLUTTER_ACTOR(self));
	if(CMK_IS_WIDGET(parent))
		return cmk_widget_get_background_color(CMK_WIDGET(parent));

	return NULL;
}

static void on_background_changed(CmkWidget *self)
{
	g_return_if_fail(CMK_IS_WIDGET(self));
	update_named_background_color(self);
}

static void update_named_background_color(CmkWidget *self)
{
	if(PRIVATE(self)->drawBackground && PRIVATE(self)->backgroundColorName)
	{
		const CmkColor *color = cmk_style_get_color(PRIVATE(self)->actualStyle, PRIVATE(self)->backgroundColorName);
		ClutterColor cc = cmk_to_clutter_color(color);
		clutter_actor_set_background_color(CLUTTER_ACTOR(self), &cc);
	}
	else
		clutter_actor_set_background_color(CLUTTER_ACTOR(self), NULL);
}
