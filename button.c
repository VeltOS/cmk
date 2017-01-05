/*
 * This file is part of graphene-desktop, the desktop environment of VeltOS.
 * Copyright (C) 2016 Velt Technologies, Aidan Shafran <zelbrium@gmail.com>
 * Licensed under the Apache License 2 <www.apache.org/licenses/LICENSE-2.0>.
 */

#include "button.h"
#include <math.h>

typedef struct _CmkButtonPrivate CmkButtonPrivate;
struct _CmkButtonPrivate 
{
	ClutterText *text; // Owned by Clutter. Do not free.
	gboolean hover;
};

enum
{
	PROP_TEXT = 1,
	PROP_LAST
};

enum
{
	SIGNAL_ACTIVATE = 1,
	SIGNAL_LAST
};

static GParamSpec *properties[PROP_LAST];
static guint signals[SIGNAL_LAST];

static void cmk_button_set_property(GObject *self_, guint propertyId, const GValue *value, GParamSpec *pspec);
static void cmk_button_get_property(GObject *self_, guint propertyId, GValue *value, GParamSpec *pspec);
static void on_clicked(ClutterClickAction *action, CmkButton *self);
static gboolean on_crossing(ClutterActor *self_, ClutterCrossingEvent *event);
static void on_style_changed(CmkWidget *self_, CmkStyle *style);
static void on_background_changed(CmkWidget *self_);
static void on_size_changed(ClutterActor *self, GParamSpec *spec, ClutterCanvas *canvas);
static gboolean on_draw_canvas(ClutterCanvas *canvas, cairo_t *cr, int width, int height, CmkButton *self);

G_DEFINE_TYPE_WITH_PRIVATE(CmkButton, cmk_button, CMK_TYPE_WIDGET);
#define PRIVATE(button) ((CmkButtonPrivate *)cmk_button_get_instance_private(button))



CmkButton * cmk_button_new()
{
	return CMK_BUTTON(g_object_new(CMK_TYPE_BUTTON, NULL));
}

CmkButton * cmk_button_new_with_text(const gchar *text)
{
	return CMK_BUTTON(g_object_new(CMK_TYPE_BUTTON, "text", text, NULL));
}

static void cmk_button_class_init(CmkButtonClass *class)
{
	GObjectClass *base = G_OBJECT_CLASS(class);
	base->set_property = cmk_button_set_property;
	base->get_property = cmk_button_get_property;

	ClutterActorClass *actorClass = CLUTTER_ACTOR_CLASS(class);
	actorClass->enter_event = on_crossing;
	actorClass->leave_event = on_crossing;

	CMK_WIDGET_CLASS(class)->style_changed = on_style_changed;
	CMK_WIDGET_CLASS(class)->background_changed = on_background_changed;
	
	properties[PROP_TEXT] = g_param_spec_string("text", "text", "text", "", G_PARAM_READWRITE);
	
	g_object_class_install_properties(base, PROP_LAST, properties);

	signals[SIGNAL_ACTIVATE] = g_signal_new("activate", G_TYPE_FROM_CLASS(class), G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(CmkButtonClass, activate), NULL, NULL, NULL, G_TYPE_NONE, 0);
}

static void cmk_button_init(CmkButton *self)
{
	ClutterContent *canvas = clutter_canvas_new();
	g_signal_connect(canvas, "draw", G_CALLBACK(on_draw_canvas), self);

	ClutterActor *actor = CLUTTER_ACTOR(self);
	clutter_actor_set_reactive(actor, TRUE);

	ClutterLayoutManager *layout = clutter_bin_layout_new(CLUTTER_BIN_ALIGNMENT_CENTER, CLUTTER_BIN_ALIGNMENT_CENTER);
	clutter_actor_set_layout_manager(actor, layout);
	g_signal_connect(actor, "notify::size", G_CALLBACK(on_size_changed), canvas);

	clutter_actor_set_content_gravity(actor, CLUTTER_CONTENT_GRAVITY_CENTER);
	clutter_actor_set_content(actor, canvas);

	PRIVATE(self)->text = CLUTTER_TEXT(clutter_text_new());
	clutter_actor_add_child(actor, CLUTTER_ACTOR(PRIVATE(self)->text));

	// This handles grabbing the cursor when the user holds down the mouse
	ClutterAction *action = clutter_click_action_new();
	g_signal_connect(action, "clicked", G_CALLBACK(on_clicked), NULL);
	clutter_actor_add_action(actor, action);
}

static void cmk_button_set_property(GObject *self_, guint propertyId, const GValue *value, GParamSpec *pspec)
{
	g_return_if_fail(CMK_IS_BUTTON(self_));
	CmkButton *self = CMK_BUTTON(self_);
	
	switch(propertyId)
	{
	case PROP_TEXT:
		cmk_button_set_text(self, g_value_get_string(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(self, propertyId, pspec);
		break;
	}
}

static void cmk_button_get_property(GObject *self_, guint propertyId, GValue *value, GParamSpec *pspec)
{
	g_return_if_fail(CMK_IS_BUTTON(self_));
	CmkButton *self = CMK_BUTTON(self_);
	
	switch(propertyId)
	{
	case PROP_TEXT:
		g_value_set_string(value, cmk_button_get_text(self));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(self, propertyId, pspec);
		break;
	}
}

static void on_clicked(ClutterClickAction *action, CmkButton *self)
{
	g_signal_emit(self, signals[SIGNAL_ACTIVATE], 0);
}

static gboolean on_crossing(ClutterActor *self_, ClutterCrossingEvent *event)
{
	if(event->type == CLUTTER_ENTER)
		PRIVATE(CMK_BUTTON(self_))->hover = TRUE;
	else
		PRIVATE(CMK_BUTTON(self_))->hover = FALSE;
	
	clutter_content_invalidate(clutter_actor_get_content(self_));
	return TRUE;
}

static void on_style_changed(CmkWidget *self_, CmkStyle *style)
{
	clutter_content_invalidate(clutter_actor_get_content(CLUTTER_ACTOR(self_)));
	float padding = cmk_style_get_padding(style);
	ClutterMargin margin = {padding, padding, padding, padding};
	clutter_actor_set_margin(CLUTTER_ACTOR(PRIVATE(CMK_BUTTON(self_))->text), &margin);
	
	CMK_WIDGET_CLASS(cmk_button_parent_class)->style_changed(self_, style);
}

static void on_background_changed(CmkWidget *self_)
{
	const gchar *background = cmk_widget_get_background_color(self_);
	CmkColor color;
	cmk_style_get_font_color_for_background(cmk_widget_get_actual_style(self_), background, &color);
	ClutterColor cc = cmk_to_clutter_color(&color);
	clutter_text_set_color(PRIVATE(CMK_BUTTON(self_))->text, &cc);
}

static void on_size_changed(ClutterActor *self, GParamSpec *spec, ClutterCanvas *canvas)
{
	gfloat width, height;
	clutter_actor_get_size(self, &width, &height);
	clutter_canvas_set_size(CLUTTER_CANVAS(canvas), width, height);
}

static gboolean on_draw_canvas(ClutterCanvas *canvas, cairo_t *cr, int width, int height, CmkButton *self)
{
	CmkStyle *style = cmk_widget_get_actual_style(CMK_WIDGET(self));
	double radius = cmk_style_get_bevel_radius(style);
	double degrees = M_PI / 180.0;

	cairo_save(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
	cairo_paint(cr);
	cairo_restore(cr);

	if(PRIVATE(self)->hover)
	{
		cairo_new_sub_path(cr);
		cairo_arc(cr, width - radius, radius, radius, -90 * degrees, 0 * degrees);
		cairo_arc(cr, width - radius, height - radius, radius, 0 * degrees, 90 * degrees);
		cairo_arc(cr, radius, height - radius, radius, 90 * degrees, 180 * degrees);
		cairo_arc(cr, radius, radius, radius, 180 * degrees, 270 * degrees);
		cairo_close_path(cr);

		cairo_set_source_cmk_color(cr, cmk_style_get_color(style, "hover"));
		cairo_fill(cr);
	}
	return TRUE;
}

void cmk_button_set_text(CmkButton *self, const gchar *text)
{
	g_return_if_fail(CMK_IS_BUTTON(self));
	clutter_text_set_text(PRIVATE(self)->text, text);
}

const gchar * cmk_button_get_text(CmkButton *self)
{
	g_return_val_if_fail(CMK_IS_BUTTON(self), NULL);
	return clutter_text_get_text(PRIVATE(self)->text);
}

const gchar * cmk_button_get_name(CmkButton *self)
{
	g_return_val_if_fail(CMK_IS_BUTTON(self), NULL);
	const gchar *name = clutter_actor_get_name(CLUTTER_ACTOR(self));
	if(name)
		return name;
	return cmk_button_get_text(self);
}
