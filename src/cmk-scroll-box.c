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
	gfloat natW, natH;
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
	CmkScrollBoxPrivate *private = PRIVATE(self);
	private->ctx = clutter_backend_get_cogl_context(clutter_get_default_backend());
	private->pipe = cogl_pipeline_new(private->ctx);
	CoglColor *c = cogl_color_new();
	cogl_color_init_from_4ub(c, 255,255,255,150);
	cogl_color_premultiply(c);
	cogl_pipeline_set_color(private->pipe, c);
	cogl_color_free(c);

	clutter_actor_set_reactive(CLUTTER_ACTOR(self), TRUE);
	clutter_actor_set_clip_to_allocation(CLUTTER_ACTOR(self), TRUE);
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
	gfloat wEndPercent = maxScrollW / private->natW;
	gfloat wPercent = private->scroll.x / private->natW;
	gfloat hEndPercent = maxScrollH / private->natH;
	gfloat hPercent = private->scroll.y / private->natH;

	ensure_edge_shadow(private->shadow, "left", &private->lShad, &private->lShadDraw, wPercent);
	ensure_edge_shadow(private->shadow, "right", &private->rShad, &private->rShadDraw, wEndPercent-wPercent);
	ensure_edge_shadow(private->shadow, "top", &private->tShad, &private->tShadDraw, hPercent);
	ensure_edge_shadow(private->shadow, "bottom", &private->bShad, &private->bShadDraw, hEndPercent-hPercent);
}

static CoglPrimitive * rect_prim(CoglContext *ctx, float x, float y, float w, float h)
{
	CoglVertexP2 verts[6] = {
		x,     y,
		x,     y+h,
		x+w,   y,
		
		x+w,   y,
		x,     y+h,
		x+w,   y+h,
	};
	
	return cogl_primitive_new_p2(ctx, COGL_VERTICES_MODE_TRIANGLES, 6, verts);	
}

static void on_paint(ClutterActor *self_)
{
	CLUTTER_ACTOR_CLASS(cmk_scroll_box_parent_class)->paint(self_);
	CmkScrollBoxPrivate *private = PRIVATE(CMK_SCROLL_BOX(self_));
	
	gfloat width, height;
	clutter_actor_get_size(self_, &width, &height);

	update_preferred_size(self_);
	
	gfloat maxScrollW = MAX(private->natW - width, 0);
	gfloat maxScrollH = MAX(private->natH - height, 0);
	gfloat hPercent = private->scroll.y / private->natH;
	gfloat wPercent = private->scroll.x / private->natW;

	ensure_shadow(private, maxScrollW, maxScrollH);
	
	ClutterActorBox box = {0, 0, width, height};
	cmk_shadoutil_paint(private->shadow, &box);

	if(!private->scrollbars)
		return;

	gfloat hBarSize = MIN(1, (height / private->natH)) * height; 
	gfloat wBarSize = MIN(1, (width / private->natW)) * width; 
	
	// TODO: Adjustable sizing + dpi scale
	const gfloat size = 8;
	
	// TODO: Scroll bars fade in and out
	// TODO: Draggable scroll bars
	
	CoglFramebuffer *fb = cogl_get_draw_framebuffer();
	
	if(private->natH > height)
	{
		// TODO: Stylable colors

		CoglPrimitive *p = 
			rect_prim(private->ctx,
			          width - size - size/2,
			          hPercent * height,
			          width- size/2,
			          hPercent * height + hBarSize);
		cogl_primitive_draw(p, fb, private->pipe);
		cogl_object_unref(p);
	}
	if(private->natW > width)
	{
		CoglPrimitive *p = 
			rect_prim(private->ctx,
			          wPercent * width,
			          height - size,
			          wPercent * width + wBarSize,
			          height);
		cogl_primitive_draw(p, fb, private->pipe);
		cogl_object_unref(p);
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
		if(new.x >= private->scroll.x && new.x <= private->scroll.x + width)
			new.x = private->scroll.x;
		if(new.y >= private->scroll.y && new.y <= private->scroll.y + height)
			new.y = private->scroll.y;
	}
	
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

void cmk_scroll_box_set_use_shadow(CmkScrollBox *self, gboolean l, gboolean r, gboolean t, gboolean b)
{
	g_return_if_fail(CMK_IS_SCROLL_BOX(self));
	CmkScrollBoxPrivate *private = PRIVATE(self);
	private->lShad = l;
	private->rShad = r;
	private->tShad = t;
	private->bShad = b;
}
