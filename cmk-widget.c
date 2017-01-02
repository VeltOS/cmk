/*
 * This file is part of graphene-desktop, the desktop environment of VeltOS.
 * Copyright (C) 2016 Velt Technologies, Aidan Shafran <zelbrium@gmail.com>
 * Licensed under the Apache License 2 <www.apache.org/licenses/LICENSE-2.0>.
 */

#include "cmk-widget.h"

typedef struct _CMKWidgetPrivate CMKWidgetPrivate;
struct _CMKWidgetPrivate
{
	CMKStyle *style;
	CMKWidget *styleParent;
	const gchar *backgroundColorName;
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
	SIGNAL_LAST
};

static GParamSpec *properties[PROP_LAST];
static guint signals[SIGNAL_LAST];

static void cmk_widget_dispose(GObject *self_);
static void cmk_widget_set_property(GObject *self_, guint propertyId, const GValue *value, GParamSpec *pspec);
static void cmk_widget_get_property(GObject *self_, guint propertyId, GValue *value, GParamSpec *pspec);
static void on_style_changed(CMKWidget *self, CMKStyle *style);
static void on_style_parent_destroy(CMKWidget *self, CMKWidget *parent);
static void update_named_background_color(CMKWidget *self);

G_DEFINE_TYPE_WITH_PRIVATE(CMKWidget, cmk_widget, CLUTTER_TYPE_ACTOR);
#define PRIVATE(widget) ((CMKWidgetPrivate *)cmk_widget_get_instance_private(widget))

CMKWidget * cmk_widget_new()
{
	return CMK_WIDGET(g_object_new(CMK_TYPE_WIDGET, NULL));
}

static void cmk_widget_class_init(CMKWidgetClass *class)
{
	GObjectClass *base = G_OBJECT_CLASS(class);
	base->get_property = cmk_widget_get_property;
	base->set_property = cmk_widget_set_property;
	base->dispose = cmk_widget_dispose;

	class->style_changed = on_style_changed;

	properties[PROP_STYLE] = g_param_spec_object("style", "style", "style", CMK_TYPE_STYLE, G_PARAM_READWRITE|G_PARAM_CONSTRUCT);
	properties[PROP_BACKGROUND_COLOR_NAME] = g_param_spec_string("background-color-name", "background-color-name", "Named background color using CMKStyle", NULL, G_PARAM_READWRITE);

	g_object_class_install_properties(base, PROP_LAST, properties);
	
	/*
	 * This signal is emitted whenever the styling changes in such a way that
	 * the widget should redraw itself. This includes any properties changing
	 * on the attached CMKStyle object, or the attached CMKStyle object being
	 * swapped for another, or after object init.
	 * To ONLY listen for the swapping of style objects, use the notify::style
	 * signal.
	 */
	signals[SIGNAL_STYLE_CHANGED] = g_signal_new("style-changed", G_TYPE_FROM_CLASS(class), G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(CMKWidgetClass, style_changed), NULL, NULL, NULL, G_TYPE_NONE, 1, CMK_TYPE_STYLE);
}

static void cmk_widget_init(CMKWidget *self)
{
	PRIVATE(self)->style = cmk_style_get_default();
}

static void cmk_widget_dispose(GObject *self_)
{
	g_clear_object(&(PRIVATE(CMK_WIDGET(self_))->style));
	on_style_parent_destroy(CMK_WIDGET(self_), PRIVATE(CMK_WIDGET(self_))->styleParent);
	G_OBJECT_CLASS(cmk_widget_parent_class)->dispose(self_);
}

static void cmk_widget_set_property(GObject *self_, guint propertyId, const GValue *value, GParamSpec *pspec)
{
	g_return_if_fail(CMK_IS_WIDGET(self_));
	CMKWidget *self = CMK_WIDGET(self_);
	
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
	CMKWidget *self = CMK_WIDGET(self_);
	
	switch(propertyId)
	{
	case PROP_STYLE:
		g_value_set_object(value, cmk_widget_get_style(self));
		break;
	case PROP_BACKGROUND_COLOR_NAME:
		g_value_set_string(value, cmk_widget_get_background_color(self));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(self, propertyId, pspec);
		break;
	}
}

static void on_style_changed(CMKWidget *self, CMKStyle *style)
{
	g_return_if_fail(CMK_IS_WIDGET(self));
	g_return_if_fail(CMK_IS_STYLE(style));
	update_named_background_color(self);
}

static void on_style_object_style_changed(CMKWidget *self, CMKStyle *style)
{
	// Relay style object's style changed signal to the widget's
	g_signal_emit(self, signals[SIGNAL_STYLE_CHANGED], 0, style);
}

CMKStyle * cmk_widget_get_style(CMKWidget *self)
{
	g_return_val_if_fail(CMK_IS_WIDGET(self), NULL);
	return PRIVATE(self)->style;
}

void cmk_widget_set_style(CMKWidget *self, CMKStyle *style)
{
	g_return_if_fail(CMK_IS_WIDGET(self));
	g_return_if_fail(style == NULL || CMK_IS_STYLE(style));

	if(PRIVATE(self)->style)
		g_signal_handlers_disconnect_by_func(PRIVATE(self)->style, G_CALLBACK(on_style_changed), self);

	g_clear_object(&(PRIVATE(self)->style));
	PRIVATE(self)->style = style ? g_object_ref(style) : cmk_style_get_default();
	g_signal_connect_swapped(PRIVATE(self)->style, "style-changed", G_CALLBACK(on_style_object_style_changed), self);

	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_STYLE]);
	g_signal_emit(self, signals[SIGNAL_STYLE_CHANGED], 0, PRIVATE(self)->style);
}

static void on_style_parent_style_object_changed(CMKWidget *self, GParamSpec *spec, CMKWidget *parent)
{
	cmk_widget_set_style(self, cmk_widget_get_style(parent));
}

static void on_style_parent_destroy(CMKWidget *self, CMKWidget *parent)
{
	g_return_if_fail(CMK_IS_WIDGET(self));
	if(CMK_IS_WIDGET(parent))
	{
		g_signal_handlers_disconnect_by_func(parent, G_CALLBACK(on_style_parent_style_object_changed), self);
		g_signal_handlers_disconnect_by_func(parent, G_CALLBACK(on_style_parent_destroy), self);
	}
	PRIVATE(self)->styleParent = NULL;
}

void cmk_widget_set_style_parent(CMKWidget *self, CMKWidget *parent)
{
	g_return_if_fail(CMK_IS_WIDGET(self));
	g_return_if_fail(CMK_IS_WIDGET(parent));
	PRIVATE(self)->styleParent = parent;
	g_signal_connect_swapped(parent, "notify::style", G_CALLBACK(on_style_parent_style_object_changed), self);
	g_signal_connect_swapped(parent, "destroy", G_CALLBACK(on_style_parent_destroy), self);
}

CMKWidget * cmk_widget_get_style_parent(CMKWidget *self)
{
	g_return_val_if_fail(CMK_IS_WIDGET(self), NULL);
	return PRIVATE(self)->styleParent;
}

void cmk_widget_set_background_color(CMKWidget *self, const gchar *namedColor)
{
	g_return_if_fail(CMK_IS_WIDGET(self));
	g_clear_pointer(&(PRIVATE(self)->backgroundColorName), g_free);
	PRIVATE(self)->backgroundColorName = g_strdup(namedColor);
	update_named_background_color(self);
	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_BACKGROUND_COLOR_NAME]);
}

const gchar * cmk_widget_get_background_color(CMKWidget *self)
{
	g_return_val_if_fail(CMK_IS_WIDGET(self), NULL);
	return PRIVATE(self)->backgroundColorName;
}

static void update_named_background_color(CMKWidget *self)
{
	if(!PRIVATE(self)->backgroundColorName)
		return;
	const CMKColor *color = cmk_style_get_color(PRIVATE(self)->style, PRIVATE(self)->backgroundColorName);
	ClutterColor cc = cmk_to_clutter_color(color);
	clutter_actor_set_background_color(CLUTTER_ACTOR(self), &cc);
}

