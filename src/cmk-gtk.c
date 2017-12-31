/*
 * libcmk, Copyright 2018 Aidan Shafran <zelbrium@gmail.com>
 * Apache License 2.0 <www.apache.org/licenses/LICENSE-2.0>.
 */

#include "cmk-gtk.h"
#include "cmk-timeline.h"

struct _CmkGtkWidget
{
	GtkWidget parent;

	CmkWidget *widget;
	GdkWindow *eventWindow;
	bool hasPangoContext;
};

static void cmk_gtk_widget_init(CmkGtkWidget *self);
static void on_dispose(GObject *self_);
static void on_realize(GtkWidget *self_);
static void on_unrealize(GtkWidget *self_);
static void on_map(GtkWidget *self_);
static void on_unmap(GtkWidget *self_);
static void on_size_allocate(GtkWidget *self_, GtkAllocation *allocation);
static gboolean on_draw(GtkWidget *self_, cairo_t *cr);
static gboolean on_event(GtkWidget *self_, GdkEvent *event);
static void on_sensitivity_changed(CmkGtkWidget *self, GParamSpec *spec, gpointer userdata);
static void on_screen_changed(GtkWidget *self_, GdkScreen *prevScreen);
static void on_style_updated(GtkWidget *self_);
static void get_preferred_width_for_height(GtkWidget *self_, gint height, gint *min, gint *nat);
static void get_preferred_width(GtkWidget *self_, gint *min, gint *nat);
static void get_preferred_height_for_width(GtkWidget *self_, gint width, gint *min, gint *nat);
static void get_preferred_height(GtkWidget *self_, gint *min, gint *nat);
static void on_widget_request_invalidate(CmkWidget *widget, const cairo_region_t *region, CmkGtkWidget *self);
static void on_widget_request_relayout(CmkWidget *widget, void *signaldata, CmkGtkWidget *self);
static void on_widget_event_mask_changed(CmkWidget *widget, GParamSpec *spec, CmkGtkWidget *self);
static void * cmk_gtk_timeline_callback(CmkTimeline *timeline, bool start, uint64_t *time, void *userdata);


G_DEFINE_TYPE(CmkGtkWidget, cmk_gtk_widget, GTK_TYPE_WIDGET);


GtkWidget * cmk_widget_to_gtk(CmkWidget *widget)
{
	g_return_val_if_fail(CMK_IS_WIDGET(widget), NULL);

	// Return existing wrapper object if available
	CmkGtkWidget *wrapper = cmk_widget_get_wrapper(widget);
	if(wrapper != NULL)
	{
		g_return_val_if_fail(CMK_IS_GTK_WIDGET(wrapper), NULL);
		return GTK_WIDGET(wrapper);
	}

	// Create new wrapper
	CmkGtkWidget *self = CMK_GTK_WIDGET(g_object_new(CMK_TYPE_GTK_WIDGET, NULL));
	g_return_val_if_fail(CMK_IS_GTK_WIDGET(self), NULL);

	// Connect wrapper and widget
	self->widget = g_object_ref_sink(widget);
	cmk_widget_set_wrapper(widget, self);
	g_signal_connect(widget, "invalidate", G_CALLBACK(on_widget_request_invalidate), self);
	g_signal_connect(widget, "relayout", G_CALLBACK(on_widget_request_relayout), self);
	g_signal_connect(widget, "notify::event-mask", G_CALLBACK(on_widget_event_mask_changed), self);
	g_signal_connect(self, "notify::sensitve", G_CALLBACK(on_sensitivity_changed), NULL);

	self->hasPangoContext = (g_object_class_find_property(G_OBJECT_GET_CLASS(widget), "pango-context") != NULL);

	cmk_timeline_set_handler_callback(cmk_gtk_timeline_callback, false);

	return GTK_WIDGET(self);
}

static void cmk_gtk_widget_class_init(CmkGtkWidgetClass *class)
{
	GObjectClass *base = G_OBJECT_CLASS(class);
	base->dispose = on_dispose;

	GtkWidgetClass *widgetClass = GTK_WIDGET_CLASS(class);
	widgetClass->size_allocate = on_size_allocate;
	widgetClass->draw = on_draw;
	widgetClass->event = on_event;
	widgetClass->realize = on_realize;
	widgetClass->unrealize = on_unrealize;
	widgetClass->map = on_map;
	widgetClass->unmap = on_unmap;
	widgetClass->get_preferred_width = get_preferred_width;
	widgetClass->get_preferred_width_for_height = get_preferred_width_for_height;
	widgetClass->get_preferred_height = get_preferred_height;
	widgetClass->get_preferred_height_for_width = get_preferred_height_for_width;
	widgetClass->screen_changed = on_screen_changed;
	widgetClass->style_updated = on_style_updated;
}

static void cmk_gtk_widget_init(CmkGtkWidget *self)
{
	gtk_widget_set_has_window(GTK_WIDGET(self), FALSE);
	gtk_widget_set_can_focus(GTK_WIDGET(self), TRUE);
	gtk_widget_set_can_default(GTK_WIDGET(self), TRUE);
	gtk_widget_set_receives_default(GTK_WIDGET(self), TRUE);
}

static void on_dispose(GObject *self_)
{
	CmkGtkWidget *self = CMK_GTK_WIDGET(self_);
	if(self->widget)
	{
		g_signal_handlers_disconnect_by_data(self->widget, self);
		cmk_widget_set_wrapper(self->widget, NULL);
	}
	g_clear_object(&self->widget);
	G_OBJECT_CLASS(cmk_gtk_widget_parent_class)->dispose(self_);
}

static void on_realize(GtkWidget *self_)
{
	CmkGtkWidget *self = CMK_GTK_WIDGET(self_);

	GtkAllocation allocation;
	gtk_widget_get_allocation(self_, &allocation);

	gtk_widget_set_realized(self_, TRUE);

	GdkWindowAttr attributes;
	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.x = allocation.x;
	attributes.y = allocation.y;
	attributes.width = allocation.width;
	attributes.height = allocation.height;
	attributes.wclass = GDK_INPUT_ONLY;
	attributes.event_mask = gtk_widget_get_events(self_);

	GdkWindow *window = gtk_widget_get_parent_window(self_);
	gtk_widget_set_window(self_, window);
	g_object_ref(window);

	self->eventWindow = gdk_window_new(window,
		&attributes, GDK_WA_X | GDK_WA_Y);
	gtk_widget_register_window(self_, CMK_GTK_WIDGET(self_)->eventWindow);

	if(self->widget)
		on_widget_event_mask_changed(self->widget, NULL, self);
}


static void on_unrealize(GtkWidget *self_)
{
	CmkGtkWidget *self = CMK_GTK_WIDGET(self_);

	if(self->eventWindow)
	{
		gtk_widget_unregister_window(self_, self->eventWindow);
		gdk_window_destroy(self->eventWindow);
		self->eventWindow = NULL;
	}

	GTK_WIDGET_CLASS(cmk_gtk_widget_parent_class)->unrealize(self_);
}

static void on_map(GtkWidget *self_)
{
	GTK_WIDGET_CLASS(cmk_gtk_widget_parent_class)->map(self_);

	CmkGtkWidget *self = CMK_GTK_WIDGET(self_);
	if(self->eventWindow)
		gdk_window_show(self->eventWindow);
}

static void on_unmap(GtkWidget *self_)
{
	CmkGtkWidget *self = CMK_GTK_WIDGET(self_);
	if(self->eventWindow)
		gdk_window_hide(self->eventWindow);

	GTK_WIDGET_CLASS(cmk_gtk_widget_parent_class)->unmap(self_);
}

static void on_size_allocate(GtkWidget *self_, GtkAllocation *allocation)
{
	CmkGtkWidget *self = CMK_GTK_WIDGET(self_);

	// Set widget allocation
	gtk_widget_set_allocation(self_, allocation);
	cmk_widget_set_size(self->widget,
		(float)allocation->width, (float)allocation->height);

	// Adjust the GtkWidget's clip to
	// match the CmkWidget's draw_area
	CmkRect rect;
	cmk_widget_get_draw_rect(self->widget, &rect);
	GdkRectangle clip = {
		.x = (gint)rect.x + allocation->x,
		.y = (gint)rect.y + allocation->y,
		.width = rect.width,
		.height = rect.height};
	gtk_widget_set_clip(self_, &clip);

	// Move event window
	if(self->eventWindow)
	{
		gdk_window_move_resize(self->eventWindow,
			allocation->x,
			allocation->y,
			allocation->width,
			allocation->height);
	}
}

static gboolean on_draw(GtkWidget *self_, cairo_t *cr)
{
	cmk_widget_draw(CMK_GTK_WIDGET(self_)->widget, cr);
	return TRUE;
}

static guint translate_gdk_modifiers(const GdkEvent *event)
{
	guint gdkmod = 0;
	if(!gdk_event_get_state(event, &gdkmod))
		return 0;
	guint cmkmod = 0;
	if((gdkmod & GDK_SHIFT_MASK) == GDK_SHIFT_MASK)
		cmkmod |= CMK_MOD_SHIFT;
	if((gdkmod & GDK_LOCK_MASK) == GDK_LOCK_MASK)
		cmkmod |= CMK_MOD_CAPS_LOCK;
	if((gdkmod & GDK_CONTROL_MASK) == GDK_CONTROL_MASK)
		cmkmod |= CMK_MOD_CONTROL;
	if((gdkmod & GDK_MOD1_MASK) == GDK_MOD1_MASK)
		cmkmod |= CMK_MOD_ALT;
	if((gdkmod & GDK_BUTTON1_MASK) == GDK_BUTTON1_MASK)
		cmkmod |= CMK_MOD_BUTTON1;
	if((gdkmod & GDK_BUTTON2_MASK) == GDK_BUTTON2_MASK)
		cmkmod |= CMK_MOD_BUTTON2;
	if((gdkmod & GDK_BUTTON3_MASK) == GDK_BUTTON3_MASK)
		cmkmod |= CMK_MOD_BUTTON3;
	if((gdkmod & GDK_SUPER_MASK) == GDK_SUPER_MASK)
		cmkmod |= CMK_MOD_SUPER;
	if((gdkmod & GDK_HYPER_MASK) == GDK_HYPER_MASK)
		cmkmod |= CMK_MOD_HYPER;
	if((gdkmod & GDK_META_MASK) == GDK_META_MASK)
		cmkmod |= CMK_MOD_META;

	GdkWindow *window = gdk_event_get_window(event);
	GdkDisplay *display = gdk_window_get_display(window);
	GdkKeymap *map = gdk_keymap_get_for_display(display);
	GdkModifierType menuaccel = gdk_keymap_get_modifier_mask(map, GDK_MODIFIER_INTENT_PRIMARY_ACCELERATOR);
	if((gdkmod & menuaccel) == menuaccel)
		cmkmod |= CMK_MOD_ACCEL;

	return cmkmod;
}

static gboolean on_event(GtkWidget *self_, GdkEvent *event)
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

	new.button.time = gdk_event_get_time(event);

	// GdkEvent coordinates are window-relative, but it seems
	// they're relative to the widget's window, not the toplevel
	// window. So need to adjust coordinates.

	// Convert GdkEvent to CmkEvent
	switch(event->type)
	{
	case GDK_BUTTON_PRESS:
	case GDK_BUTTON_RELEASE:
		new.button.type = CMK_EVENT_BUTTON;
		new.button.modifiers = translate_gdk_modifiers(event);
		gdk_event_get_coords(event, &new.button.x, &new.button.y);
		new.button.press = (event->type == GDK_BUTTON_PRESS);
		gdk_event_get_button(event, &new.button.button);
		// TODO: Click count is always 1 for GDK_BUTTON_PRESS.
		// Must manually track click count for GDK.
		new.button.clickCount = 1;
		break;
	case GDK_ENTER_NOTIFY:
	case GDK_LEAVE_NOTIFY:
		new.crossing.type = CMK_EVENT_CROSSING;
		gdk_event_get_coords(event, &new.crossing.x, &new.crossing.y);
		new.crossing.enter = (event->type == GDK_ENTER_NOTIFY);
		break;
	case GDK_MOTION_NOTIFY:
		new.motion.type = CMK_EVENT_MOTION;
		new.motion.modifiers = translate_gdk_modifiers(event);
		gdk_event_get_coords(event, &new.motion.x, &new.motion.y);
		break;
	case GDK_SCROLL:
		new.scroll.type = CMK_EVENT_SCROLL;
		new.scroll.modifiers = translate_gdk_modifiers(event);
		gdk_event_get_coords(event, &new.scroll.x, &new.scroll.y);
		if(((GdkEventScroll *)event)->direction == GDK_SCROLL_DOWN)
			new.scroll.dx = 1, new.scroll.dy = 0;
		else if(((GdkEventScroll *)event)->direction == GDK_SCROLL_UP)
			new.scroll.dx = -1, new.scroll.dy = 0;
		else if(((GdkEventScroll *)event)->direction == GDK_SCROLL_LEFT)
			new.scroll.dx = 0, new.scroll.dy = -1;
		else if(((GdkEventScroll *)event)->direction == GDK_SCROLL_RIGHT)
			new.scroll.dx = 0, new.scroll.dy = 1;
		else if(((GdkEventScroll *)event)->direction == GDK_SCROLL_SMOOTH)
			gdk_event_get_scroll_deltas(event, &new.scroll.dx, &new.scroll.dy);
		else
			new.scroll.dx = 0, new.scroll.dy = 0;
		break;
	case GDK_KEY_PRESS:
	case GDK_KEY_RELEASE:
		new.key.type = CMK_EVENT_KEY;
		new.key.modifiers = translate_gdk_modifiers(event);
		new.key.press = (event->type == GDK_KEY_PRESS);
		gdk_event_get_keyval(event, &new.key.keyval);
		break;
	case GDK_FOCUS_CHANGE:
		new.focus.type = CMK_EVENT_FOCUS;
		new.focus.in = (gboolean)((GdkEventFocus *)event)->in;
		break;
	// TODO: CmkEventText
	default:
		// TODO: Unsupported event type.
		return GDK_EVENT_PROPAGATE;
	}

	return cmk_widget_event(CMK_GTK_WIDGET(self_)->widget, (CmkEvent *)&new);
}

static void on_sensitivity_changed(CmkGtkWidget *self, UNUSED GParamSpec *spec, UNUSED gpointer userdata)
{
	cmk_widget_set_disabled(self->widget, !gtk_widget_get_sensitive(GTK_WIDGET(self)));
}

static void update_pango_context(GtkWidget *self_)
{
	CmkWidget *widget = CMK_GTK_WIDGET(self_)->widget;
	if(CMK_GTK_WIDGET(self_)->hasPangoContext)
	{
		g_object_set(widget,
			"pango-context", gtk_widget_get_pango_context(self_),
			NULL);
	}
}

static void on_screen_changed(GtkWidget *self_, UNUSED GdkScreen *prevScreen)
{
	update_pango_context(self_);
}

static void on_style_updated(GtkWidget *self_)
{
	GTK_WIDGET_CLASS(cmk_gtk_widget_parent_class)->style_updated(self_);
	update_pango_context(self_);
}

static void get_preferred_width_for_height(GtkWidget *self_, gint height, gint *min, gint *nat)
{
	float fmin, fnat;
	cmk_widget_get_preferred_width(CMK_GTK_WIDGET(self_)->widget,
		(float)height, &fmin, &fnat);
	*min = (gint)fmin;
	*nat = (gint)fnat;
}

static void get_preferred_width(GtkWidget *self_, gint *min, gint *nat)
{
	float fmin, fnat;
	cmk_widget_get_preferred_width(CMK_GTK_WIDGET(self_)->widget,
		-1, &fmin, &fnat);
	*min = (gint)fmin;
	*nat = (gint)fnat;
}

static void get_preferred_height_for_width(GtkWidget *self_, gint width, gint *min, gint *nat)
{
	float fmin, fnat;
	cmk_widget_get_preferred_height(CMK_GTK_WIDGET(self_)->widget,
		(float)width, &fmin, &fnat);
	*min = (gint)fmin;
	*nat = (gint)fnat;
}

static void get_preferred_height(GtkWidget *self_, gint *min, gint *nat)
{
	float fmin, fnat;
	cmk_widget_get_preferred_height(CMK_GTK_WIDGET(self_)->widget,
		-1, &fmin, &fnat);
	*min = (gint)fmin;
	*nat = (gint)fnat;
}

static void on_widget_request_invalidate(UNUSED CmkWidget *widget, const cairo_region_t *region, CmkGtkWidget *self)
{
	if(region)
	{
		// Translate region to window coordinates
		GtkAllocation allocation;
		gtk_widget_get_allocation(GTK_WIDGET(self), &allocation);

		cairo_region_t *r = cairo_region_copy(region);
		cairo_region_translate(r, allocation.x, allocation.y);

		gtk_widget_queue_draw_region(GTK_WIDGET(self), r);

		cairo_region_destroy(r);
	}
	else
		gtk_widget_queue_draw(GTK_WIDGET(self));
}

static void on_widget_request_relayout(UNUSED CmkWidget *widget, UNUSED void *signaldata, CmkGtkWidget *self)
{
	gtk_widget_queue_resize(GTK_WIDGET(self));
}

static void on_widget_event_mask_changed(CmkWidget *widget, UNUSED GParamSpec *spec, CmkGtkWidget *self)
{
	if(!self->eventWindow)
		return;

	unsigned int widgetMask = cmk_widget_get_event_mask(widget);
	guint events = 0;

	if(widgetMask & CMK_EVENT_BUTTON)
		events |= GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK;
	if(widgetMask & CMK_EVENT_CROSSING)
		events |= GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK;
	if(widgetMask & CMK_EVENT_MOTION)
		events |= GDK_POINTER_MOTION_MASK | GDK_BUTTON_MOTION_MASK;
	if(widgetMask & CMK_EVENT_KEY)
		events |= GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK;
	// TODO: CMK_EVENT_TEXT ?
	if(widgetMask & CMK_EVENT_FOCUS)
		events |= GDK_FOCUS_CHANGE_MASK;
	if(widgetMask & CMK_EVENT_SCROLL)
		events |= GDK_SCROLL_MASK;
	
	events |= gtk_widget_get_events(GTK_WIDGET(self));

	gdk_window_set_events(self->eventWindow, events);
}

CmkWidget * cmk_gtk_widget_get_widget(CmkGtkWidget *self)
{
	g_return_val_if_fail(CMK_IS_GTK_WIDGET(self), NULL);
	return self->widget;
}


static gboolean on_frame(UNUSED GtkWidget *self_, GdkFrameClock *clock, gpointer timeline)
{
	cmk_timeline_update(timeline, gdk_frame_clock_get_frame_time(clock));
	return G_SOURCE_CONTINUE;
}

static void * cmk_gtk_timeline_callback(CmkTimeline *timeline, bool start, uint64_t *time, void *userdata)
{
	// Get the GtkWidget that this CmkTimeline belongs to
	CmkWidget *widget = cmk_timeline_get_widget(timeline);
	GtkWidget *gtkwidget = GTK_WIDGET(cmk_widget_get_wrapper(widget));
	g_return_val_if_fail(CMK_IS_GTK_WIDGET(gtkwidget), NULL);

	// Get the time
	if(time)
		*time = gdk_frame_clock_get_frame_time(gtk_widget_get_frame_clock(gtkwidget));

	if(start) // Request to start timeline
	{
		cmk_timeline_ref(timeline);
		gint id = gtk_widget_add_tick_callback(gtkwidget,
			on_frame, timeline, (GDestroyNotify)cmk_timeline_unref);

		// This is passed back as userdata in the 'stop' event
		return GINT_TO_POINTER(id);
	}
	else // Request to stop timeline
	{
		gtk_widget_remove_tick_callback(gtkwidget, GPOINTER_TO_INT(userdata));
		return NULL;
	}
}
