#include "cmk-gtk.h"

struct _CmkGtkWidget
{
	GtkWidget parent;

	CmkWidget *widget;
	GdkWindow *eventWindow;
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
static void on_disable(CmkGtkWidget *self, GParamSpec *spec, gpointer userdata);
static void get_preferred_width_for_height(GtkWidget *self_, gint height, gint *min, gint *nat);
static void get_preferred_width(GtkWidget *self_, gint *min, gint *nat);
static void get_preferred_height_for_width(GtkWidget *self_, gint width, gint *min, gint *nat);
static void get_preferred_height(GtkWidget *self_, gint *min, gint *nat);
static void on_widget_request_invalidate(CmkWidget *widget, const cairo_region_t *region, CmkGtkWidget *self);
static void on_widget_request_relayout(CmkWidget *widget, CmkGtkWidget *self);


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

	// Call this again now that self->widget is set
	cmk_gtk_widget_init(self);

	return GTK_WIDGET(self);
}

static void cmk_gtk_widget_class_init(CmkGtkWidgetClass *class)
{
	GObjectClass *base = G_OBJECT_CLASS(class);
	base->dispose = on_dispose;

	GtkWidgetClass *gwclass = GTK_WIDGET_CLASS(class);
	gwclass->size_allocate = on_size_allocate;
	gwclass->draw = on_draw;
	gwclass->event = on_event;
	gwclass->realize = on_realize;
	gwclass->unrealize = on_unrealize;
	gwclass->map = on_map;
	gwclass->unmap = on_unmap;
	gwclass->get_preferred_width = get_preferred_width;
	gwclass->get_preferred_width_for_height = get_preferred_width_for_height;
	gwclass->get_preferred_height = get_preferred_height;
	gwclass->get_preferred_height_for_width = get_preferred_height_for_width;
}

static void cmk_gtk_widget_init(CmkGtkWidget *self)
{
	if(self->widget == NULL)
	{
		// True cmk_gtk_widget_init

		gtk_widget_set_has_window(GTK_WIDGET(self), FALSE);
		gtk_widget_set_can_focus(GTK_WIDGET(self), TRUE);
		gtk_widget_set_can_default(GTK_WIDGET(self), TRUE);
		gtk_widget_set_receives_default(GTK_WIDGET(self), TRUE);
		return;
	}

	// Called again after self->widget is set

	g_signal_connect(self->widget, "invalidate", G_CALLBACK(on_widget_request_invalidate), self);
	g_signal_connect(self->widget, "relayout", G_CALLBACK(on_widget_request_relayout), self);
	g_signal_connect(self, "notify::sensitve", G_CALLBACK(on_disable), NULL);
}

static void on_dispose(GObject *self_)
{
	CmkGtkWidget *self = CMK_GTK_WIDGET(self_);
	if(self->widget)
		g_signal_handlers_disconnect_by_data(self->widget, self);
	g_clear_object(&self->widget);
	G_OBJECT_CLASS(cmk_gtk_widget_parent_class)->dispose(self_);
}

static void on_realize(GtkWidget *self_)
{
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
	attributes.event_mask |= (GDK_BUTTON_PRESS_MASK |
		GDK_BUTTON_RELEASE_MASK |
		GDK_TOUCH_MASK |
		GDK_ENTER_NOTIFY_MASK |
		GDK_LEAVE_NOTIFY_MASK);

	GdkWindow *window = gtk_widget_get_parent_window(self_);
	gtk_widget_set_window(self_, window);
	g_object_ref(window);

	CMK_GTK_WIDGET(self_)->eventWindow = gdk_window_new(window,
		&attributes, GDK_WA_X | GDK_WA_Y);
	gtk_widget_register_window(self_, CMK_GTK_WIDGET(self_)->eventWindow);
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

	gtk_widget_set_allocation(self_, allocation);
	cmk_widget_set_size(self->widget,
		(float)allocation->width, (float)allocation->height);

	if(gtk_widget_get_realized(self_))
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
		new.button.modifiers = translate_gdk_modifiers(event);
		gdk_event_get_coords(event, &new.button.x, &new.button.y);
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
		gdk_event_get_keyval(event, &new.key.keyval);
		break;
	case GDK_FOCUS_CHANGE:
		new.focus.type = CMK_EVENT_FOCUS;
		new.focus.in = (gboolean)((GdkEventFocus *)event)->in;
		break;
	default:
		// TODO: Unsupported event type.
		return GDK_EVENT_PROPAGATE;
	}

	return cmk_widget_event(CMK_GTK_WIDGET(self_)->widget, (CmkEvent *)&new);
}

static void on_disable(CmkGtkWidget *self, UNUSED GParamSpec *spec, UNUSED gpointer userdata)
{
	cmk_widget_set_disabled(self->widget, !gtk_widget_get_sensitive(GTK_WIDGET(self)));
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
		gtk_widget_queue_draw_region(GTK_WIDGET(self), region);
	else
		gtk_widget_queue_draw(GTK_WIDGET(self));
}

static void on_widget_request_relayout(UNUSED CmkWidget *widget, CmkGtkWidget *self)
{
	gtk_widget_queue_resize(GTK_WIDGET(self));
}

CmkWidget * cmk_gtk_widget_get_widget(CmkGtkWidget *self)
{
	g_return_val_if_fail(CMK_IS_GTK_WIDGET(self), NULL);
	return self->widget;
}
