/*
 * libcmk, Copyright 2018 Aidan Shafran <zelbrium@gmail.com>
 * Apache License 2.0 <www.apache.org/licenses/LICENSE-2.0>.
 */

#include <stdlib.h>
#include <string.h>
#include "cmk-widget.h"

#define CMK_WIDGET_EVENT_MASK 0
//CMK_EVENT_BUTTON | CMK_EVENT_CROSSING | CMK_EVENT_KEY | CMK_EVENT_FOCUS

typedef struct _CmkWidgetPrivate CmkWidgetPrivate;
struct _CmkWidgetPrivate
{
	void *wrapper;

	bool disabled;
	float setWidth, setHeight;

	// Preferred width / height cache
	float prefWforH, prefWmin, prefWnat;
	float prefHforW, prefHmin, prefHnat;
	bool prefWvalid, prefHvalid;

	// Draw rect cache
	CmkRect drawRect;
	bool drawRectValid;
};

enum
{
	PROP_DISABLED = 1,
	PROP_EVENT_MASK,
	PROP_WRAPPER,
	PROP_LAST
};

enum
{
	SIGNAL_INVALIDATE = 1,
	SIGNAL_RELAYOUT,
	SIGNAL_LAST
};

static GParamSpec *properties[PROP_LAST];
static guint signals[SIGNAL_LAST];

G_DEFINE_TYPE_WITH_PRIVATE(CmkWidget, cmk_widget, G_TYPE_INITIALLY_UNOWNED);
#define PRIV(widget) ((CmkWidgetPrivate *)cmk_widget_get_instance_private(widget))


static void on_dispose(GObject *self_);
static void set_property(GObject *self_, guint id, const GValue *value, GParamSpec *pspec);
static void get_property(GObject *self_, guint id, GValue *value, GParamSpec *pspec);
static void on_draw(CmkWidget *self, cairo_t *cr);
static bool on_event(CmkWidget *self, const CmkEvent *event);
static void on_disable(CmkWidget *self, bool disabled);
static void get_preferred_width(CmkWidget *self, float forHeight, float *min, float *nat);
static void get_preferred_height(CmkWidget *self, float forWidth, float *min, float *nat);
static void get_draw_rect(CmkWidget *self, CmkRect *rect);


CmkWidget * cmk_widget_new(void)
{
	return CMK_WIDGET(g_object_new(CMK_TYPE_WIDGET, NULL));
}

static void cmk_widget_class_init(CmkWidgetClass *class)
{
	GObjectClass *base = G_OBJECT_CLASS(class);
	base->dispose = on_dispose;
	base->get_property = get_property;
	base->set_property = set_property;

	class->draw = on_draw;
	class->disable = on_disable;
	class->event = on_event;
	class->get_preferred_width = get_preferred_width;
	class->get_preferred_height = get_preferred_height;
	class->get_draw_rect = get_draw_rect;

	/**
	 * CmkWidget:disabled:
	 *
	 * Makes the widget gray and inactive. Usually set
	 * automatically by the widget's wrapper based on the
	 * wrapper class's disabled property, and so probably
	 * shouldn't be set directly.
	 */
	properties[PROP_DISABLED] =
		g_param_spec_boolean("disabled", "disabled", "disabled",
		                     false,
		                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

	/**
	 * CmkWidget:event-mask:
	 *
	 * The #CmkEvent types that this widget will respond to.
	 * The widget's wrapper may optimize by not even sending
	 * these events.
	 */
	properties[PROP_EVENT_MASK] =
		g_param_spec_uint("event-mask", "event-mask", "event-mask",
		                  0, CMK_EVENT_MASK_ALL, 0,
		                  G_PARAM_READABLE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

	/**
	 * CmkWidget:wrapper:
	 *
	 * A toolkit-specific wrapper object.
	 */
	properties[PROP_WRAPPER] =
		g_param_spec_pointer("wrapper", "wrapper", "wrapper",
		                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

	g_object_class_install_properties(base, PROP_LAST, properties);

	/**
	 * CmkWidget::invalidate:
	 * @widget: The widget that was invalidated.
	 * @region: a const cairo_region_t * specifying the region
	 *          invalidated, or NULL to requests the entire
	 *          area of the widget.
	 *
	 * A redraw of the widget should be performed.
	 */
	signals[SIGNAL_INVALIDATE] =
		g_signal_new("invalidate",
		             G_TYPE_FROM_CLASS(class),
		             G_SIGNAL_RUN_FIRST,
		             0, NULL, NULL, NULL,
		             G_TYPE_NONE,
		             1, G_TYPE_POINTER);

	/**
	 * CmkWidget::relayout:
	 * @widget: The widget that requested relayout.
	 *
	 * The widget's size request has changed, and a relayout
	 * should be performed.
	 */
	signals[SIGNAL_RELAYOUT] =
		g_signal_new("relayout",
		             G_TYPE_FROM_CLASS(class),
		             G_SIGNAL_RUN_FIRST,
		             0, NULL, NULL, NULL,
		             G_TYPE_NONE,
		             0);
}


static void cmk_widget_init(CmkWidget *self)
{
	CmkWidgetPrivate *priv = PRIV(self);
	priv->setWidth = priv->setHeight = -1;
}

static void on_dispose(GObject *self_)
{
	G_OBJECT_CLASS(cmk_widget_parent_class)->dispose(self_);
}

static void get_property(GObject *self_, guint id, GValue *value, GParamSpec *pspec)
{
	CmkWidget *self = CMK_WIDGET(self_);
	
	switch(id)
	{
	case PROP_DISABLED:
		g_value_set_boolean(value, PRIV(self)->disabled);
		break;
	case PROP_EVENT_MASK:
		g_value_set_uint(value, CMK_WIDGET_EVENT_MASK);
		break;
	case PROP_WRAPPER:
		g_value_set_pointer(value, PRIV(self)->wrapper);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(self, id, pspec);
		break;
	}
}

static void set_property(GObject *self_, guint id, const GValue *value, GParamSpec *pspec)
{
	CmkWidget *self = CMK_WIDGET(self_);
	
	switch(id)
	{
	case PROP_DISABLED:
		cmk_widget_set_disabled(self, g_value_get_boolean(value));
		break;
	case PROP_WRAPPER:
		cmk_widget_set_wrapper(self, g_value_get_pointer(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(self, id, pspec);
		break;
	}
}

static void on_draw(UNUSED CmkWidget *self, UNUSED cairo_t *cr)
{
	// Do nothing
}

static bool on_event(UNUSED CmkWidget *self, UNUSED const CmkEvent *event)
{
	return false;
}

static void on_disable(CmkWidget *self, UNUSED bool disabled)
{
	// TODO: Fade the widget out to gray or in to color
	cmk_widget_invalidate(self, NULL);
}

static void get_preferred_width(UNUSED CmkWidget *self, UNUSED float forHeight, float *min, float *nat)
{
	*min = *nat = 0;
}

static void get_preferred_height(UNUSED CmkWidget *self, UNUSED float forWidth, float *min, float *nat)
{
	*min = *nat = 0;
}

static void get_draw_rect(CmkWidget *self, CmkRect *rect)
{
	rect->x = rect->y = 0;
	rect->width = PRIV(self)->setWidth;
	rect->height = PRIV(self)->setHeight;
}

void cmk_widget_set_wrapper(CmkWidget *self, void *wrapper)
{
	g_return_if_fail(CMK_IS_WIDGET(self));
	if(PRIV(self)->wrapper != wrapper)
	{
		PRIV(self)->wrapper = wrapper;
		g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_WRAPPER]);
	}
}

void * cmk_widget_get_wrapper(CmkWidget *self)
{
	g_return_val_if_fail(CMK_IS_WIDGET(self), NULL);
	return PRIV(self)->wrapper;
}

void cmk_widget_invalidate(CmkWidget *self, const cairo_region_t *region)
{
	g_return_if_fail(CMK_IS_WIDGET(self));
	g_signal_emit(self, signals[SIGNAL_INVALIDATE], 0, region);
}

void cmk_widget_relayout(CmkWidget *self)
{
	g_return_if_fail(CMK_IS_WIDGET(self));

	PRIV(self)->prefWvalid = PRIV(self)->prefHvalid = false;
	PRIV(self)->drawRectValid = false;

	g_signal_emit(self, signals[SIGNAL_RELAYOUT], 0);
}

void cmk_widget_draw(CmkWidget *self, cairo_t *cr)
{
	g_return_if_fail(CMK_IS_WIDGET(self));
	g_return_if_fail(cr != NULL);

	if(CMK_WIDGET_GET_CLASS(self)->draw)
		CMK_WIDGET_GET_CLASS(self)->draw(self, cr);
}

bool cmk_widget_event(CmkWidget *self, const CmkEvent *event)
{
	g_return_val_if_fail(CMK_IS_WIDGET(self), false);
	g_return_val_if_fail(event != NULL, false);

	if(CMK_WIDGET_GET_CLASS(self)->event)
		CMK_WIDGET_GET_CLASS(self)->event(self, event);
	return false;
}

unsigned int cmk_widget_get_event_mask(CmkWidget *self)
{
	g_return_val_if_fail(CMK_IS_WIDGET(self), 0);
	unsigned int mask;
	g_object_get(self, "event-mask", &mask, NULL);
	return mask;
}

void cmk_widget_get_preferred_width(CmkWidget *self, float forHeight, float *min, float *nat)
{
	g_return_if_fail(CMK_IS_WIDGET(self));

	if(PRIV(self)->prefWvalid && PRIV(self)->prefWforH == forHeight)
	{
		// Return cached value
		if(min) *min = PRIV(self)->prefWmin;
		if(nat) *nat = PRIV(self)->prefWnat;
		return;
	}

	float m = 0, n = 0;
	if(min == NULL) min = &m;
	if(nat == NULL) nat = &n;

	if(CMK_WIDGET_GET_CLASS(self)->get_preferred_width)
		CMK_WIDGET_GET_CLASS(self)->get_preferred_width(self, forHeight, min, nat);

	PRIV(self)->prefWforH = forHeight;
	PRIV(self)->prefWmin = *min;
	PRIV(self)->prefWnat = *nat;
	PRIV(self)->prefWvalid = true;
}

void cmk_widget_get_preferred_height(CmkWidget *self, float forWidth, float *min, float *nat)
{
	g_return_if_fail(CMK_IS_WIDGET(self));

	if(PRIV(self)->prefHvalid && PRIV(self)->prefHforW == forWidth)
	{
		// Return cached value
		if(min) *min = PRIV(self)->prefHmin;
		if(nat) *nat = PRIV(self)->prefHnat;
		return;
	}

	float m = 0, n = 0;
	if(min == NULL) min = &m;
	if(nat == NULL) nat = &n;

	if(CMK_WIDGET_GET_CLASS(self)->get_preferred_height)
		CMK_WIDGET_GET_CLASS(self)->get_preferred_height(self, forWidth, min, nat);

	PRIV(self)->prefHforW = forWidth;
	PRIV(self)->prefHmin = *min;
	PRIV(self)->prefHnat = *nat;
	PRIV(self)->prefHvalid = true;
}

void cmk_widget_get_draw_rect(CmkWidget *self, CmkRect *rect)
{
	g_return_if_fail(rect);

	rect->x = 0;
	rect->y = 0;
	rect->width = 0;
	rect->height = 0;

	g_return_if_fail(CMK_IS_WIDGET(self));

	if(PRIV(self)->drawRectValid)
	{
		*rect = PRIV(self)->drawRect;
		return;
	}

	if(CMK_WIDGET_GET_CLASS(self)->get_draw_rect)
		CMK_WIDGET_GET_CLASS(self)->get_draw_rect(self, rect);

	PRIV(self)->drawRect = *rect;
}

void cmk_widget_set_size(CmkWidget *self, float width, float height)
{
	g_return_if_fail(CMK_IS_WIDGET(self));

	if(PRIV(self)->setWidth != width || PRIV(self)->setHeight != height)
		PRIV(self)->drawRectValid = false;

	PRIV(self)->setWidth = width;
	PRIV(self)->setHeight = height;
}

void cmk_widget_get_size(CmkWidget *self, float *width, float *height)
{
	*width = *height = 0;

	g_return_if_fail(CMK_IS_WIDGET(self));

	if(PRIV(self)->setWidth < 0)
		cmk_widget_get_preferred_width(self, -1, NULL, width);
	else
		*width = PRIV(self)->setWidth;

	if(PRIV(self)->setHeight < 0)
		cmk_widget_get_preferred_height(self, -1, NULL, height);
	else
		*height = PRIV(self)->setHeight;
}

void cmk_widget_set_disabled(CmkWidget *self, bool disabled)
{
	g_return_if_fail(CMK_IS_WIDGET(self));

	if(PRIV(self)->disabled != disabled)
	{
		PRIV(self)->disabled = disabled;

		if(CMK_WIDGET_GET_CLASS(self)->disable)
			CMK_WIDGET_GET_CLASS(self)->disable(self, disabled);
	}
}

bool cmk_widget_get_disabled(CmkWidget *self)
{
	g_return_val_if_fail(CMK_IS_WIDGET(self), false);
	return PRIV(self)->disabled;
}
