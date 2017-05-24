/*
 * This file is part of graphene-desktop, the desktop environment of VeltOS.
 * Copyright (C) 2016 Velt Technologies, Aidan Shafran <zelbrium@gmail.com>
 * Licensed under the Apache License 2 <www.apache.org/licenses/LICENSE-2.0>.
 */

#include "cmk-scroll-box.h"

typedef struct _CmkScrollBoxPrivate CmkScrollBoxPrivate;
struct _CmkScrollBoxPrivate
{
	ClutterScrollMode scrollMode;
	ClutterPoint scroll;
	gboolean scrollbars;
	gfloat natW, natH;
};

enum
{
	PROP_SCROLL_MODE = 1,
	PROP_SHOW_SCROLLBARS,
	PROP_LAST
};

static GParamSpec *properties[PROP_LAST];

static void cmk_scroll_box_get_property(GObject *self_, guint propertyId, GValue *value, GParamSpec *pspec);
static void cmk_scroll_box_set_property(GObject *self_, guint propertyId, const GValue *value, GParamSpec *pspec);
static void on_queue_relayout(ClutterActor *self_);
static void on_paint(ClutterActor *self_);
static gboolean on_scroll(ClutterActor *self_, ClutterScrollEvent *event);
static void scroll_to(CmkScrollBox *self, const ClutterPoint *point, gboolean exact);
static void on_key_focus_changed(CmkWidget *self, ClutterActor *newfocus);

G_DEFINE_TYPE_WITH_PRIVATE(CmkScrollBox, cmk_scroll_box, CMK_TYPE_WIDGET);
#define PRIVATE(self) ((CmkScrollBoxPrivate *)cmk_scroll_box_get_instance_private(self))

CmkScrollBox * cmk_scroll_box_new(ClutterScrollMode scrollMode)
{
	return CMK_SCROLL_BOX(g_object_new(CMK_TYPE_SCROLL_BOX, "scroll-mode", scrollMode, NULL));
}

static void cmk_scroll_box_class_init(CmkScrollBoxClass *class)
{
	GObjectClass *base = G_OBJECT_CLASS(class);
	base->get_property = cmk_scroll_box_get_property;
	base->set_property = cmk_scroll_box_set_property;

	ClutterActorClass *actorClass = CLUTTER_ACTOR_CLASS(class);
	actorClass->queue_relayout = on_queue_relayout;
	actorClass->paint = on_paint;
	actorClass->scroll_event = on_scroll;
	
	CMK_WIDGET_CLASS(class)->key_focus_changed = on_key_focus_changed;
	
	properties[PROP_SCROLL_MODE] = g_param_spec_flags("scroll-mode", "scroll mode", "scrolling mode", CLUTTER_TYPE_SCROLL_MODE, CLUTTER_SCROLL_BOTH, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
	
	properties[PROP_SHOW_SCROLLBARS] = g_param_spec_boolean("scrollbars", "show scrollbars", "show scrollbars", TRUE, G_PARAM_READWRITE|G_PARAM_CONSTRUCT);
	
	g_object_class_install_properties(base, PROP_LAST, properties);
}

static void cmk_scroll_box_init(CmkScrollBox *self)
{
	clutter_actor_set_reactive(CLUTTER_ACTOR(self), TRUE);
	clutter_actor_set_clip_to_allocation(CLUTTER_ACTOR(self), TRUE);
	clutter_point_init(&(PRIVATE(self)->scroll), 0.0f, 0.0f);
}

static void cmk_scroll_box_get_property(GObject *self_, guint propertyId, GValue *value, GParamSpec *pspec)
{
	switch(propertyId)
	{
	case PROP_SCROLL_MODE:
		g_value_set_flags(value, PRIVATE(CMK_SCROLL_BOX(self_))->scrollMode);
		break;
	case PROP_SHOW_SCROLLBARS:
		g_value_set_boolean(value, PRIVATE(CMK_SCROLL_BOX(self_))->scrollbars);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(self_, propertyId, pspec);
		break;
	}
}

static void cmk_scroll_box_set_property(GObject *self_, guint propertyId, const GValue *value, GParamSpec *pspec)
{
	switch(propertyId)
	{
	case PROP_SCROLL_MODE:
		PRIVATE(CMK_SCROLL_BOX(self_))->scrollMode = g_value_get_flags(value);
		break;
	case PROP_SHOW_SCROLLBARS:
		PRIVATE(CMK_SCROLL_BOX(self_))->scrollbars = g_value_get_boolean(value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(self_, propertyId, pspec);
		break;
	}
}

static void update_preferred_size(ClutterActor *self_)
{
	CmkScrollBoxPrivate *private = PRIVATE(CMK_SCROLL_BOX(self_));
	if(private->natW > 0 && private->natH > 0)
		return;
	clutter_actor_get_preferred_size(self_, NULL, NULL, &private->natW, &private->natH); 
}

static void on_queue_relayout(ClutterActor *self_)
{
	CLUTTER_ACTOR_CLASS(cmk_scroll_box_parent_class)->queue_relayout(self_);
	CmkScrollBoxPrivate *private = PRIVATE(CMK_SCROLL_BOX(self_));
	private->natW = private->natH = 0;
	update_preferred_size(self_);
	scroll_to(CMK_SCROLL_BOX(self_), &private->scroll, TRUE);
}

static void on_paint(ClutterActor *self_)
{
	CLUTTER_ACTOR_CLASS(cmk_scroll_box_parent_class)->paint(self_);
	CmkScrollBoxPrivate *private = PRIVATE(CMK_SCROLL_BOX(self_));
	
	if(!private->scrollbars)
		return;
	
	gfloat width, height;
	clutter_actor_get_size(self_, &width, &height);
	update_preferred_size(self_);
	
	gfloat hBarSize = MIN(1, (height / private->natH)) * height; 
	gfloat wBarSize = MIN(1, (width / private->natW)) * width; 
	gfloat hPercent = private->scroll.y / private->natH;
	gfloat wPercent = private->scroll.x / private->natW;

	// TODO: Adjustable sizing + dpi scale
	const gfloat size = 8;
	
	// TODO: Scroll bars fade in and out
	// TODO: Draggable scroll bars
	
	if(private->natH > height)
	{
		// TODO: Non-depricated drawing API
		// TODO: Stylable colors
		cogl_set_source_color4ub(255,255,255,150);
		cogl_rectangle(width - size,
		               hPercent * height,
		               width,
		               hPercent * height + hBarSize);
	}
	if(private->natW > width)
	{
		cogl_set_source_color4ub(255,255,255,150);
		cogl_rectangle(wPercent * width,
		               height - size,
		               wPercent * width + wBarSize,
		               height);
	}
}

static void scroll_to(CmkScrollBox *self, const ClutterPoint *point, gboolean exact)
{
	CmkScrollBoxPrivate *private = PRIVATE(self);

	gfloat width = clutter_actor_get_width(CLUTTER_ACTOR(self));
	gfloat height = clutter_actor_get_height(CLUTTER_ACTOR(self));
	
	ClutterPoint new = *point;

	if(!exact)
	{
		// Don't scroll anywhere if the requested point is already in view
		if(new.x >= private->scroll.x && new.x <= private->scroll.x + height)
			new.x = private->scroll.x;
		if(new.y >= private->scroll.y && new.y <= private->scroll.y + height)
			new.y = private->scroll.y;
	}

	//if(clutter_point_equals(&private->scroll, &new))
	//	return;

	update_preferred_size(CLUTTER_ACTOR(self));
	
	gfloat maxScrollW = MAX(private->natW - width, 0);
	gfloat maxScrollH = MAX(private->natH - height, 0);
	new.x = MIN(MAX(0, new.x), maxScrollW);
	new.y = MIN(MAX(0, new.y), maxScrollH);

	if(clutter_point_equals(&private->scroll, &new))
		return;

	private->scroll = new;

	ClutterMatrix transform;
	clutter_matrix_init_identity(&transform);
	cogl_matrix_translate(&transform,
		(private->scrollMode & CLUTTER_SCROLL_HORIZONTALLY) ? -new.x : 0.0f,
		(private->scrollMode & CLUTTER_SCROLL_VERTICALLY)   ? -new.y : 0.0f,
		0.0f);
	clutter_actor_set_child_transform(CLUTTER_ACTOR(self), &transform);
}

static inline void scroll_by(CmkScrollBox *self, gdouble dx, gdouble dy)
{
	ClutterPoint point;
	point.x = PRIVATE(self)->scroll.x + dx;
	point.y = PRIVATE(self)->scroll.y + dy;
	scroll_to(self, &point, TRUE);
}

static gboolean on_scroll(ClutterActor *self_, ClutterScrollEvent *event)
{
	if(event->direction == CLUTTER_SCROLL_SMOOTH)
	{
		gdouble dx=0.0f, dy=0.0f;
		clutter_event_get_scroll_delta((ClutterEvent *)event, &dx, &dy);
		dx *= 50; // TODO: Not magic number for multiplier
		dy *= 50;
		scroll_by(CMK_SCROLL_BOX(self_), dx, dy);
	}
	return CLUTTER_EVENT_STOP;
}

static void scroll_to_actor(CmkScrollBox *self, ClutterActor *scrollto)
{
	ClutterVertex vert = {0};
	ClutterVertex out = {0};
	clutter_actor_apply_relative_transform_to_point(scrollto, CLUTTER_ACTOR(self), &vert, &out);
	ClutterPoint *scroll = &(PRIVATE(self)->scroll);
	ClutterPoint p = {out.x + scroll->x, out.y + scroll->y};
	scroll_to(self, &p, FALSE);
}

static void on_key_focus_changed(CmkWidget *self, ClutterActor *newfocus)
{
	scroll_to_actor(CMK_SCROLL_BOX(self), newfocus);
}

void cmk_scroll_box_set_show_scrollbars(CmkScrollBox *self, gboolean show)
{
	g_return_if_fail(CMK_IS_SCROLL_BOX(self));
	CmkScrollBoxPrivate *private = PRIVATE(self);
	if(private->scrollbars == show)
		return;
	private->scrollbars = show;
	clutter_actor_queue_redraw(CLUTTER_ACTOR(self));
}
