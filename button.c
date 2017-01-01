/*
 * This file is part of graphene-desktop, the desktop environment of VeltOS.
 * Copyright (C) 2016 Velt Technologies, Aidan Shafran <zelbrium@gmail.com>
 * Licensed under the Apache License 2 <www.apache.org/licenses/LICENSE-2.0>.
 */

#include "button.h"
#include <math.h>

struct _CMKButton
{
	ClutterActor parent;
	ClutterText *text; // Owned by Clutter. Do not free.
	CMKStyle *style;
	gchar *backgroundColorName;
};

enum
{
	PROP_TEXT = 1,
	PROP_LAST
};

static GParamSpec *properties[PROP_LAST];

static void cmk_button_dispose(GObject *self_);
static void cmk_button_set_property(GObject *self_, guint propertyId, const GValue *value, GParamSpec *pspec);
static void cmk_button_get_property(GObject *self_, guint propertyId, GValue *value, GParamSpec *pspec);
static gboolean on_button_press(ClutterActor *actor, ClutterButtonEvent *event);
static gboolean on_button_release(ClutterActor *actor, ClutterButtonEvent *event);
static void on_style_changed(CMKWidget *self_, CMKStyle *style);
static void on_size_changed(ClutterActor *self, GParamSpec *spec, ClutterCanvas *canvas);
static gboolean on_draw_canvas(ClutterCanvas *canvas, cairo_t *cr, int width, int height, CMKButton *self);
static void on_notify_pressed(CMKButton *self, GParamSpec *spec, ClutterClickAction *action);

G_DEFINE_TYPE(CMKButton, cmk_button, CMK_TYPE_WIDGET);



CMKButton * cmk_button_new()
{
	return CMK_BUTTON(g_object_new(CMK_TYPE_BUTTON, NULL));
}

CMKButton * cmk_button_new_with_text(const gchar *text)
{
	return CMK_BUTTON(g_object_new(CMK_TYPE_BUTTON, "text", text, NULL));
}

static void cmk_button_class_init(CMKButtonClass *class)
{
	GObjectClass *base = G_OBJECT_CLASS(class);
	base->dispose = cmk_button_dispose;
	base->set_property = cmk_button_set_property;
	base->get_property = cmk_button_get_property;

	ClutterActorClass *actorClass = CLUTTER_ACTOR_CLASS(class);
	actorClass->button_press_event = on_button_press;
	actorClass->button_release_event = on_button_release;

	CMK_WIDGET_CLASS(class)->style_changed = on_style_changed;
	
	properties[PROP_TEXT] = g_param_spec_string("text", "text", "text", "", G_PARAM_READWRITE);
	
	g_object_class_install_properties(base, PROP_LAST, properties);
}

static void cmk_button_init(CMKButton *self)
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

	//ClutterAction *action = clutter_click_action_new();
	//clutter_actor_add_action(actor, action);
	//g_signal_connect(action, "clicked", G_CALLBACK(on_clicked), NULL);
	//g_signal_connect_swapped(action, "notify::pressed", G_CALLBACK(on_notify_pressed), self);

	self->text = CLUTTER_TEXT(clutter_text_new());
	clutter_actor_add_child(actor, CLUTTER_ACTOR(self->text));

	self->backgroundColorName = g_strdup("background");
	on_style_changed(CMK_WIDGET(self), cmk_widget_get_style(CMK_WIDGET(self)));
}

static void cmk_button_dispose(GObject *self_)
{
	g_clear_object(&(CMK_BUTTON(self_)->style));
	g_clear_pointer(&(CMK_BUTTON(self_)->backgroundColorName), g_free);
	G_OBJECT_CLASS(cmk_button_parent_class)->dispose(self_);
}

static void cmk_button_set_property(GObject *self_, guint propertyId, const GValue *value, GParamSpec *pspec)
{
	g_return_if_fail(CMK_IS_BUTTON(self_));
	CMKButton *self = CMK_BUTTON(self_);
	
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
	CMKButton *self = CMK_BUTTON(self_);
	
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

static gboolean on_button_press(ClutterActor *actor, ClutterButtonEvent *event)
{
	//clutter_input_device_grab(event->device, actor);
	return TRUE;
}

static gboolean on_button_release(ClutterActor *actor, ClutterButtonEvent *event)
{
	//clutter_input_device_ungrab(event->device);
	return TRUE;
}

static void on_style_changed(CMKWidget *self_, CMKStyle *style)
{
	clutter_content_invalidate(clutter_actor_get_content(CLUTTER_ACTOR(self_)));
	float padding = cmk_style_get_padding(style);
	ClutterMargin margin = {padding, padding, padding, padding};
	clutter_actor_set_margin(CLUTTER_ACTOR(CMK_BUTTON(self_)->text), &margin);
	
	CMKColor color;
	cmk_style_get_font_color_for_background(style, "background", &color);
	ClutterColor cc = cmk_to_clutter_color(&color);
	clutter_text_set_color(CMK_BUTTON(self_)->text, &cc);
}

static void on_size_changed(ClutterActor *self, GParamSpec *spec, ClutterCanvas *canvas)
{
	gfloat width, height;
	clutter_actor_get_size(self, &width, &height);
	clutter_canvas_set_size(CLUTTER_CANVAS(canvas), width, height);
}

static gboolean on_draw_canvas(ClutterCanvas *canvas, cairo_t *cr, int width, int height, CMKButton *self)
{
	CMKStyle *style = cmk_widget_get_style(CMK_WIDGET(self));
	double radius = cmk_style_get_bevel_radius(style);
	double degrees = M_PI / 180.0;

	cairo_save(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
	cairo_paint(cr);
	cairo_restore(cr);
	return TRUE;

	cairo_new_sub_path(cr);
	cairo_arc(cr, width - radius, radius, radius, -90 * degrees, 0 * degrees);
	cairo_arc(cr, width - radius, height - radius, radius, 0 * degrees, 90 * degrees);
	cairo_arc(cr, radius, height - radius, radius, 90 * degrees, 180 * degrees);
	cairo_arc(cr, radius, radius, radius, 180 * degrees, 270 * degrees);
	cairo_close_path(cr);

	cairo_set_source_cmk_color(cr, cmk_style_get_color(style, "primary"));
	cairo_fill(cr);
	return TRUE;
}

static void on_notify_pressed(CMKButton *self, GParamSpec *spec, ClutterClickAction *action)
{
	gboolean pressed;
	g_object_get(action, "pressed", &pressed, NULL);
	g_message("pressed: %i", pressed);
}

void cmk_button_set_text(CMKButton *self, const gchar *text)
{
	g_return_if_fail(CMK_IS_BUTTON(self));
	clutter_text_set_text(self->text, text);
}

const gchar * cmk_button_get_text(CMKButton *self)
{
	g_return_val_if_fail(CMK_IS_BUTTON(self), NULL);
	return clutter_text_get_text(self->text);
}

void cmk_button_set_background_color_name(CMKButton *self, const gchar *name)
{
	g_return_if_fail(CMK_IS_BUTTON(self));
	self->backgroundColorName = g_strdup(name);
	CMKColor color;
	cmk_style_get_font_color_for_background(cmk_widget_get_style(CMK_WIDGET(self)), name, &color);
	ClutterColor cc = cmk_to_clutter_color(&color);
	clutter_text_set_color(self->text, &cc);
}

const gchar * cmk_button_get_name(CMKButton *self)
{
	g_return_val_if_fail(CMK_IS_BUTTON(self), NULL);
	const gchar *name = clutter_actor_get_name(CLUTTER_ACTOR(self));
	if(name)
		return name;
	return cmk_button_get_text(self);
}
