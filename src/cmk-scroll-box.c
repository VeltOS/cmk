/*
 * This file is part of graphene-desktop, the desktop environment of VeltOS.
 * Copyright (C) 2016 Velt Technologies, Aidan Shafran <zelbrium@gmail.com>
 * Licensed under the Apache License 2 <www.apache.org/licenses/LICENSE-2.0>.
 */

// Removes implicit declaration warnings
#define COGL_ENABLE_EXPERIMENTAL_API
#define CLUTTER_ENABLE_EXPERIMENTAL_API

#include "cmk-scroll-box.h"
#include "cmk-shadow.h"

typedef struct _CmkScrollBoxPrivate CmkScrollBoxPrivate;
struct _CmkScrollBoxPrivate
{
	CoglContext *ctx;
	CoglPipeline *pipe;
	ClutterScrollMode scrollMode;
	ClutterPoint scroll;
	gfloat prefW, prefH;
	float allocW, allocH;
	gboolean scrollbars;
	CmkShadowEffect *shadow;
	gboolean lShad, rShad, tShad, bShad;
	gboolean lShadDraw, rShadDraw, tShadDraw, bShadDraw;
	gboolean scrollToBottom;
	
	ClutterStage *lastStage;
	guint stageFocusSignalId;
};

enum
{
	PROP_SCROLL_MODE = 1,
	PROP_SHOW_SCROLLBARS,
	PROP_LAST
};

static GParamSpec *properties[PROP_LAST];

static void cmk_scroll_box_dispose(GObject *self_);
static void cmk_scroll_box_get_property(GObject *self_, guint propertyId, GValue *value, GParamSpec *pspec);
static void cmk_scroll_box_set_property(GObject *self_, guint propertyId, const GValue *value, GParamSpec *pspec);
static void on_map(ClutterActor *self_);
static void on_unmap(ClutterActor *self_);
static void on_allocate(ClutterActor *self_, const ClutterActorBox *box, ClutterAllocationFlags flags);
static gboolean get_paint_volume(ClutterActor *self_, ClutterPaintVolume *volume);
static void on_paint(ClutterActor *self_);
static gboolean on_scroll(ClutterActor *self_, ClutterScrollEvent *event);
static void scroll_to_actor(CmkScrollBox *self, ClutterActor *scrollto);
static void scroll_to(CmkScrollBox *self, const ClutterPoint *point, gboolean exact);

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
	base->dispose = cmk_scroll_box_dispose;

	ClutterActorClass *actorClass = CLUTTER_ACTOR_CLASS(class);
	actorClass->map = on_map;
	actorClass->unmap = on_unmap;
	actorClass->allocate = on_allocate;
	actorClass->get_paint_volume = get_paint_volume;
	actorClass->paint = on_paint;
	actorClass->scroll_event = on_scroll;
	
	properties[PROP_SCROLL_MODE] = g_param_spec_flags("scroll-mode", "scroll mode", "scrolling mode", CLUTTER_TYPE_SCROLL_MODE, CLUTTER_SCROLL_BOTH, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
	
	properties[PROP_SHOW_SCROLLBARS] = g_param_spec_boolean("scrollbars", "show scrollbars", "show scrollbars", TRUE, G_PARAM_READWRITE|G_PARAM_CONSTRUCT);
	
	g_object_class_install_properties(base, PROP_LAST, properties);
}

static void cmk_scroll_box_init(CmkScrollBox *self)
{
	CmkScrollBoxPrivate *private = PRIVATE(self);
	private->ctx = clutter_backend_get_cogl_context(clutter_get_default_backend());
	private->pipe = cogl_pipeline_new(private->ctx);

	clutter_actor_set_reactive(CLUTTER_ACTOR(self), TRUE);
	clutter_point_init(&(PRIVATE(self)->scroll), 0.0f, 0.0f);

	private->shadow = cmk_shadow_effect_new(10);
	cmk_shadow_effect_set_inset(private->shadow, 0, 0, 0, 0);
	clutter_actor_add_effect(CLUTTER_ACTOR(self), CLUTTER_EFFECT(private->shadow));
}

static void cmk_scroll_box_dispose(GObject *self_)
{
	CmkScrollBoxPrivate *private = PRIVATE(CMK_SCROLL_BOX(self_));
	g_clear_pointer(&private->pipe, cogl_object_unref);
	if(private->stageFocusSignalId)
		g_signal_handler_disconnect(private->lastStage, private->stageFocusSignalId);
	private->stageFocusSignalId = 0;
	G_OBJECT_CLASS(cmk_scroll_box_parent_class)->dispose(self_);
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

static void on_stage_focus_changed(CmkScrollBox *self, UNUSED GParamSpec *spec, ClutterStage *stage)
{
	ClutterActor *focused = clutter_stage_get_key_focus(stage);
	if(clutter_actor_contains(CLUTTER_ACTOR(self), focused))
		scroll_to_actor(self, focused);
}

static void on_map(ClutterActor *self_)
{
	CLUTTER_ACTOR_CLASS(cmk_scroll_box_parent_class)->map(self_);
	CmkScrollBoxPrivate *private = PRIVATE(CMK_SCROLL_BOX(self_));
	if(private->stageFocusSignalId)
		g_signal_handler_disconnect(private->lastStage, private->stageFocusSignalId);
	private->lastStage = (ClutterStage *)clutter_actor_get_stage(self_);
	private->stageFocusSignalId = g_signal_connect_swapped(private->lastStage, "notify::key-focus", G_CALLBACK(on_stage_focus_changed), self_);
}

static void on_unmap(ClutterActor *self_)
{
	CLUTTER_ACTOR_CLASS(cmk_scroll_box_parent_class)->unmap(self_);
	CmkScrollBoxPrivate *private = PRIVATE(CMK_SCROLL_BOX(self_));
	if(private->stageFocusSignalId)
		g_signal_handler_disconnect(private->lastStage, private->stageFocusSignalId);
	private->stageFocusSignalId = 0;
}

static void on_allocate(ClutterActor *self_, const ClutterActorBox *box, ClutterAllocationFlags flags)
{
	CmkScrollBoxPrivate *private = PRIVATE(CMK_SCROLL_BOX(self_));

	// Since calling Clutter's allocate sets the values returned by
	// get_width and get_height, and they're not the real width and
	// height, the values must be saved manually.
	private->allocW = (box->x2 - box->x1);
	private->allocH = (box->y2 - box->y1);

	// Can't use clip to allocation because we fake the allocation
	clutter_actor_set_clip(self_, 0, 0, private->allocW, private->allocH);
	
	// Using get_preferred_size doesn't use the forWidth/forHeight
	// values, which are important to get the right size on actors such
	// as text which change height depending on their allocated width.
	clutter_actor_get_preferred_width(self_,
		private->allocH, &private->prefW, NULL);
	clutter_actor_get_preferred_height(self_,
		private->allocW, &private->prefH, NULL);
	
	// Do the regular Clutter allocation, except tell it that we have
	// as much size as we're requesting, not how much we actually have.
	// (Unless the actual size is larger than the request.)
	ClutterActorBox new = {
		box->x1,
		box->y1,
		box->x1 + MAX(private->allocW, private->prefW),
		box->y1 + MAX(private->allocH, private->prefH)
	};
	CLUTTER_ACTOR_CLASS(cmk_scroll_box_parent_class)->allocate(self_, &new, flags | CLUTTER_DELEGATE_LAYOUT);

	// See cmk_scroll_box_scroll_to_bottom
	if(private->scrollToBottom)
	{
		gfloat maxScrollH = MAX(private->prefH - private->allocH, 0);
		ClutterPoint new = private->scroll;
		new.y = maxScrollH;
		scroll_to(CMK_SCROLL_BOX(self_), &new, TRUE);
	}
	else
	{
		// Scroll to the current scroll position, which performs bounds
		// checks on the position to make sure it's still within the valid
		// scroll range.
		scroll_to(CMK_SCROLL_BOX(self_), &private->scroll, TRUE);
	}
}

static void ensure_shadow(CmkScrollBoxPrivate *private, gfloat maxScrollW, gfloat maxScrollH)
{
	gfloat wEndPercent = maxScrollW / private->prefW;
	gfloat wPercent = private->scroll.x / private->prefW;
	gfloat hEndPercent = maxScrollH / private->prefH;
	gfloat hPercent = private->scroll.y / private->prefH;

	cmk_shadow_effect_inset_animate_edges(private->shadow,
		(wPercent > 0 && private->lShad) ? 1 : 0,
		((wEndPercent-wPercent && private->rShad) > 0) ? 1 : 0,
		(hPercent > 0 && private->tShad) ? 1 : 0,
		((hEndPercent-hPercent) > 0 && private->bShad) ? 1 : 0);
}

static CoglPrimitive * rect_prim(CoglContext *ctx, float x1, float y1, float x2, float y2)
{
	CoglVertexP2 verts[6] = {
		{x1,  y1},
		{x1,  y2},
		{x2,  y1},
		
		{x2,  y1},
		{x1,  y2},
		{x2,  y2}
	};
	
	return cogl_primitive_new_p2(ctx, COGL_VERTICES_MODE_TRIANGLES, 6, verts);	
}

static gboolean get_paint_volume(ClutterActor *self_, ClutterPaintVolume *volume)
{
	CmkScrollBoxPrivate *private = PRIVATE(CMK_SCROLL_BOX(self_));
	clutter_paint_volume_set_width(volume, private->allocW);
	clutter_paint_volume_set_height(volume, private->allocH);
	return TRUE;
}

static void on_paint(ClutterActor *self_)
{
	CLUTTER_ACTOR_CLASS(cmk_scroll_box_parent_class)->paint(self_);
	CmkScrollBoxPrivate *private = PRIVATE(CMK_SCROLL_BOX(self_));
	
	gfloat hPercent = private->scroll.y / private->prefH;
	gfloat wPercent = private->scroll.x / private->prefW;

	if(!private->scrollbars)
		return;

	const gfloat barThickness = CMK_DP(self_, 4);
	
	// TODO: Scroll bars fade in and out
	// TODO: Draggable scroll bars
	// TODO: Stylable colors
	
	CoglFramebuffer *fb = cogl_get_draw_framebuffer();
	float alpha = 0.6 * clutter_actor_get_paint_opacity(self_)/255.0;
	cogl_pipeline_set_color4f(private->pipe,
		1*alpha, // premultiplied alpha
		1*alpha,
		1*alpha,
		alpha);
	
	if(private->prefH > private->allocH && (private->scrollMode & CLUTTER_SCROLL_VERTICALLY))
	{
		float hBarSize = MIN(1, (private->allocH / private->prefH)) * private->allocH; 
		CoglPrimitive *p = 
			rect_prim(private->ctx,
			          private->allocW - barThickness - barThickness/2,
			          hPercent * private->allocH,
			          private->allocW - barThickness/2,
			          hPercent * private->allocH + hBarSize);
		cogl_primitive_draw(p, fb, private->pipe);
		cogl_object_unref(p);
	}
	if(private->prefW > private->allocW && (private->scrollMode & CLUTTER_SCROLL_HORIZONTALLY))
	{
		float wBarSize = MIN(1, (private->allocW / private->prefW)) * private->allocW; 
		CoglPrimitive *p = 
			rect_prim(private->ctx,
			          wPercent * private->allocW,
			          private->allocH - barThickness,
			          wPercent * private->allocW + wBarSize,
			          private->allocH);
		cogl_primitive_draw(p, fb, private->pipe);
		cogl_object_unref(p);
	}
}

static void scroll_to(CmkScrollBox *self, const ClutterPoint *point, gboolean exact)
{
	CmkScrollBoxPrivate *private = PRIVATE(self);
	ClutterPoint new = *point;
	
	if(!exact)
	{
		// Don't scroll anywhere if the requested point is already in view
		if(new.x >= private->scroll.x && new.x <= private->scroll.x + private->allocW)
			new.x = private->scroll.x;
		if(new.y >= private->scroll.y && new.y <= private->scroll.y + private->allocH)
			new.y = private->scroll.y;
	}
	
	gfloat maxScrollW = MAX(private->prefW - private->allocW, 0);
	gfloat maxScrollH = MAX(private->prefH - private->allocH, 0);
	new.x = MIN(MAX(0, new.x), maxScrollW);
	new.y = MIN(MAX(0, new.y), maxScrollH);
	
	ensure_shadow(private, maxScrollW, maxScrollH);
	
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
		CmkScrollBoxPrivate *private = PRIVATE(CMK_SCROLL_BOX(self_));
		if(!(private->scrollMode & CLUTTER_SCROLL_HORIZONTALLY))
			dx = 0;
		if(!(private->scrollMode & CLUTTER_SCROLL_VERTICALLY))
			dy = 0;
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

void cmk_scroll_box_set_show_scrollbars(CmkScrollBox *self, gboolean show)
{
	g_return_if_fail(CMK_IS_SCROLL_BOX(self));
	CmkScrollBoxPrivate *private = PRIVATE(self);
	if(private->scrollbars == show)
		return;
	private->scrollbars = show;
	clutter_actor_queue_redraw(CLUTTER_ACTOR(self));
}

void cmk_scroll_box_set_use_shadow(CmkScrollBox *self, gboolean l, gboolean r, gboolean t, gboolean b)
{
	g_return_if_fail(CMK_IS_SCROLL_BOX(self));
	CmkScrollBoxPrivate *private = PRIVATE(self);
	private->lShad = l;
	private->rShad = r;
	private->tShad = t;
	private->bShad = b;
}

void cmk_scroll_box_scroll_to_bottom(CmkScrollBox *self)
{
	// Can't scroll here, because the allocation may not have been updated
	// yet. There's not a way to update the real and requested size of the
	// scroll box without running a relayout.
	PRIVATE(self)->scrollToBottom = TRUE;
	clutter_actor_queue_relayout(CLUTTER_ACTOR(self));
}
