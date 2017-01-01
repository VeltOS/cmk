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
};

enum
{
	PROP_STYLE = 1
};

enum
{
	SIGNAL_STYLE_CHANGED = 1,
	SIGNAL_LAST
};

static guint signals[SIGNAL_LAST];

static void cmk_widget_dispose(GObject *self_);
static void cmk_widget_set_property(GObject *self_, guint propertyId, const GValue *value, GParamSpec *pspec);
static void cmk_widget_get_property(GObject *self_, guint propertyId, GValue *value, GParamSpec *pspec);
static void on_style_changed(CMKWidget *self, CMKStyle *style);

G_DEFINE_TYPE_WITH_PRIVATE(CMKWidget, cmk_widget, CLUTTER_TYPE_ACTOR);
#define PRIVATE(widget) ((CMKWidgetPrivate *)cmk_widget_get_instance_private(widget))

static void cmk_widget_class_init(CMKWidgetClass *class)
{
	GObjectClass *base = G_OBJECT_CLASS(class);
	base->get_property = cmk_widget_get_property;
	base->set_property = cmk_widget_set_property;
	base->dispose = cmk_widget_dispose;
	
	g_object_class_install_property(base, PROP_STYLE, g_param_spec_object("style", "style", "style", CMK_TYPE_STYLE, G_PARAM_READWRITE));
	
	signals[SIGNAL_STYLE_CHANGED] = g_signal_new("style-changed", G_TYPE_FROM_CLASS(class), G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET(CMKWidgetClass, style_changed), NULL, NULL, NULL, G_TYPE_NONE, 1, CMK_TYPE_STYLE);
}

static void cmk_widget_init(CMKWidget *self)
{
	PRIVATE(self)->style = cmk_style_get_default();
	
	// Relay the style's style-changed signal to the widget's signal
	g_signal_connect_swapped(PRIVATE(self)->style, "style-changed", G_CALLBACK(on_style_changed), self);
}

static void cmk_widget_dispose(GObject *self_)
{
	g_clear_object(&(PRIVATE(CMK_WIDGET(self_))->style));
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
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(self, propertyId, pspec);
		break;
	}
}

static void on_style_changed(CMKWidget *self, CMKStyle *style)
{
	g_signal_emit(self, signals[SIGNAL_STYLE_CHANGED], 0, style);
}

void cmk_widget_set_style(CMKWidget *self, CMKStyle *style)
{
	g_return_if_fail(CMK_IS_WIDGET(self));
	g_return_if_fail(CMK_IS_STYLE(style));
	g_signal_handlers_disconnect_by_func(PRIVATE(self)->style, G_CALLBACK(on_style_changed), self);
	g_clear_object(&(PRIVATE(self)->style));
	PRIVATE(self)->style = g_object_ref(style);
	g_signal_connect_swapped(style, "style-changed", G_CALLBACK(on_style_changed), self);
	on_style_changed(self, style);
}

CMKStyle * cmk_widget_get_style(CMKWidget *self)
{
	g_return_val_if_fail(CMK_IS_WIDGET(self), NULL);
	return PRIVATE(self)->style;
}
