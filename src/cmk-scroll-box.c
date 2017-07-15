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
	CmkShadoutil *shadow;
	gboolean lShad, rShad, tShad, bShad;
	gboolean lShadDraw, rShadDraw, tShadDraw, bShadDraw;
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
static void on_allocate(ClutterActor *self_, const ClutterActorBox *box, ClutterAllocationFlags flags);
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
	base->dispose = cmk_scroll_box_dispose;

	ClutterActorClass *actorClass = CLUTTER_ACTOR_CLASS(class);
	actorClass->allocate = on_allocate;
	//actorClass->queue_relayout = on_queue_relayout;
	actorClass->paint = on_paint;
	actorClass->scroll_event = on_scroll;
	
	CMK_WIDGET_CLASS(class)->key_focus_changed = on_key_focus_changed;
	
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

	private->shadow = cmk_shadoutil_new();
	cmk_shadoutil_set_size(private->shadow, 20);
	cmk_shadoutil_set_edges(private->shadow, 0, 0, 0, 0);
	cmk_shadoutil_set_mode(private->shadow, CMK_SHADOW_MODE_INNER);
	cmk_shadoutil_set_actor(private->shadow, CLUTTER_ACTOR(self));
}

static void cmk_scroll_box_dispose(GObject *self_)
{
	CmkScrollBoxPrivate *private = PRIVATE(CMK_SCROLL_BOX(self_));
	g_clear_object(&private->shadow);
	g_clear_pointer(&private->pipe, cogl_object_unref);
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

	// Scroll to the current scroll position, which performs bounds
	// checks on the position to make sure it's still within the valid
	// scroll range.
	scroll_to(CMK_SCROLL_BOX(self_), &private->scroll, TRUE);
}

#define FLOAT_TO_POINTER(f) (void *)*(gsize*)&f
#define POINTER_TO_FLOAT(p) *(gfloat *)(guint32*)&p

static void fdoanimate(ClutterTimeline *timeline, gint msecs, GObject *object)
{
	const gchar *prop = g_object_get_data(G_OBJECT(timeline), "prop");
	gpointer startp = g_object_get_data(G_OBJECT(timeline), "start");
	gpointer endp = g_object_get_data(G_OBJECT(timeline), "end");
	gfloat start = POINTER_TO_FLOAT(startp);
	gfloat end = POINTER_TO_FLOAT(endp);
	
	gfloat prog = clutter_timeline_get_progress(timeline);
	gfloat val = start + (end-start)*prog;

	g_object_set(object, prop, val, NULL);
}

static void fanimate(gpointer object, const gchar *prop, guint ms, gfloat end) 
{
	gfloat start;
	g_object_get(G_OBJECT(object), prop, &start, NULL);
	if(start == end)
		return;
	
	ClutterTimeline *timeline = clutter_timeline_new(ms);
	g_object_set_data_full(G_OBJECT(timeline), "prop", g_strdup(prop), g_free);
	g_object_set_data(G_OBJECT(timeline), "start", FLOAT_TO_POINTER(start));
	g_object_set_data(G_OBJECT(timeline), "end", FLOAT_TO_POINTER(end));
	g_signal_connect(timeline, "new-frame", G_CALLBACK(fdoanimate), object);
	g_signal_connect(timeline, "completed", G_CALLBACK(g_object_unref), NULL);
	clutter_timeline_start(timeline);
}

static void ensure_edge_shadow(CmkShadoutil *util, const gchar *edge, gboolean *shad, gboolean *shadDraw, gfloat percent)
{
	if(*shad)
	{
		if(!*shadDraw && percent > 0)
		{
			*shadDraw = TRUE;
			fanimate(util, edge, 100, 1);
		}
		else if(*shadDraw && percent == 0)
		{
			*shadDraw = FALSE;
			fanimate(util, edge, 100, 0);
		}
	}
	else if(*shadDraw)
	{
		*shadDraw = FALSE;
		fanimate(util, edge, 100, 0);
	}
}

static void ensure_shadow(CmkScrollBoxPrivate *private, gfloat maxScrollW, gfloat maxScrollH)
{
	gfloat wEndPercent = maxScrollW / private->prefW;
	gfloat wPercent = private->scroll.x / private->prefW;
	gfloat hEndPercent = maxScrollH / private->prefH;
	gfloat hPercent = private->scroll.y / private->prefH;

	ensure_edge_shadow(private->shadow, "left", &private->lShad, &private->lShadDraw, wPercent);
	ensure_edge_shadow(private->shadow, "right", &private->rShad, &private->rShadDraw, wEndPercent-wPercent);
	ensure_edge_shadow(private->shadow, "top", &private->tShad, &private->tShadDraw, hPercent);
	ensure_edge_shadow(private->shadow, "bottom", &private->bShad, &private->bShadDraw, hEndPercent-hPercent);
}

static CoglPrimitive * rect_prim(CoglContext *ctx, float x1, float y1, float x2, float y2)
{
	CoglVertexP2 verts[6] = {
		x1,  y1,
		x1,  y2,
		x2,  y1,
		
		x2,  y1,
		x1,  y2,
		x2,  y2,
	};
	
	return cogl_primitive_new_p2(ctx, COGL_VERTICES_MODE_TRIANGLES, 6, verts);	
}

static void on_paint(ClutterActor *self_)
{
	CLUTTER_ACTOR_CLASS(cmk_scroll_box_parent_class)->paint(self_);
	CmkScrollBoxPrivate *private = PRIVATE(CMK_SCROLL_BOX(self_));
	
	gfloat maxScrollW = MAX(private->prefW - private->allocW, 0);
	gfloat maxScrollH = MAX(private->prefH - private->allocW, 0);
	gfloat hPercent = private->scroll.y / private->prefH;
	gfloat wPercent = private->scroll.x / private->prefW;

	ensure_shadow(private, maxScrollW, maxScrollH);
	
	ClutterActorBox box = {0, 0, private->allocW, private->allocH};
	cmk_shadoutil_paint(private->shadow, &box);

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

void cmk_scroll_box_set_use_shadow(CmkScrollBox *self, gboolean l, gboolean r, gboolean t, gboolean b)
{
	g_return_if_fail(CMK_IS_SCROLL_BOX(self));
	CmkScrollBoxPrivate *private = PRIVATE(self);
	private->lShad = l;
	private->rShad = r;
	private->tShad = t;
	private->bShad = b;
}
