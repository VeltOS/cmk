/*
 * libcmk, Copyright 2018 Aidan Shafran <zelbrium@gmail.com>
 * Apache License 2.0 <www.apache.org/licenses/LICENSE-2.0>.
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "cmk-button.h"
#include "cmk-timeline.h"
#include "cmk-shadow.h"

#define CMK_BUTTON_EVENT_MASK (CMK_EVENT_BUTTON | CMK_EVENT_CROSSING | CMK_EVENT_KEY | CMK_EVENT_FOCUS)

#define ANIM_TIME 150

// Based on Material design spec
#define WIDTH_PADDING 16 // dp
#define HEIGHT_PADDING 9 // dp
#define BEVEL_RADIUS 4 // dp
#define SHADOW_RADIUS 6

typedef struct _CmkButtonPrivate CmkButtonPrivate;
struct _CmkButtonPrivate
{
	CmkButtonType type;
	CmkLabel *label;
	CmkShadow *shadow;

	CmkTimeline *hover, *up, *down;
	float clickX, clickY;
	bool press, enter, focus;

	const CmkColor *backgroundColor;
	const CmkColor *selectedColor;
	const CmkColor *hoverColor;

	float prevWidth, prevHeight;
};

enum
{
	PROP_TYPE = 1,
	PROP_LABEL,
	PROP_PANGO_CONTEXT,
	PROP_LAST,
	PROP_OVERRIDE_EVENT_MASK
};

enum
{
	SIGNAL_ACTIVATE = 1,
	SIGNAL_LAST
};

static GParamSpec *properties[PROP_LAST];
static guint signals[SIGNAL_LAST];

G_DEFINE_TYPE_WITH_PRIVATE(CmkButton, cmk_button, CMK_TYPE_WIDGET);
#define PRIV(button) ((CmkButtonPrivate *)cmk_button_get_instance_private(button))


static void on_dispose(GObject *self_);
static void set_property(GObject *self_, guint id, const GValue *value, GParamSpec *pspec);
static void get_property(GObject *self_, guint id, GValue *value, GParamSpec *pspec);
static void set_type(CmkButton *self, CmkButtonType type);
static void on_draw(CmkWidget *self_, cairo_t *cr);
static bool on_event(CmkWidget *self_, const CmkEvent *event);
static void get_preferred_width(CmkWidget *self_, float forHeight, float *min, float *nat);
static void get_preferred_height(CmkWidget *self_, float forWidth, float *min, float *nat);
static void get_draw_rect(CmkWidget *self_, CmkRect *rect);
static void on_palette_changed(CmkButton *self);
static void on_pango_context_changed(CmkButton *self);


CmkButton * cmk_button_new(CmkButtonType type)
{
	return CMK_BUTTON(g_object_new(CMK_TYPE_BUTTON,
		"type", type,
		NULL));
}

CmkButton * cmk_button_new_with_label(CmkButtonType type, const char *label)
{
	return CMK_BUTTON(g_object_new(CMK_TYPE_BUTTON,
		"type", type,
		"label", label,
		NULL));
}

static void cmk_button_class_init(CmkButtonClass *class)
{
	GObjectClass *base = G_OBJECT_CLASS(class);
	base->dispose = on_dispose;
	base->get_property = get_property;
	base->set_property = set_property;

	CmkWidgetClass *widgetClass = CMK_WIDGET_CLASS(class);
	widgetClass->draw = on_draw;
	widgetClass->event = on_event;
	widgetClass->get_preferred_width = get_preferred_width;
	widgetClass->get_preferred_height = get_preferred_height;
	widgetClass->get_draw_rect = get_draw_rect;

	/**
	 * CmkButton:type:
	 *
	 * The type of the button, from the #CmkButtonType enum.
	 */
	properties[PROP_TYPE] =
		g_param_spec_uint("type", "type", "type",
		                  0, CMK_BUTTON_ACTION, 0,
		                  G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

	/**
	 * CmkButton:label:
	 *
	 * The label text of the button.
	 */
	properties[PROP_LABEL] =
		g_param_spec_string("label", "label", "label text",
		                    NULL,
		                    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

	/**
	 * CmkLabel:pango-context:
	 *
	 * Sets the Pango context to use, or NULL to use
	 * a default context. This can be set by the
	 * widget wrapper class.
	 */
	properties[PROP_PANGO_CONTEXT] =
		g_param_spec_object("pango-context", "pango context", "pango context",
		                    PANGO_TYPE_CONTEXT,
		                    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

	g_object_class_install_properties(base, PROP_LAST, properties);

	g_object_class_override_property(base, PROP_OVERRIDE_EVENT_MASK, "event-mask");

	/**
	 * CmkButton::activate:
	 * @widget: The button that was activated.
	 *
	 * Emitted when the button has been activated. That is,
	 * the user has clicked it, or cmk_button_activate() has
	 * been called.
	 */
	signals[SIGNAL_ACTIVATE] =
		g_signal_new("activate",
		             G_TYPE_FROM_CLASS(class),
		             G_SIGNAL_RUN_FIRST,
		             0, NULL, NULL, NULL,
		             G_TYPE_NONE,
		             0);
}

void cmk_button_init(CmkButton *self)
{
	PRIV(self)->shadow = cmk_shadow_new(SHADOW_RADIUS);

	PRIV(self)->label = cmk_label_new_bold(NULL);
	cmk_label_set_single_line(PRIV(self)->label, true);
	cmk_label_set_alignment(PRIV(self)->label, PANGO_ALIGN_CENTER);

	PRIV(self)->hover = cmk_timeline_new(CMK_WIDGET(self), ANIM_TIME);
	PRIV(self)->up = cmk_timeline_new(CMK_WIDGET(self), ANIM_TIME);
	PRIV(self)->down = cmk_timeline_new(CMK_WIDGET(self), 300);

	cmk_timeline_set_action(PRIV(self)->hover, (CmkTimelineActionCallback)cmk_widget_invalidate, NULL);
	cmk_timeline_set_action(PRIV(self)->up, (CmkTimelineActionCallback)cmk_widget_invalidate, NULL);
	cmk_timeline_set_action(PRIV(self)->down, (CmkTimelineActionCallback)cmk_widget_invalidate, NULL);
	cmk_timeline_set_easing_mode(PRIV(self)->down, CMK_TIMELINE_SINE_OUT);

	g_signal_connect(self,
	                 "notify::palette",
	                 G_CALLBACK(on_palette_changed),
	                 NULL);
	g_signal_connect(self,
	                 "notify::pango-context",
	                 G_CALLBACK(on_pango_context_changed),
	                 NULL);
}

static void on_dispose(GObject *self_)
{
	G_OBJECT_CLASS(cmk_button_parent_class)->dispose(self_);
}

static void set_property(GObject *self_, guint id, const GValue *value, GParamSpec *pspec)
{
	CmkButton *self = CMK_BUTTON(self_);

	switch(id)
	{
	case PROP_TYPE:
		set_type(self, g_value_get_uint(value));
		break;
	case PROP_LABEL:
		cmk_button_set_label(self, g_value_get_string(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(self, id, pspec);
		break;
	}
}

static void get_property(GObject *self_, guint id, GValue *value, GParamSpec *pspec)
{
	CmkButton *self = CMK_BUTTON(self_);

	switch(id)
	{
	case PROP_TYPE:
		g_value_set_uint(value, PRIV(self)->type);
		break;
	case PROP_LABEL:
		g_value_set_string(value, cmk_button_get_label(self));
		break;
	case PROP_OVERRIDE_EVENT_MASK:
		g_value_set_uint(value, CMK_BUTTON_EVENT_MASK);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(self, id, pspec);
		break;
	}
}

static void set_type(CmkButton *self, CmkButtonType type)
{
	PRIV(self)->type = type;
	if(type == CMK_BUTTON_RAISED)
	{
		cmk_widget_set_palette(CMK_WIDGET(self), cmk_palette_get_primary(G_TYPE_FROM_INSTANCE(self)));
	}
	else if(type == CMK_BUTTON_ACTION)
	{
		cmk_widget_set_palette(CMK_WIDGET(self), cmk_palette_get_secondary(G_TYPE_FROM_INSTANCE(self)));
	}
	// otherwise, the base palette is default
}

static void on_draw(CmkWidget *self_, cairo_t *cr)
{
	CmkButtonPrivate *priv = PRIV(CMK_BUTTON(self_));

	float width, height;
	cmk_widget_get_size(self_, &width, &height);

	cairo_save(cr);
	
	// Shadow
	cmk_shadow_set_percent(priv->shadow,
		0.5 + 0.5 * cmk_timeline_get_progress(priv->hover));

	if(priv->type == CMK_BUTTON_RAISED)
	{
		cmk_shadow_set_rectangle(priv->shadow, width, height, false);
		cairo_translate(cr, 0, 1);
		cmk_shadow_draw(priv->shadow, cr);
		cairo_translate(cr, 0, -1);
	}

	// Clip
	if(priv->type == CMK_BUTTON_EMBED)
	{
		cairo_rectangle(cr, 0, 0, width, height);
		cairo_clip(cr);
	}
	else
	{
		double radius;
		static const double degrees = M_PI / 180.0;

		if(priv->type == CMK_BUTTON_FLAT || priv->type == CMK_BUTTON_RAISED)
			radius = BEVEL_RADIUS;
		else
			radius = MIN(width, height)/2;

		radius = MIN(MAX(radius, 0), MIN(width,height)/2);

		cairo_new_sub_path(cr);
		cairo_arc(cr, width - radius, radius, radius, -90 * degrees, 0 * degrees);
		cairo_arc(cr, width - radius, height - radius, radius, 0 * degrees, 90 * degrees);
		cairo_arc(cr, radius, height - radius, radius, 90 * degrees, 180 * degrees);
		cairo_arc(cr, radius, radius, radius, 180 * degrees, 270 * degrees);
		cairo_close_path(cr);

		if(priv->type == CMK_BUTTON_ACTION)
		{
			if(priv->prevWidth != width || priv->prevHeight != height)
			{
				priv->prevWidth = width;
				priv->prevHeight = height;
				// TODO: Use circle shadow eventually
				cmk_shadow_set_shape(priv->shadow, cairo_copy_path(cr), false);
			}
			cairo_translate(cr, 0, 1);
			cmk_shadow_draw(priv->shadow, cr);
			cairo_translate(cr, 0, -1);
		}

		cairo_clip(cr);
	}

	// Paint background
	cmk_cairo_set_source_color(cr, priv->backgroundColor);
	cairo_paint(cr);

	// Paint hover
	cmk_cairo_set_source_color(cr, priv->hoverColor);
	cairo_paint_with_alpha(cr, cmk_timeline_get_progress(priv->hover));

	// Paint click bubble
	float downProgress = cmk_timeline_get_progress(priv->down);
	if(downProgress > 0)
	{
		float upProgress = cmk_timeline_get_progress(priv->up);
		float x = priv->clickX;
		float y = priv->clickY;
		// Largest distance from any corner
		float r1 = x*x + y*y;
		float r2 = (width-x)*(width-x) + (height-y)*(height-y);
		float r3 = (width-x)*(width-x) + (y)*(y);
		float r4 = x*x + (height-y)*(height-y);
		float r = sqrt(MAX(MAX(MAX(r1, r2), r3), r4));
		
		cairo_save(cr);
		cairo_new_sub_path(cr);
		cairo_arc(cr, x, y, r * downProgress, 0, 2 * M_PI);
		cairo_close_path(cr);
		cairo_clip(cr);
		cmk_cairo_set_source_color(cr, priv->selectedColor);
		cairo_paint_with_alpha(cr, (1 - upProgress));
		cairo_restore(cr);
	}

	// Paint text
	float textWidth;
	cmk_widget_get_preferred_width(CMK_WIDGET(priv->label), -1, NULL, &textWidth);
	textWidth = MIN(textWidth, width);

	float textHeight;
	cmk_widget_get_preferred_height(CMK_WIDGET(priv->label), textWidth, NULL, &textHeight);

	cmk_widget_set_size(CMK_WIDGET(priv->label), textWidth, textHeight);

	cairo_translate(cr,
		width / 2 - textWidth / 2,
		height / 2 - textHeight / 2);

	cmk_widget_draw(CMK_WIDGET(priv->label), cr);

	cairo_restore(cr);
}

static bool on_event(CmkWidget *self_, const CmkEvent *event)
{
	CmkButtonPrivate *priv = PRIV(CMK_BUTTON(self_));

	if(event->type == CMK_EVENT_BUTTON)
	{
		CmkEventButton *button = (CmkEventButton *)event;
		priv->press = button->press;
		priv->clickX = button->x;
		priv->clickY = button->y;
		if(button->press)
		{
			cmk_timeline_goto(priv->down, 0);
			cmk_timeline_goto(priv->up, 0);
			cmk_timeline_start(priv->down);
			cmk_timeline_start(priv->hover);
		}
		else
		{
			cmk_timeline_start(priv->up);
			if(!(priv->enter || priv->focus))
				cmk_timeline_start_reverse(priv->hover);
		}
		return true;
	}
	else if(event->type == CMK_EVENT_CROSSING)
	{
		CmkEventCrossing *crossing = (CmkEventCrossing *)event;
		priv->enter = crossing->enter;
		if(!priv->press && !priv->focus)
		{
			if(crossing->enter)
				cmk_timeline_start(priv->hover);
			else
				cmk_timeline_start_reverse(priv->hover);
		}
		return true;
	}
	else if(event->type == CMK_EVENT_FOCUS)
	{
		CmkEventFocus *focus = (CmkEventFocus *)event;
		priv->focus = focus->in;
		if(!priv->press && !priv->enter)
		{
			if(focus->in)
				cmk_timeline_start(priv->hover);
			else
				cmk_timeline_start_reverse(priv->hover);
		}
	}
	else if(event->type == CMK_EVENT_KEY)
	{
		// TODO
	}
	return false;
}

static void get_preferred_width(CmkWidget *self_, UNUSED float forHeight, float *min, float *nat)
{
	cmk_widget_get_preferred_width(CMK_WIDGET(PRIV(CMK_BUTTON(self_))->label), -1, min, nat);
	*min += WIDTH_PADDING * 2;
	*nat += WIDTH_PADDING * 2;
}

static void get_preferred_height(CmkWidget *self_, UNUSED float forWidth, float *min, float *nat)
{
	cmk_widget_get_preferred_height(CMK_WIDGET(PRIV(CMK_BUTTON(self_))->label), -1, min, nat);
	*min += HEIGHT_PADDING * 2;
	*nat += HEIGHT_PADDING * 2;
}

static void get_draw_rect(CmkWidget *self_, CmkRect *rect)
{
	cmk_widget_get_size(self_, &rect->width, &rect->height);
	rect->x -= SHADOW_RADIUS;
	rect->y -= SHADOW_RADIUS - 1;
	rect->width += SHADOW_RADIUS * 2;
	rect->height += SHADOW_RADIUS * 2;
}

static void on_palette_changed(CmkButton *self)
{
	CmkButtonPrivate *priv = PRIV(self);
	CmkWidget *self_ = CMK_WIDGET(self);
	priv->backgroundColor = cmk_widget_get_color(self_, "background");
	priv->selectedColor = cmk_widget_get_color(self_, "selected");
	priv->hoverColor = cmk_widget_get_color(self_, "hover");
	cmk_widget_set_palette(CMK_WIDGET(PRIV(self)->label), cmk_widget_get_palette(CMK_WIDGET(self)));
}

static void on_pango_context_changed(CmkButton *self)
{
	cmk_widget_set_pango_context(CMK_WIDGET(PRIV(self)->label), cmk_widget_get_pango_context(CMK_WIDGET(self)));
}

void cmk_button_set_label(CmkButton *self, const char *label)
{
	g_return_if_fail(CMK_IS_BUTTON(self));
	cmk_label_set_text(PRIV(self)->label, label);
	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_LABEL]);
}

const char * cmk_button_get_label(CmkButton *self)
{
	g_return_val_if_fail(CMK_IS_BUTTON(self), NULL);
	return cmk_label_get_text(PRIV(self)->label);
}

CmkLabel * cmk_button_get_label_widget(CmkButton *self)
{
	g_return_val_if_fail(CMK_IS_BUTTON(self), NULL);
	return PRIV(self)->label;
}

void cmk_button_activate(CmkButton *self)
{
	g_return_if_fail(CMK_IS_BUTTON(self));
	g_signal_emit(self, signals[SIGNAL_ACTIVATE], 0);
}
