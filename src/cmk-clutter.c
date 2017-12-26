/*
 * libcmk, Copyright 2018 Aidan Shafran <zelbrium@gmail.com>
 * Apache License 2.0 <www.apache.org/licenses/LICENSE-2.0>.
 */

#define COGL_ENABLE_EXPERIMENTAL_API
#define CLUTTER_ENABLE_EXPERIMENTAL_API

#include "cmk-clutter.h"
#include "cmk-timeline.h"
#include <cogl/cogl.h>
#include <cairo/cairo.h>

struct _CmkClutterWidget
{
	ClutterActor parent;

	CoglContext *ctx;
	CoglTexture *tex;
	gboolean npot;

	cairo_surface_t *surface;

	// The region which has been invalidated since last
	// redraw. If NULL, all of the surface is dirty.
	cairo_region_t *dirty;

	CmkWidget *widget;
};

static void cmk_clutter_widget_init(CmkClutterWidget *self);
static void on_dispose(GObject *self_);
static void on_allocate(ClutterActor *self_, const ClutterActorBox *box, ClutterAllocationFlags flags);
static gboolean on_event(ClutterActor *self_, ClutterEvent *event);
static void on_reactive_changed(CmkClutterWidget *self, GParamSpec *spec, gpointer userdata);
static void on_paint_node(ClutterActor *self_, ClutterPaintNode *root);
static gboolean get_paint_volume(ClutterActor *self_, ClutterPaintVolume *volume);
static void get_preferred_width(ClutterActor *self_, gfloat forHeight, gfloat *min, gfloat *nat);
static void get_preferred_height(ClutterActor *self_, float forWidth, gfloat *min, gfloat *nat);
static void on_widget_request_invalidate(UNUSED CmkWidget *widget, const cairo_region_t *region, CmkClutterWidget *self);
static void on_widget_request_relayout(UNUSED CmkWidget *widget, UNUSED void *signaldata, CmkClutterWidget *self);
static void on_widget_event_mask_changed(CmkWidget *widget, UNUSED void *signaldata, CmkClutterWidget *self);
static void * cmk_clutter_timeline_callback(CmkTimeline *timeline, bool start, uint64_t *time, void *userdata);


G_DEFINE_TYPE(CmkClutterWidget, cmk_clutter_widget, CLUTTER_TYPE_ACTOR);


ClutterActor * cmk_widget_to_clutter(CmkWidget *widget)
{
	g_return_val_if_fail(widget, NULL);

	// Return existing wrapper object if available
	CmkClutterWidget *wrapper = cmk_widget_get_wrapper(widget);
	if(wrapper != NULL)
	{
		g_return_val_if_fail(CMK_IS_CLUTTER_WIDGET(wrapper), NULL);
		return CLUTTER_ACTOR(wrapper);
	}

	// Create new wrapper
	CmkClutterWidget *self = CMK_CLUTTER_WIDGET(g_object_new(CMK_TYPE_CLUTTER_WIDGET, NULL));
	g_return_val_if_fail(CMK_IS_CLUTTER_WIDGET(self), NULL);

	// Connect wrapper and widget
	self->widget = cmk_widget_ref(widget);
	cmk_widget_set_wrapper(widget, self);
	cmk_widget_listen(widget, "invalidate", CMK_CALLBACK(on_widget_request_invalidate), self);
	cmk_widget_listen(widget, "relayout", CMK_CALLBACK(on_widget_request_relayout), self);
	cmk_widget_listen(widget, "event-mask", CMK_CALLBACK(on_widget_event_mask_changed), self);
	g_signal_connect(self, "notify::reactive", G_CALLBACK(on_reactive_changed), NULL);

	cmk_timeline_set_handler_callback(cmk_clutter_timeline_callback, false);

	return CLUTTER_ACTOR(self);
}

static void cmk_clutter_widget_class_init(CmkClutterWidgetClass *class)
{
	GObjectClass *base = G_OBJECT_CLASS(class);
	base->dispose = on_dispose;

	ClutterActorClass *caclass = CLUTTER_ACTOR_CLASS(class);
	caclass->allocate = on_allocate;
	caclass->event = on_event;
	caclass->paint_node = on_paint_node;
	caclass->get_paint_volume = get_paint_volume;
	caclass->get_preferred_width = get_preferred_width;
	caclass->get_preferred_height = get_preferred_height;
}

static void cmk_clutter_widget_init(CmkClutterWidget *self)
{
	clutter_actor_set_reactive(CLUTTER_ACTOR(self), TRUE);

	self->ctx = clutter_backend_get_cogl_context(clutter_get_default_backend());
	self->npot = cogl_has_feature(self->ctx, COGL_FEATURE_ID_TEXTURE_NPOT_BASIC);
}

static void on_dispose(GObject *self_)
{
	CmkClutterWidget *self = CMK_CLUTTER_WIDGET(self_);
	if(self->widget)
	{
		cmk_widget_unlisten_by_userdata(self->widget, self);
		cmk_widget_set_wrapper(self->widget, NULL);
	}
	g_clear_pointer(&self->widget, cmk_widget_unref);
	G_OBJECT_CLASS(cmk_clutter_widget_parent_class)->dispose(self_);
}

static void on_allocate(ClutterActor *self_, const ClutterActorBox *box, ClutterAllocationFlags flags)
{
	cmk_widget_set_size(CMK_CLUTTER_WIDGET(self_)->widget,
		box->x2 - box->x1,
		box->y2 - box->y1);

	CLUTTER_ACTOR_CLASS(cmk_clutter_widget_parent_class)->allocate(self_, box, flags);
}

static guint translate_clutter_modifiers(const ClutterEvent *event)
{
	guint clmod = clutter_event_get_state(event);
	if(clmod == 0)
		return 0;
	guint cmkmod = 0;
	if((clmod & CLUTTER_SHIFT_MASK) == CLUTTER_SHIFT_MASK)
		cmkmod |= CMK_MOD_SHIFT;
	if((clmod & CLUTTER_LOCK_MASK) == CLUTTER_LOCK_MASK)
		cmkmod |= CMK_MOD_CAPS_LOCK;
	if((clmod & CLUTTER_CONTROL_MASK) == CLUTTER_CONTROL_MASK)
	{
		cmkmod |= CMK_MOD_CONTROL;
		// TODO: Map to control or meta depending on platform
		cmkmod |= CMK_MOD_ACCEL;
	}
	if((clmod & CLUTTER_MOD1_MASK) == CLUTTER_MOD1_MASK)
		cmkmod |= CMK_MOD_ALT;
	if((clmod & CLUTTER_BUTTON1_MASK) == CLUTTER_BUTTON1_MASK)
		cmkmod |= CMK_MOD_BUTTON1;
	if((clmod & CLUTTER_BUTTON2_MASK) == CLUTTER_BUTTON2_MASK)
		cmkmod |= CMK_MOD_BUTTON2;
	if((clmod & CLUTTER_BUTTON3_MASK) == CLUTTER_BUTTON3_MASK)
		cmkmod |= CMK_MOD_BUTTON3;
	if((clmod & CLUTTER_SUPER_MASK) == CLUTTER_SUPER_MASK)
		cmkmod |= CMK_MOD_SUPER;
	if((clmod & CLUTTER_HYPER_MASK) == CLUTTER_HYPER_MASK)
		cmkmod |= CMK_MOD_HYPER;
	if((clmod & CLUTTER_META_MASK) == CLUTTER_META_MASK)
		cmkmod |= CMK_MOD_META;

	return cmkmod;
}

static gboolean on_event(ClutterActor *self_, ClutterEvent *event)
{
	union
	{
		CmkEventButton button;
		CmkEventCrossing crossing;
		CmkEventMotion motion;
		CmkEventScroll scroll;
		CmkEventKey key;
		CmkEventText text;
		CmkEventFocus focus;
	} new = {0};

	new.button.time = clutter_event_get_time(event);

	ClutterEventType type = clutter_event_type(event);

	gfloat x, y;

	switch(type)
	{
	case CLUTTER_BUTTON_PRESS:
	case CLUTTER_BUTTON_RELEASE:
		new.button.type = CMK_EVENT_BUTTON;
		new.button.modifiers = translate_clutter_modifiers(event);
		clutter_actor_transform_stage_point(self_,
			((ClutterButtonEvent *)event)->x, ((ClutterButtonEvent *)event)->y,
			&x, &y);
		new.button.x = x;
		new.button.y = y;
		new.button.press = (type == CLUTTER_BUTTON_PRESS);
		new.button.button = ((ClutterButtonEvent *)event)->button;
		new.button.clickCount = ((ClutterButtonEvent *)event)->click_count;
		break;
	case CLUTTER_ENTER:
	case CLUTTER_LEAVE:
		new.crossing.type = CMK_EVENT_CROSSING;
		clutter_actor_transform_stage_point(self_,
			((ClutterCrossingEvent *)event)->x, ((ClutterCrossingEvent *)event)->y,
			&x, &y);
		new.button.x = x;
		new.button.y = y;
		new.crossing.enter = (type == CLUTTER_ENTER);
		break;
	case CLUTTER_MOTION:
		new.motion.type = CMK_EVENT_MOTION;
		new.motion.modifiers = translate_clutter_modifiers(event);
		clutter_actor_transform_stage_point(self_,
			((ClutterMotionEvent *)event)->x, ((ClutterMotionEvent *)event)->y,
			&x, &y);
		new.button.x = x;
		new.button.y = y;
		break;
	case CLUTTER_SCROLL:
		new.scroll.type = CMK_EVENT_SCROLL;
		new.scroll.modifiers = translate_clutter_modifiers(event);
		clutter_actor_transform_stage_point(self_,
			((ClutterScrollEvent *)event)->x, ((ClutterScrollEvent *)event)->y,
			&x, &y);
		new.button.x = x;
		new.button.y = y;
		if(((ClutterScrollEvent *)event)->direction == CLUTTER_SCROLL_DOWN)
			new.scroll.dx = 1, new.scroll.dy = 0;
		else if(((ClutterScrollEvent *)event)->direction == CLUTTER_SCROLL_UP)
			new.scroll.dx = -1, new.scroll.dy = 0;
		else if(((ClutterScrollEvent *)event)->direction == CLUTTER_SCROLL_LEFT)
			new.scroll.dx = 0, new.scroll.dy = -1;
		else if(((ClutterScrollEvent *)event)->direction == CLUTTER_SCROLL_RIGHT)
			new.scroll.dx = 0, new.scroll.dy = 1;
		else if(((ClutterScrollEvent *)event)->direction == CLUTTER_SCROLL_SMOOTH)
			clutter_event_get_scroll_delta(event, &new.scroll.dx, &new.scroll.dy);
		else
			new.scroll.dx = 0, new.scroll.dy = 0;
		break;
	case CLUTTER_KEY_PRESS:
	case CLUTTER_KEY_RELEASE:
		new.key.type = CMK_EVENT_KEY;
		new.key.modifiers = translate_clutter_modifiers(event);
		new.key.press = (type == CLUTTER_KEY_PRESS);
		new.key.keyval = ((ClutterKeyEvent *)event)->keyval;
		break;

	// TODO: CmkEventText
	// TODO: Keyboard focus change (no Clutter event for this).

	default:
		return CLUTTER_EVENT_PROPAGATE;
	}

	return cmk_widget_event(CMK_CLUTTER_WIDGET(self_)->widget, (CmkEvent *)&new);
}

static void on_reactive_changed(CmkClutterWidget *self, UNUSED GParamSpec *spec, UNUSED gpointer userdata)
{
	cmk_widget_set_disabled(self->widget, !clutter_actor_get_reactive(CLUTTER_ACTOR(self)));
}

// http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
static inline guint32 next_pot(guint32 v)
{
	--v;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	++v;
	return v;
}

static void on_paint_node(ClutterActor *self_, ClutterPaintNode *root)
{
	CmkClutterWidget *self = CMK_CLUTTER_WIDGET(self_);

	const ClutterPaintVolume *volume = clutter_actor_get_paint_volume(self_);
	gfloat width = clutter_paint_volume_get_width(volume);
	gfloat height = clutter_paint_volume_get_height(volume);

	ClutterVertex origin;
	clutter_paint_volume_get_origin(volume, &origin);
	ClutterActorBox box = {
		.x1 = origin.x,
		.y1 = origin.y,
		.x2 = origin.x + width,
		.y2 = origin.y + height,
	};

	gint surfWidth = width, surfHeight = height;
	guint texWidth = width, texHeight = height;
	// Some GPUs don't support non-power-of-two textures
	if(!self->npot)
	{
		texWidth = next_pot(texWidth);
		texHeight = next_pot(texHeight);
	}

	int windowScale = 1;
	// Not all builds of Clutter (specifically mutter-clutter
	// since commit 20fcb8863293) have this property, so make
	// sure it exists first.
	ClutterSettings *settings = clutter_settings_get_default();
	if(g_object_class_find_property(G_OBJECT_GET_CLASS(settings), "window-scaling-factor") != NULL)
		g_object_get(settings, "window-scaling-factor", &windowScale, NULL);

	texWidth *= windowScale;
	texHeight *= windowScale;
	surfWidth *= windowScale;
	surfHeight *= windowScale;

	// Ensure a large enough texture exists to draw into
	if(!self->tex
	|| cogl_texture_get_width(self->tex) < texWidth
	|| cogl_texture_get_height(self->tex) < texHeight
	// If the texture is over twice as big as the needed
	// size it's probably best to generate a smaller one.
	|| cogl_texture_get_width(self->tex) > texWidth * 2
	|| cogl_texture_get_height(self->tex) > texHeight * 2)
	{
		if(self->tex)
			cogl_object_unref(self->tex);
		self->tex = cogl_texture_2d_new_with_size(self->ctx, texWidth, texHeight);
		cogl_texture_set_premultiplied(self->tex, TRUE);
	}

	// Ensure a big enough surface
	if(!self->surface
	|| cairo_image_surface_get_width(self->surface) < surfWidth
	|| cairo_image_surface_get_height(self->surface) < surfHeight
	|| cairo_image_surface_get_width(self->surface) > surfWidth * 2
	|| cairo_image_surface_get_height(self->surface) > surfHeight * 2)
	{
		if(self->surface)
			cairo_surface_destroy(self->surface);
		self->surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, surfWidth, surfHeight);
		cairo_surface_set_device_scale(self->surface, windowScale, windowScale);

		// New surface, so the whole thing is dirty
		if(self->dirty)
		{
			cairo_region_destroy(self->dirty);
			self->dirty = NULL;
		}
	}

	// Cairo context
	cairo_t *cr = cairo_create(self->surface);

	// Establish clipping region
	if(self->dirty)
	{
		int n = cairo_region_num_rectangles(self->dirty);
		cairo_rectangle_int_t rect;
		for(int i = 0; i < n; ++i)
		{
			cairo_region_get_rectangle(self->dirty, i, &rect);
			cairo_rectangle(cr, rect.x, rect.y, rect.width, rect.height);
		}
		cairo_clip(cr);
	}

	// Draw
	// Translate from texture coordinates to allocation coordates
	cairo_translate(cr, -origin.x, -origin.y);
	cmk_widget_draw(CMK_CLUTTER_WIDGET(self_)->widget, cr);

	// Done painting
	cairo_destroy(cr);
	cairo_surface_flush(self->surface);

	// Upload surface to texture
	int stride = cairo_image_surface_get_stride(self->surface);
	uint8_t *data = cairo_image_surface_get_data(self->surface);
	if(self->dirty)
	{
		int n = cairo_region_num_rectangles(self->dirty);
		cairo_rectangle_int_t rect;
		for(int i = 0; i < n; ++i)
		{
			cairo_region_get_rectangle(self->dirty, i, &rect);
			cogl_texture_set_region(self->tex,
				rect.x * windowScale, rect.y * windowScale,
				rect.x * windowScale, rect.y * windowScale,
				rect.width * windowScale, rect.height * windowScale,
				rect.width * windowScale, rect.height * windowScale,
				CLUTTER_CAIRO_FORMAT_ARGB32,
				stride,
				data);
		}
		cairo_region_destroy(self->dirty);
		self->dirty = NULL;
	}
	else
	{
		cogl_texture_set_region(self->tex,
			0, 0,
			0, 0,
			width * windowScale, height * windowScale,
			width * windowScale, height * windowScale,
			CLUTTER_CAIRO_FORMAT_ARGB32,
			stride,
			data);
	}

	// Create a new, blank dirty region; not the same as NULL,
	// since NULL means the entire surface is dirty.
	self->dirty = cairo_region_create();

	// Add the texture node to the paint tree
	ClutterPaintNode *node = clutter_texture_node_new(self->tex,
		CLUTTER_COLOR_White,
		CLUTTER_SCALING_FILTER_TRILINEAR,
		CLUTTER_SCALING_FILTER_LINEAR);

	// The actual paint box might be smaller than the texture,
	// so only use the part of the texture that is drawn into.
	clutter_paint_node_add_texture_rectangle(node, &box,
		0, 0,
		width * windowScale / cogl_texture_get_width(self->tex),
		height * windowScale / cogl_texture_get_width(self->tex));

	clutter_paint_node_add_child(root, node);
	clutter_paint_node_unref(node);
}

static gboolean get_paint_volume(ClutterActor *self_, ClutterPaintVolume *volume)
{
	CmkRect rect;
	cmk_widget_get_draw_rect(CMK_CLUTTER_WIDGET(self_)->widget, &rect);

	ClutterVertex origin = {rect.x, rect.y, 0};
	clutter_paint_volume_set_origin(volume, &origin);
	clutter_paint_volume_set_width(volume, rect.width);
	clutter_paint_volume_set_height(volume, rect.height);
	return TRUE;
}

static void get_preferred_width(ClutterActor *self_, gfloat forHeight, gfloat *min, gfloat *nat)
{
	cmk_widget_get_preferred_width(CMK_CLUTTER_WIDGET(self_)->widget,
		forHeight, min, nat);
}

static void get_preferred_height(ClutterActor *self_, gfloat forWidth, gfloat *min, gfloat *nat)
{
	cmk_widget_get_preferred_height(CMK_CLUTTER_WIDGET(self_)->widget,
		forWidth, min, nat);
}

static void on_widget_request_invalidate(UNUSED CmkWidget *widget, const cairo_region_t *region, CmkClutterWidget *self)
{
	if(region)
	{
		if(self->dirty)
			cairo_region_union(self->dirty, region);

		cairo_rectangle_int_t extents;
		cairo_region_get_extents(region, &extents);
		clutter_actor_queue_redraw_with_clip(CLUTTER_ACTOR(self), &extents);
	}
	else
	{
		clutter_actor_queue_redraw(CLUTTER_ACTOR(self));

		if(self->dirty)
		{
			cairo_region_destroy(self->dirty);
			self->dirty = NULL;
		}
	}
}

static void on_widget_request_relayout(UNUSED CmkWidget *widget, UNUSED void *signaldata, CmkClutterWidget *self)
{
	clutter_actor_queue_relayout(CLUTTER_ACTOR(self));
}

static void on_widget_event_mask_changed(CmkWidget *widget, UNUSED void *signaldata, CmkClutterWidget *self)
{
	gboolean reactive = (cmk_widget_get_event_mask(widget) != 0);
	clutter_actor_set_reactive(CLUTTER_ACTOR(self), reactive);
}


static void on_frame(gpointer timeline)
{
	cmk_timeline_update(timeline, g_get_monotonic_time());
}

static void * cmk_clutter_timeline_callback(CmkTimeline *timeline, bool start, uint64_t *time, void *userdata)
{
	if(time)
		*time = g_get_monotonic_time();
	
	if(start) // Request to start timeline
	{
		cmk_timeline_ref(timeline);

		// Clutter's master clock has no external API, so attach
		// to it using a ClutterTimeline which runs forever.
		ClutterTimeline *ctimeline = clutter_timeline_new(10000000);
		clutter_timeline_set_repeat_count(ctimeline, -1); // Always repeat
		g_signal_connect_swapped(ctimeline, "new-frame", G_CALLBACK(on_frame), timeline);
		clutter_timeline_start(ctimeline);

		// This is passed back as userdata in the 'stop' event
		return ctimeline;
	}
	else // Request to stop timeline
	{
		ClutterTimeline *timeline = userdata;
		clutter_timeline_stop(timeline);
		g_object_unref(timeline);
		return NULL;
	}
}
