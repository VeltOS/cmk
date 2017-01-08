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
	CmkWidget *content;
	ClutterText *text; // Owned by Clutter. Do not free.
	gboolean hover;
	gboolean beveled;
};

enum
{
	PROP_TEXT = 1,
	PROP_BEVELED,
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
static void cmk_button_get_preferred_width(ClutterActor *self_, gfloat forHeight, gfloat *minWidth, gfloat *natWidth);
static void cmk_button_get_preferred_height(ClutterActor *self_, gfloat forWidth, gfloat *minHeight, gfloat *natHeight);
static void cmk_button_allocate(ClutterActor *self_, const ClutterActorBox *box, ClutterAllocationFlags flags);
static void on_clicked(ClutterClickAction *action, CmkButton *self);
static gboolean on_crossing(ClutterActor *self_, ClutterCrossingEvent *event);
static void on_style_changed(CmkWidget *self_, CmkStyle *style);
static void on_background_changed(CmkWidget *self_);
static void on_size_changed(ClutterActor *self, GParamSpec *spec, ClutterCanvas *canvas);
static gboolean on_draw_canvas(ClutterCanvas *canvas, cairo_t *cr, int width, int height, CmkButton *self);

G_DEFINE_TYPE_WITH_PRIVATE(CmkButton, cmk_button, CMK_TYPE_WIDGET);
#define PRIVATE(button) ((CmkButtonPrivate *)cmk_button_get_instance_private(button))



CmkButton * cmk_button_new(void)
{
	return CMK_BUTTON(g_object_new(CMK_TYPE_BUTTON, NULL));
}

CmkButton * cmk_button_new_with_text(const gchar *text)
{
	return CMK_BUTTON(g_object_new(CMK_TYPE_BUTTON, "text", text, NULL));
}

CmkButton * cmk_beveled_button_new(void)
{
	return CMK_BUTTON(g_object_new(CMK_TYPE_BUTTON, "beveled", TRUE, NULL));
}

CmkButton * cmk_beveled_button_new_with_text(const gchar *text)
{
	return CMK_BUTTON(g_object_new(CMK_TYPE_BUTTON, "text", text, "beveled", TRUE, NULL));
}

static void cmk_button_class_init(CmkButtonClass *class)
{
	GObjectClass *base = G_OBJECT_CLASS(class);
	base->set_property = cmk_button_set_property;
	base->get_property = cmk_button_get_property;

	ClutterActorClass *actorClass = CLUTTER_ACTOR_CLASS(class);
	actorClass->enter_event = on_crossing;
	actorClass->leave_event = on_crossing;
	actorClass->get_preferred_width = cmk_button_get_preferred_width;
	actorClass->get_preferred_height = cmk_button_get_preferred_height;
	actorClass->allocate = cmk_button_allocate;

	CMK_WIDGET_CLASS(class)->style_changed = on_style_changed;
	CMK_WIDGET_CLASS(class)->background_changed = on_background_changed;
	
	properties[PROP_TEXT] = g_param_spec_string("text", "text", "text", "", G_PARAM_READWRITE);
	properties[PROP_BEVELED] = g_param_spec_boolean("beveled", "beveled", "beveled", FALSE, G_PARAM_READWRITE);
	
	g_object_class_install_properties(base, PROP_LAST, properties);

	signals[SIGNAL_ACTIVATE] = g_signal_new("activate", G_TYPE_FROM_CLASS(class), G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(CmkButtonClass, activate), NULL, NULL, NULL, G_TYPE_NONE, 0);
}

static void cmk_button_init(CmkButton *self)
{
	ClutterContent *canvas = clutter_canvas_new();
	g_signal_connect(canvas, "draw", G_CALLBACK(on_draw_canvas), self);

	ClutterActor *actor = CLUTTER_ACTOR(self);
	clutter_actor_set_reactive(actor, TRUE);

	g_signal_connect(actor, "notify::size", G_CALLBACK(on_size_changed), canvas);

	clutter_actor_set_content_gravity(actor, CLUTTER_CONTENT_GRAVITY_CENTER);
	clutter_actor_set_content(actor, canvas);

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
	case PROP_BEVELED:
		cmk_button_set_beveled(self, g_value_get_boolean(value));
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
	case PROP_BEVELED:
		g_value_set_boolean(value, PRIVATE(self)->beveled);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(self, propertyId, pspec);
		break;
	}
}

static void cmk_button_get_preferred_width(ClutterActor *self_, gfloat forHeight, gfloat *minWidth, gfloat *natWidth)
{
	CmkButtonPrivate *private = PRIVATE(CMK_BUTTON(self_));
	*minWidth = 0;
	float padding = cmk_style_get_padding(cmk_widget_get_actual_style(CMK_WIDGET(self_)));

	if(private->content)
	{
		gfloat min, nat;
		clutter_actor_get_preferred_width(CLUTTER_ACTOR(private->content), forHeight, &min, &nat);
		*minWidth += nat;
	}
	
	if(private->content && private->text)
		*minWidth += padding;

	if(private->text)
	{
		gfloat min, nat;
		clutter_actor_get_preferred_width(CLUTTER_ACTOR(private->text), forHeight, &min, &nat);
		*minWidth += nat;
	}

	*minWidth += (padding*2);
	*natWidth = *minWidth;
}

static void cmk_button_get_preferred_height(ClutterActor *self_, gfloat forWidth, gfloat *minHeight, gfloat *natHeight)
{
	CmkButtonPrivate *private = PRIVATE(CMK_BUTTON(self_));
	*minHeight = 0;
	float padding = cmk_style_get_padding(cmk_widget_get_actual_style(CMK_WIDGET(self_)));
	
	if(private->content)
	{
		gfloat min, nat;
		clutter_actor_get_preferred_height(CLUTTER_ACTOR(private->content), forWidth, &min, &nat);
		*minHeight = nat;
	}

	if(private->text)
	{
		gfloat min, nat;
		clutter_actor_get_preferred_height(CLUTTER_ACTOR(private->text), forWidth, &min, &nat);
		*minHeight = MAX(nat, *minHeight);
	}

	*minHeight += (padding*2);
	*natHeight = *minHeight;
}

static void cmk_button_allocate(ClutterActor *self_, const ClutterActorBox *box, ClutterAllocationFlags flags)
{
	/*
	 * Goal is to get an allocation like this:
	 * ----------------------------
	 * |                          |  <- padding (hPad)
	 * |  [Con.] [Text         ]  |  <- minHeight
	 * |                          |
	 * ----------------------------
	 *	   ^---- minWidth ----^  ^wPad
	 * Where both Content and Text are optional (both, either, neither).
	 * If both are present, Content should get its natural width and Text
	 * gets the rest. Otherwise, the single child gets all the space except
	 * padding.
	 */

	CmkButtonPrivate *private = PRIVATE(CMK_BUTTON(self_));
	if(!private->content && !private->text)
	{
		CLUTTER_ACTOR_CLASS(cmk_button_parent_class)->allocate(self_, box, flags);
		return;
	}

	float padding = cmk_style_get_padding(cmk_widget_get_actual_style(CMK_WIDGET(self_)));
	
	gfloat minHeight, natHeight, minWidth, natWidth;
	clutter_actor_get_preferred_height(self_, -1, &minHeight, &natHeight);
	clutter_actor_get_preferred_width(self_, -1, &minWidth, &natWidth);

	gfloat maxHeight = box->y2 - box->y1;
	gfloat maxWidth = box->x2 - box->x1;
	gfloat hPad = MIN(MAX((maxHeight - (minHeight-(padding*2)))/2, 0), padding);
	gfloat wPad = MIN(MAX((maxWidth - (minWidth-(padding*2)))/2, 0), padding);

	if(private->content && private->text)
	{
		gfloat min, nat;
		clutter_actor_get_preferred_width(CLUTTER_ACTOR(private->content), -1, &min, &nat);
		//g_message("allocate with content, %f, %f", min, nat);
		gfloat contentRight = MIN(wPad+nat, maxWidth);
		ClutterActorBox contentBox = {wPad, hPad, contentRight, maxHeight-hPad};
		clutter_actor_allocate(CLUTTER_ACTOR(private->content), &contentBox, flags);
		gfloat textRight = MAX(contentRight+wPad, maxWidth-wPad);
		ClutterActorBox textBox = {contentRight+wPad, hPad, textRight, maxHeight-hPad};
		clutter_actor_allocate(CLUTTER_ACTOR(private->text), &textBox, flags);
	}
	else
	{
		ClutterActorBox contentBox = {wPad, hPad, maxWidth-wPad, maxHeight-hPad};
		if(private->content)
			clutter_actor_allocate(CLUTTER_ACTOR(private->content), &contentBox, flags);
		else
			clutter_actor_allocate(CLUTTER_ACTOR(private->text), &contentBox, flags);
	}

	CLUTTER_ACTOR_CLASS(cmk_button_parent_class)->allocate(self_, box, flags);
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
	//float padding = cmk_style_get_padding(style);
	//ClutterMargin margin = {padding, padding, padding, padding};
	//clutter_actor_set_margin(CLUTTER_ACTOR(PRIVATE(CMK_BUTTON(self_))->text), &margin);
	
	CMK_WIDGET_CLASS(cmk_button_parent_class)->style_changed(self_, style);
}

static void on_background_changed(CmkWidget *self_)
{
	const gchar *background = cmk_widget_get_background_color(self_);
	CmkColor color;
	cmk_style_get_font_color_for_background(cmk_widget_get_actual_style(self_), background, &color);
	ClutterColor cc = cmk_to_clutter_color(&color);
	if(PRIVATE(CMK_BUTTON(self_))->text)
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
		cairo_set_source_cmk_color(cr, cmk_style_get_color(style, "hover"));
		if(PRIVATE(self)->beveled)
		{
			cairo_new_sub_path(cr);
			cairo_arc(cr, width - radius, radius, radius, -90 * degrees, 0 * degrees);
			cairo_arc(cr, width - radius, height - radius, radius, 0 * degrees, 90 * degrees);
			cairo_arc(cr, radius, height - radius, radius, 90 * degrees, 180 * degrees);
			cairo_arc(cr, radius, radius, radius, 180 * degrees, 270 * degrees);
			cairo_close_path(cr);
			cairo_fill(cr);
		}
		else
		{
			cairo_paint(cr);
		}
	}
	return TRUE;
}

void cmk_button_set_text(CmkButton *self, const gchar *text)
{
	g_return_if_fail(CMK_IS_BUTTON(self));
	if(text)
	{
		if(!PRIVATE(self)->text)
		{
			PRIVATE(self)->text = CLUTTER_TEXT(clutter_text_new());
			clutter_actor_set_y_align(CLUTTER_ACTOR(PRIVATE(self)->text), CLUTTER_ACTOR_ALIGN_CENTER);
			clutter_actor_add_child(CLUTTER_ACTOR(self), CLUTTER_ACTOR(PRIVATE(self)->text));
		}
		clutter_text_set_text(PRIVATE(self)->text, text);
	}
	else if(PRIVATE(self)->text)
	{
		clutter_actor_remove_child(CLUTTER_ACTOR(self), CLUTTER_ACTOR(PRIVATE(self)->text));
	}
}

const gchar * cmk_button_get_text(CmkButton *self)
{
	g_return_val_if_fail(CMK_IS_BUTTON(self), NULL);
	if(!PRIVATE(self)->text)
		return NULL;
	return clutter_text_get_text(PRIVATE(self)->text);
}

void cmk_button_set_content(CmkButton *self, CmkWidget *content)
{
	g_return_if_fail(CMK_IS_BUTTON(self));
	if(clutter_actor_get_parent(CLUTTER_ACTOR(content)))
		return;
	if(PRIVATE(self)->content)
		clutter_actor_remove_child(CLUTTER_ACTOR(self), CLUTTER_ACTOR(PRIVATE(self)->content));
	PRIVATE(self)->content = content;
	if(content)
		clutter_actor_add_child(CLUTTER_ACTOR(self), CLUTTER_ACTOR(content));
	//clutter_actor_queue_relayout(CLUTTER_ACTOR(self));
}

CmkWidget * cmk_button_get_content(CmkButton *self)
{
	g_return_val_if_fail(CMK_IS_BUTTON(self), NULL);
	return PRIVATE(self)->content;
}

void cmk_button_set_beveled(CmkButton *self, gboolean beveled)
{
	g_return_if_fail(CMK_IS_BUTTON(self));
	if(PRIVATE(self)->beveled != beveled)
	{
		PRIVATE(self)->beveled = beveled;
		if(PRIVATE(self)->hover)
			clutter_content_invalidate(clutter_actor_get_content(CLUTTER_ACTOR(self)));
	}
}

gboolean cmk_button_get_beveled(CmkButton *self)
{
	g_return_val_if_fail(CMK_IS_BUTTON(self), FALSE);
	return PRIVATE(self)->beveled;
}

const gchar * cmk_button_get_name(CmkButton *self)
{
	g_return_val_if_fail(CMK_IS_BUTTON(self), NULL);
	const gchar *name = clutter_actor_get_name(CLUTTER_ACTOR(self));
	if(name)
		return name;
	return cmk_button_get_text(self);
}
