/*
 * This file is part of graphene-desktop, the desktop environment of VeltOS.
 * Copyright (C) 2016 Velt Technologies, Aidan Shafran <zelbrium@gmail.com>
 * Licensed under the Apache License 2 <www.apache.org/licenses/LICENSE-2.0>.
 *
 * Styling properties are inherited through the styles-changed singal.
 *
 * When a widget is attached to a style parent (in update_actual_style_parent)
 * either automatically from its real parent or manually by calling
 * cmk_widget_set_style_parent(), a signal listener is attached to the style
 * parent's style-changed signal.
 *
 * This signal triggers on_style_parent_property_changed(), which checks
 * if the changed properties would affect child widgets, and if so, re-emits
 * the signal on self.
 *
 * To make loading faster, properties are not propagated if a widget is
 * not attached to a stage. This is so that you can create a widget subclass
 * which has its own child widgets, and then attach it to the stage, without
 * those child widgets receiving styles-changed twice (once on attaching
 * to their parent, and again on their parent attaching to the stage).
 * Only when a widget is attached to the stage do its children start
 * propagating style events. See on_parent_changed() for more info on
 * checking for stage attachment.
 *
 * This means that styles_changed will only be called by changes to the
 * own widget's style before it's attached to the stage. Your widget
 * subclasses should always be prepared for the style to change and update
 * accordingly.
 */

#include "cmk-widget.h"

typedef struct _CmkWidgetPrivate CmkWidgetPrivate;
struct _CmkWidgetPrivate
{
	CmkWidget *styleParent; // Not ref'd
	CmkWidget *actualStyleParent; // styleParent, real parent, or NULL (not ref'd)
	
	GHashTable *colors;
	gchar *backgroundColorName;
	gboolean drawBackground;
	int disabled; // -1 unset, 0 no, 1 yes
	float dpScale, dpScaleCache;
	float paddingMultiplier, bevelRadiusMultiplier;
	float mLeft, mRight, mTop, mBottom; // Margin multiplier factors
	
	gboolean tabbable, justTabbed;

	gboolean disposed; // Various callbacks (ex clutter canvas draws) like to emit even after their actor has been disposed. This is checked to avoid runtime "CRITICAL" messages on disposed objects (and unnecessary processing)
	gboolean debug;
};

typedef struct
{
	// Only one of these will be set at a time
	ClutterColor *color;
	gchar *link;
} CmkColor;

enum
{
	PROP_STYLE_PARENT = 1,
	PROP_DRAW_BACKGROUND,
	PROP_DP_SCALE,
	PROP_PADDING_MULTIPLIER,
	PROP_BEVEL_RADIUS_MULTIPLIER,
	PROP_TABBABLE,
	PROP_DISABLED,
	PROP_LAST
};

enum
{
	SIGNAL_STYLES_CHANGED = 1,
	SIGNAL_KEY_FOCUS_CHANGED,
	SIGNAL_REPLACE,
	SIGNAL_BACK,
	SIGNAL_LAST
};

static GParamSpec *properties[PROP_LAST];
static guint signals[SIGNAL_LAST];

static void cmk_widget_dispose(GObject *self_);
static void cmk_widget_set_property(GObject *self_, guint propertyId, const GValue *value, GParamSpec *pspec);
static void cmk_widget_get_property(GObject *self_, guint propertyId, GValue *value, GParamSpec *pspec);
static void on_parent_changed(ClutterActor *self_, ClutterActor *prevParent);
static void update_style_properties(CmkWidget *self, guint flags);
static void on_styles_changed(CmkWidget *self, guint flags);
static void update_actual_style_parent(CmkWidget *self);
static void update_background_color(CmkWidget *self);
static void update_dp_cache(CmkWidget *self);
//static void on_key_focus_changed(CmkWidget *self, ClutterActor *newfocus);
//static void update_named_background_color(CmkWidget *self);
static gboolean on_key_pressed(ClutterActor *self, ClutterKeyEvent *event);
static void on_key_focus_out(ClutterActor *self_);
static gboolean on_button_press(ClutterActor *self_, ClutterButtonEvent *event);
static void cmk_color_free(CmkColor *color);
#define dmsg(self, fmt...) if(PRIVATE((CmkWidget *)(self))->debug){g_message("CmkWidget: " fmt);}

G_DEFINE_TYPE_WITH_PRIVATE(CmkWidget, cmk_widget, CLUTTER_TYPE_ACTOR);
#define PRIVATE(widget) ((CmkWidgetPrivate *)cmk_widget_get_instance_private(widget))

CmkWidget * cmk_widget_new(void)
{
	return CMK_WIDGET(g_object_new(CMK_TYPE_WIDGET, NULL));
}

static void cmk_widget_class_init(CmkWidgetClass *class)
{
	GObjectClass *base = G_OBJECT_CLASS(class);
	base->dispose = cmk_widget_dispose;
	base->get_property = cmk_widget_get_property;
	base->set_property = cmk_widget_set_property;
	
	CLUTTER_ACTOR_CLASS(class)->key_press_event = on_key_pressed;
	CLUTTER_ACTOR_CLASS(class)->button_press_event = on_button_press;
	
	class->styles_changed = on_styles_changed;
	
	/**
	 * CmkWidget:style-parent:
	 *
	 * The current widget used for inheriting styles.
	 *
	 * Getting this property returns only the manually set value; if the
	 * active style parent is automatically determined, this gets NULL.
	 * Use cmk_widget_get_style_parent() to get the inherited value.
	 */
	properties[PROP_STYLE_PARENT] =
		g_param_spec_object("style-parent",
		                    "Style Parent",
		                    "The current widget used for inheriting styles",
		                    CMK_TYPE_WIDGET,
		                    G_PARAM_READWRITE);

	/**
	 * CmkWidget:draw-background:
	 *
	 * Whether or not #CmkWidget should automatically draw the background
	 * color set with cmk_widget_set_background_color(). If this is FALSE, the
	 * color should be drawn by someone else (namely a subclass of #CmkWidget)
	 */
	properties[PROP_DRAW_BACKGROUND] =
		g_param_spec_boolean("draw-background",
		                     "Draw Background",
		                     "Whether or not to automatically draw the background color set in background-color-name",
		                     FALSE,
		                     G_PARAM_READWRITE);

	/**
	 * CmkWidget:dp-scale:
	 *
	 * Number of pixels per 1 dp.
	 *
	 * Getting this value returns only the manually set value on this widget;
	 * that is, if the inherited dp scale is 2, but the value is only 1 on
	 * this widget, this returns 1. Use cmk_widget_get_dp_scale() to get the
	 * inherited value.
	 */
	properties[PROP_DP_SCALE] =
		g_param_spec_float("dp-scale",
		                   "dp Scale",
		                   "Number of pixels per 1 dp",
		                   0,
		                   G_MAXFLOAT,
		                   1,
		                   G_PARAM_READWRITE);

	/**
	 * CmkWidget:padding-multiplier:
	 *
	 * Scale to apply to the widget's padding, if applicable.
	 *
	 * Getting this value returns only the manually set value on this widget;
	 * if the padding multiplier is inherited, this returns -1. Use
	 * cmk_widget_get_padding_multiplier() to get the inherited value.
	 */
	properties[PROP_PADDING_MULTIPLIER] =
		g_param_spec_float("padding-multiplier",
		                   "Padding Multiplier",
		                   "Scale to apply to the widget's padding, if applicable",
		                   -1,
		                   G_MAXFLOAT,
		                   1,
		                   G_PARAM_READWRITE);

	/**
	 * CmkWidget:bevel-radius-multiplier:
	 *
	 * Scale to apply to the widget's bevel radius, if applicable.
	 *
	 * Getting this value returns only the manually set value on this widget;
	 * if the bevel radius is inherited, this returns -1. Use
	 * cmk_widget_get_bevel_radius_multiplier() to get the inherited value.
	 */
	properties[PROP_BEVEL_RADIUS_MULTIPLIER] =
		g_param_spec_float("bevel-radius-multiplier",
		                   "Bevel Radius Multiplier",
		                   "Scale to apply to the widget's bevel radius, if applicable",
		                   -1,
		                   G_MAXFLOAT,
		                   1,
		                   G_PARAM_READWRITE);

	properties[PROP_TABBABLE] =
		g_param_spec_boolean("tabbable",
		                     "Tabbable",
		                     "TRUE if this widget can be tabbed to",
		                     FALSE,
		                     G_PARAM_READWRITE);
	
	properties[PROP_DISABLED] =
		g_param_spec_boolean("disabled",
		                     "Disabled",
		                     "TRUE if this widget is disabled",
		                     FALSE,
		                     G_PARAM_READWRITE);

	g_object_class_install_properties(base, PROP_LAST, properties);

	/**
	 * CmkWidget::styles-changed:
	 * @self: The widget on which styles changed
	 * @flags: A set of #CmkStyleFlag indicating which style(s)
	 * have changed. 
	 *
	 * This signal is emitted when style properties are updated on this
	 * widget or any parent widgets. CmkWidget subclasses should use this
	 * as an indicator to invalidate ClutterCanvases or update colors.
	 * When overriding this signal's class handler, chain up to the
	 * parent class BEFORE updating your widget.
	 * 
	 * Changes to the dp and padding properties automatically queue
	 * relayouts, which in turn queue redraws, so you may not need to
	 * manually update anything when those properties change.
	 */
	signals[SIGNAL_STYLES_CHANGED] =
		g_signal_new("styles-changed",
		             G_TYPE_FROM_CLASS(class),
		             G_SIGNAL_RUN_FIRST,
		             G_STRUCT_OFFSET(CmkWidgetClass, styles_changed),
		             NULL, NULL, NULL,
		             G_TYPE_NONE,
		             1, G_TYPE_UINT);

	/**
	 * CmkWidget::key_focus_changed:
	 * @newfocus: The widget which just gained keyboard focus
	 *
	 * Emitted when a #CmkWidget in the actor heirarchy below this widget
	 * gains key focus.
	 */
	//signals[SIGNAL_KEY_FOCUS_CHANGED] =
	//	g_signal_new("key-focus-changed",
	//	             G_TYPE_FROM_CLASS(class),
	//	             G_SIGNAL_RUN_FIRST,
	//	             G_STRUCT_OFFSET(CmkWidgetClass, key_focus_changed),
	//	             NULL, NULL, NULL,
	//	             G_TYPE_NONE,
	//	             1, CLUTTER_TYPE_ACTOR);

	/**
	 * CmkWidget::replace:
	 * @self: Widget being replaced
	 * @replacement: The widget replacing @self
	 *
	 * Emitted by widget subclasses when the parent widget should replace
	 * them with a new widget. It is up to the parent widget to honor this.
	 * (This can also be used as a general 'forward' signal, with replacement
	 * being %NULL and the parent having a planned list of screens.)
	 *
	 * Use cmk_widget_replace() to conveniently emit this signal.
	 */
	signals[SIGNAL_REPLACE] =
		g_signal_new("replace",
		             G_TYPE_FROM_CLASS(class),
		             G_SIGNAL_RUN_FIRST,
		             G_STRUCT_OFFSET(CmkWidgetClass, replace),
		             NULL, NULL, NULL,
		             G_TYPE_NONE,
		             1, CMK_TYPE_WIDGET);

	/**
	 * CmkWidget::back:
	 * @self: Widget being replaced
	 *
	 * Emitted by widget subclasses when the parent should remove this widget
	 * and return to the previously shown widget (opposite of 'replace').
	 * It is up to the parent widget to honor this.
	 *
	 * Use cmk_widget_back() to conveniently emit this signal.
	 */
	signals[SIGNAL_BACK] =
		g_signal_new("back",
		             G_TYPE_FROM_CLASS(class),
		             G_SIGNAL_RUN_FIRST,
		             G_STRUCT_OFFSET(CmkWidgetClass, back),
		             NULL, NULL, NULL,
		             G_TYPE_NONE,
		             0);
}

static void cmk_widget_init(CmkWidget *self)
{
	CmkWidgetPrivate *private = PRIVATE(self);

	private->colors = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)cmk_color_free);
	private->dpScale = private->dpScaleCache = 1;
	private->paddingMultiplier = -1;
	private->bevelRadiusMultiplier = -1;
	private->disabled = -1;
	private->mLeft = private->mRight = private->mTop = private->mBottom = -1;

	// key-focus-out and parent-set are Run Last, so can use signal
	// connect to make sure this happens before subclasse's class
	// handlers are run.
	g_signal_connect(self, "key-focus-out", G_CALLBACK(on_key_focus_out), NULL);
	g_signal_connect(self, "parent-set", G_CALLBACK(on_parent_changed), NULL);
}

static void cmk_widget_dispose(GObject *self_)
{
	CmkWidgetPrivate *private = PRIVATE(CMK_WIDGET(self_));
	private->styleParent = NULL;
	g_clear_pointer(&private->colors, g_hash_table_unref);
	g_clear_pointer(&private->backgroundColorName, g_free);
	private->disposed = TRUE;
	G_OBJECT_CLASS(cmk_widget_parent_class)->dispose(self_);
}

static void cmk_widget_set_property(GObject *self_, guint propertyId, const GValue *value, GParamSpec *pspec)
{
	g_return_if_fail(CMK_IS_WIDGET(self_));
	CmkWidget *self = CMK_WIDGET(self_);
	
	switch(propertyId)
	{
	case PROP_STYLE_PARENT:
		cmk_widget_set_style_parent(self, g_value_get_object(value));
		break;
	case PROP_DRAW_BACKGROUND:
		cmk_widget_set_draw_background_color(self, g_value_get_boolean(value));
		break;
	case PROP_DP_SCALE:
		cmk_widget_set_dp_scale(self, g_value_get_float(value));
		break;
	case PROP_PADDING_MULTIPLIER:
		cmk_widget_set_padding_multiplier(self, g_value_get_float(value));
		break;
	case PROP_BEVEL_RADIUS_MULTIPLIER:
		cmk_widget_set_bevel_radius_multiplier(self, g_value_get_float(value));
		break;
	case PROP_TABBABLE:
		cmk_widget_set_tabbable(self, g_value_get_boolean(value));
		break;
	case PROP_DISABLED:
		cmk_widget_set_disabled(self, g_value_get_boolean(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(self, propertyId, pspec);
		break;
	}
}

static void cmk_widget_get_property(GObject *self_, guint propertyId, GValue *value, GParamSpec *pspec)
{
	g_return_if_fail(CMK_IS_WIDGET(self_));
	CmkWidget *self = CMK_WIDGET(self_);
	
	switch(propertyId)
	{
	case PROP_STYLE_PARENT:
		g_value_set_object(value, PRIVATE(self)->styleParent);
		break;
	case PROP_DRAW_BACKGROUND:
		g_value_set_boolean(value, PRIVATE(self)->drawBackground);
		break;
	case PROP_DP_SCALE:
		g_value_set_float(value, PRIVATE(self)->dpScale);
		break;
	case PROP_PADDING_MULTIPLIER:
		g_value_set_float(value, PRIVATE(self)->paddingMultiplier);
		break;
	case PROP_BEVEL_RADIUS_MULTIPLIER:
		g_value_set_float(value, PRIVATE(self)->bevelRadiusMultiplier);
		break;
	case PROP_TABBABLE:
		g_value_set_boolean(value, PRIVATE(self)->tabbable);
		break;
	case PROP_DISABLED:
		g_value_set_boolean(value, PRIVATE(self)->disabled);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(self, propertyId, pspec);
		break;
	}
}


gboolean cmk_widget_destroy(CmkWidget *widget)
{
	clutter_actor_destroy(CLUTTER_ACTOR(widget));
	return G_SOURCE_REMOVE;
}


static void on_style_parent_destroy(CmkWidget *self, CmkWidget *styleParent)
{
	if(PRIVATE(self)->styleParent == styleParent)
		PRIVATE(self)->styleParent = NULL;
	update_actual_style_parent(self);
}

static gboolean using_custom_style_parent(CmkWidgetPrivate *private)
{
	if(private->styleParent)
		return TRUE;
	else if(private->actualStyleParent)
		return using_custom_style_parent(PRIVATE(private->actualStyleParent));
	else
		return FALSE;
}

static void on_style_parent_property_changed(CmkWidget *self, guint flags)
{
	CmkWidgetPrivate *private = PRIVATE(self);
	if(private->disposed)
		return;
	
	// Don't bother updating if we're not attached to a stage yet
	if(!clutter_actor_get_stage(CLUTTER_ACTOR(self))
	// Except in the case that we have a custom style parent, because
	// that might never be attached to the stage.
	&& !using_custom_style_parent(private))
	{
		dmsg(self, "%p: not propagating because no stage", self);
		return;
	}
	
	// When an inheritable property changes on the parent, only relay
	// it to this widget if this widget isn't setting its own value for
	// the property.
	// dp and colors can't be overridden like the others, so don't check
	if(flags & CMK_STYLE_FLAG_PADDING_MUL)
	{
		if(private->paddingMultiplier >= 0)
			flags &= ~CMK_STYLE_FLAG_PADDING_MUL;
	}
	if(flags & CMK_STYLE_FLAG_BEVEL_MUL)
	{
		if(private->bevelRadiusMultiplier >= 0)
			flags &= ~CMK_STYLE_FLAG_BEVEL_MUL;
	}
	if(flags & CMK_STYLE_FLAG_DISABLED)
	{
		if(private->disabled >= 0)
			flags &= ~CMK_STYLE_FLAG_DISABLED;
	}
	
	dmsg(self, "%p: propagating style changes from parent (%i)", self, flags);
	if(flags)
		update_style_properties(self, flags);
}

static void on_parent_changed(ClutterActor *self_, ClutterActor *prevParent)
{
	dmsg(self_, "%p: parent changed from %p", self_, prevParent);
	g_return_if_fail(CMK_IS_WIDGET(self_));
	if((CmkWidget *)prevParent == PRIVATE(CMK_WIDGET(self_))->actualStyleParent)
		update_actual_style_parent(CMK_WIDGET(self_));
}

static void update_actual_style_parent(CmkWidget *self)
{
	dmsg(self, "%p: maybe updating style parent", self);
	CmkWidgetPrivate *private = PRIVATE(self);
	if(G_UNLIKELY(private->disposed))
		return;
	
	ClutterActor *parent = clutter_actor_get_parent(CLUTTER_ACTOR(self));

	CmkWidget *newStyleParent = NULL;
	if(private->styleParent)
		newStyleParent = private->styleParent;
	else if(CMK_IS_WIDGET(parent))
		newStyleParent = CMK_WIDGET(parent);
	
	if(newStyleParent == private->actualStyleParent)
		return;
	dmsg(self, "%p: updating style parent to %p", self, newStyleParent);
	
	if(private->actualStyleParent)
	{
		g_signal_handlers_disconnect_by_func(private->actualStyleParent, G_CALLBACK(on_style_parent_destroy), self);
		g_signal_handlers_disconnect_by_func(private->actualStyleParent, G_CALLBACK(on_style_parent_property_changed), self);
	}

	private->actualStyleParent = newStyleParent;
	
	if(newStyleParent)
	{
		g_signal_connect_swapped(private->actualStyleParent, "destroy", G_CALLBACK(on_style_parent_destroy), self);
		g_signal_connect_swapped(private->actualStyleParent, "styles-changed", G_CALLBACK(on_style_parent_property_changed), self);
	}

	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_STYLE_PARENT]);
	
	on_style_parent_property_changed(self, CMK_STYLE_FLAG_ALL);
	return;
}

void cmk_widget_set_style_parent(CmkWidget *self, CmkWidget *parent)
{
	g_return_if_fail(CMK_IS_WIDGET(self));
	// parent can be NULL
	g_return_if_fail(parent == NULL || CMK_IS_WIDGET(parent));
	
	dmsg(self, "%p: setting style parent to %p", self, parent);
	if(PRIVATE(self)->styleParent == parent)
		return;
	PRIVATE(self)->styleParent = parent;
	update_actual_style_parent(self);
}

CmkWidget * cmk_widget_get_style_parent(CmkWidget *self)
{
	g_return_val_if_fail(CMK_IS_WIDGET(self), NULL);
	return PRIVATE(self)->actualStyleParent;
}

static void update_style_properties(CmkWidget *self, guint flags)
{
	g_signal_emit(self, signals[SIGNAL_STYLES_CHANGED], 0, flags);
}

/*
 * Class handler for the styles-changed signal. Update properties
 * and redraw/relayout if necessary.
 */
static void on_styles_changed(CmkWidget *self, guint flags)
{
	dmsg(self, "%p: on_styles_changed (%i)", self, flags);
	CmkWidgetPrivate *private = PRIVATE(self);
	if(flags & CMK_STYLE_FLAG_COLORS)
	{
		update_background_color(self);
	}
	if(flags & CMK_STYLE_FLAG_DP)
	{
		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));
		update_dp_cache(self);
		cmk_widget_set_margin(self, private->mLeft, private->mRight, private->mTop, private->mBottom);
	}
	if(flags & CMK_STYLE_FLAG_PADDING_MUL)
	{
		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));
	}
	if(flags & CMK_STYLE_FLAG_DISABLED)
	{
		clutter_actor_queue_redraw(CLUTTER_ACTOR(self));
	}
}


void cmk_widget_set_named_color(CmkWidget *self, const gchar *name, const ClutterColor *color)
{
	g_return_if_fail(CMK_IS_WIDGET(self));
	if(!color)
	{
		if(!g_hash_table_remove(PRIVATE(self)->colors, name))
			return;
	}
	else
	{
		CmkColor *c = g_new0(CmkColor, 1);
		c->color = clutter_color_copy(color);
		g_hash_table_insert(PRIVATE(self)->colors, g_strdup(name), c);
	}
	// Updating the COLORS property will call update_background_color 
	update_style_properties(self, CMK_STYLE_FLAG_COLORS);
}

void cmk_widget_set_named_color_link(CmkWidget *self, const gchar *name, const gchar *link)
{
	g_return_if_fail(CMK_IS_WIDGET(self));
	if(!link)
	{
		if(!g_hash_table_remove(PRIVATE(self)->colors, name))
			return;
	}
	else
	{
		CmkColor *c = g_new0(CmkColor, 1);
		c->link = g_strdup(link);
		g_hash_table_insert(PRIVATE(self)->colors, g_strdup(name), c);
	}
	// Updating the COLORS property will call update_background_color 
	update_style_properties(self, CMK_STYLE_FLAG_COLORS);
}

void cmk_widget_set_named_colors(CmkWidget *self, const CmkNamedColor *colors)
{
	g_return_if_fail(CMK_IS_WIDGET(self));
	for(guint i=0; colors[i].name != NULL; ++i)
	{
		CmkColor *c = g_new0(CmkColor, 1);
		c->color = clutter_color_copy(&colors[i].color);
		g_hash_table_insert(PRIVATE(self)->colors, g_strdup(colors[i].name), c);
	}
	update_style_properties(self, CMK_STYLE_FLAG_COLORS);
}

static const ClutterColor * cmk_widget_get_named_color_internal(CmkWidgetPrivate *private, const gchar *name, const gchar *original)
{
	if(G_UNLIKELY(!name || private->disposed))
		return NULL;
	const CmkColor *ret = NULL;
	if(g_hash_table_size(private->colors) > 0)
		ret = g_hash_table_lookup(private->colors, name);
	if(ret)
	{
		if(ret->color)
			return ret->color;
		else if(ret->link && g_strcmp0(ret->link, original) != 0) // Prevent some cycles
			return cmk_widget_get_named_color_internal(private, ret->link, original);
	}
	if(private->actualStyleParent)
		return cmk_widget_get_named_color(private->actualStyleParent, name);
	return NULL;
}

const ClutterColor * cmk_widget_get_named_color(CmkWidget *self, const gchar *name)
{
	g_return_val_if_fail(CMK_IS_WIDGET(self), NULL);
	return cmk_widget_get_named_color_internal(PRIVATE(self), name, name);
}

const ClutterColor * cmk_widget_get_default_named_color(CmkWidget *self, const gchar *name)
{
	const ClutterColor *color = clutter_color_get_static(CLUTTER_COLOR_BLACK);
	g_return_val_if_fail(CMK_IS_WIDGET(self), color);

	color = cmk_widget_get_named_color(self, name);
	if(color)
		return color;
	
	if(g_str_has_suffix(name, "background"))
		return clutter_color_get_static(CLUTTER_COLOR_WHITE);
	if(g_str_has_suffix(name, "foreground"))
		return clutter_color_get_static(CLUTTER_COLOR_BLACK);
	return clutter_color_get_static(CLUTTER_COLOR_GRAY);
}

static void update_background_color(CmkWidget *self)
{
	if(!PRIVATE(self)->drawBackground)
		return;
	const ClutterColor *color = cmk_widget_get_named_color(self, "background");
	clutter_actor_set_background_color(CLUTTER_ACTOR(self), color);
}

/*
 * Changing draw background color shouldn't actually effect colors; when
 * this is false, the widget is expected to draw its background color
 * manually. So changing this doesn't result in a style update emission.
 */
void cmk_widget_set_draw_background_color(CmkWidget *self, gboolean draw)
{
	g_return_if_fail(CMK_IS_WIDGET(self));
	if(PRIVATE(self)->drawBackground == draw)
		return;

	PRIVATE(self)->drawBackground = draw;
	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_DRAW_BACKGROUND]);

	// If changing from drawing background to not, clear the background color
	if(draw)
		update_background_color(self);
	else
		clutter_actor_set_background_color(CLUTTER_ACTOR(self), NULL);
}

void cmk_widget_set_dp_scale(CmkWidget *self, float dp)
{
	//g_return_if_fail(CMK_IS_WIDGET(self));
	g_return_if_fail(dp >= 0);
	CmkWidgetPrivate *private = PRIVATE(self);
	if(dp < 0) dp = 0;
	if(private->dpScale != dp)
	{
		private->dpScale = dp;
		g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_DP_SCALE]);
		update_style_properties(self, CMK_STYLE_FLAG_DP);
	}
}

// dp can be requested a lot during layout, so cache
// it to make lookups slightly faster
static void update_dp_cache(CmkWidget *self)
{
	CmkWidgetPrivate *private = PRIVATE(self);
	if(G_UNLIKELY(private->disposed))
		private->dpScaleCache = 1;
	if(private->actualStyleParent)
		private->dpScaleCache = cmk_widget_get_dp_scale(private->actualStyleParent) * private->dpScale;
	else
		private->dpScaleCache = private->dpScale;
}

float cmk_widget_get_dp_scale(CmkWidget *self)
{
	g_return_val_if_fail(CMK_IS_WIDGET(self), 1);
	CmkWidgetPrivate *private = PRIVATE(self);
	return private->dpScaleCache;
}

void cmk_widget_set_bevel_radius_multiplier(CmkWidget *self, float multiplier)
{
	g_return_if_fail(CMK_IS_WIDGET(self));
	CmkWidgetPrivate *private = PRIVATE(self);
	if(private->bevelRadiusMultiplier == multiplier)
		return;
	private->bevelRadiusMultiplier = multiplier;
	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_BEVEL_RADIUS_MULTIPLIER]);
	update_style_properties(self, CMK_STYLE_FLAG_BEVEL_MUL);
}

float cmk_widget_get_bevel_radius_multiplier(CmkWidget *self)
{
	g_return_val_if_fail(CMK_IS_WIDGET(self), 1);
	CmkWidgetPrivate *private = PRIVATE(self);
	if(G_UNLIKELY(private->disposed))
		return 1;
	else if(private->bevelRadiusMultiplier >= 0)
		return private->bevelRadiusMultiplier;
	else if(private->actualStyleParent)
		return cmk_widget_get_bevel_radius_multiplier(private->actualStyleParent);
	return 1;
}

void cmk_widget_set_padding_multiplier(CmkWidget *self, float multiplier)
{
	g_return_if_fail(CMK_IS_WIDGET(self));
	CmkWidgetPrivate *private = PRIVATE(self);
	if(multiplier == private->paddingMultiplier)
		return;
	private->paddingMultiplier = multiplier;
	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_PADDING_MULTIPLIER]);
	update_style_properties(self, CMK_STYLE_FLAG_PADDING_MUL);
}

float cmk_widget_get_padding_multiplier(CmkWidget *self)
{
	g_return_val_if_fail(CMK_IS_WIDGET(self), 1);
	CmkWidgetPrivate *private = PRIVATE(self);
	if(G_UNLIKELY(private->disposed))
		return 1;
	else if(private->paddingMultiplier >= 0)
		return private->paddingMultiplier;
	else if(private->actualStyleParent)
		return cmk_widget_get_padding_multiplier(private->actualStyleParent);
	else
		return 1;
}

void cmk_widget_set_margin(CmkWidget *self, float left, gfloat right, gfloat top, gfloat bottom)
{
	g_return_if_fail(CMK_IS_WIDGET(self));
	
	CmkWidgetPrivate *private = PRIVATE(self);
	private->mLeft = left;
	private->mRight = right;
	private->mTop = top;
	private->mBottom = bottom;
	
	if(left < 0 || right < 0 || top < 0 || bottom < 0)
		return;
	
	float dp = cmk_widget_get_dp_scale(self);
	ClutterMargin margin = {
		left * dp,
		right * dp,
		top * dp,
		bottom * dp,
	};
	clutter_actor_set_margin(CLUTTER_ACTOR(self), &margin);
}

void cmk_widget_set_disabled(CmkWidget *self, int disabled)
{
	g_return_if_fail(CMK_IS_WIDGET(self));
	if(PRIVATE(self)->disabled == disabled)
		return;
	PRIVATE(self)->disabled = disabled;
	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_DISABLED]);
	update_style_properties(self, CMK_STYLE_FLAG_DISABLED);
}

gboolean cmk_widget_get_disabled(CmkWidget *self)
{
	g_return_val_if_fail(CMK_IS_WIDGET(self), FALSE);
	CmkWidgetPrivate *private = PRIVATE(self);
	if(G_UNLIKELY(private->disposed))
		return FALSE;
	else if(private->disabled >= 0)
		return private->disabled;
	else if(private->actualStyleParent)
		return cmk_widget_get_disabled(private->actualStyleParent);
	else
		return FALSE;
}

void cmk_widget_replace(CmkWidget *self, CmkWidget *replacement)
{
	g_signal_emit(self, signals[SIGNAL_REPLACE], 0, replacement);
}

void cmk_widget_back(CmkWidget *self)
{
	g_signal_emit(self, signals[SIGNAL_BACK], 0);
}

void cmk_widget_add_child(CmkWidget *self, CmkWidget *child)
{
	clutter_actor_add_child(CLUTTER_ACTOR(self), CLUTTER_ACTOR(child));
}

void cmk_widget_bind_fill(CmkWidget *self)
{
	ClutterActor *self_ = CLUTTER_ACTOR(self);
	ClutterActor *parent = clutter_actor_get_parent(self_);
	clutter_actor_add_constraint_with_name(self_, "cmk-widget-bind-fill", clutter_bind_constraint_new(parent, CLUTTER_BIND_ALL, 0));
}

static void fade_out_complete(ClutterActor *actor, gboolean destroy)
{
	g_signal_handlers_disconnect_by_func(actor, fade_out_complete, (gpointer)(gsize)destroy);
	if(destroy)
		clutter_actor_destroy(actor);
	else
		clutter_actor_hide(actor);
}

void cmk_widget_fade_out(CmkWidget *self, gboolean destroy)
{
	ClutterActor *self_ = CLUTTER_ACTOR(self);
	g_signal_connect(self_, "transitions_completed", G_CALLBACK(fade_out_complete), (gpointer)(gsize)destroy);
	clutter_actor_save_easing_state(self_);
	clutter_actor_set_easing_mode(self_, CLUTTER_EASE_OUT_SINE);
	clutter_actor_set_easing_duration(self_, 200);
	clutter_actor_set_opacity(self_, 0);
	clutter_actor_restore_easing_state(self_);
	// TRANSITION_MEMLEAK_FIX(self_, "opacity");
}

void cmk_widget_fade_in(CmkWidget *self)
{
	ClutterActor *self_ = CLUTTER_ACTOR(self);
	clutter_actor_set_opacity(self_, 0);
	clutter_actor_show(self_);
	clutter_actor_save_easing_state(self_);
	clutter_actor_set_easing_mode(self_, CLUTTER_EASE_IN_SINE);
	clutter_actor_set_easing_duration(self_, 200);
	clutter_actor_set_opacity(self_, 255);
	clutter_actor_restore_easing_state(self_);
}

void cmk_widget_set_tabbable(CmkWidget *self, gboolean tabbable)
{
	g_return_if_fail(CMK_IS_WIDGET(self));
	CmkWidgetPrivate *private = PRIVATE(self);
	if(private->tabbable == tabbable)
		return;
	private->tabbable = tabbable;
	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_TABBABLE]);
}

gboolean cmk_widget_get_tabbable(CmkWidget *self)
{
	g_return_val_if_fail(CMK_IS_WIDGET(self), FALSE);
	return PRIVATE(self)->tabbable;
}

gboolean cmk_widget_get_just_tabbed(CmkWidget *self)
{
	g_return_val_if_fail(CMK_IS_WIDGET(self), FALSE);
	return PRIVATE(self)->justTabbed;
}

// Foucs stack globals
static GList *focusStack = NULL;
static guint focusStackTopMappedSignalId = 0;
static CmkGrabHandler grabHandler = NULL;
static gpointer grabHandlerData = NULL;

/*
 * get_next_tabstop finds the next CmkWidget in the Clutter scene
 * graph which is tabbable. Given a starting actor, it does a depth-
 * first-search on all of its child actors (i.e. excluding itself),
 * and if that fails, begins to do an "inverse" depth-first search,
 * recursively running depth-first searches on siblings and parent
 * nodes. Returns NULL if there are no more tabstops in the graph.
 */
static ClutterActor * get_next_tabstop_dfs(ClutterActor *start)
{
	if(!clutter_actor_is_mapped(start))
		return NULL;
	
	if(CMK_IS_WIDGET(start)
	&& PRIVATE(CMK_WIDGET(start))->tabbable)
		return start;
	
	ClutterActor *next = clutter_actor_get_first_child(start);
	ClutterActor *stop;
	while(next)
	{
		if((stop = get_next_tabstop_dfs(next)))
			return stop;
		next = clutter_actor_get_next_sibling(next);
	}
	return NULL;
}

static ClutterActor * get_next_tabstop(ClutterActor *start)
{
	ClutterActor *stop, *next;
	
	// Search the children of @start
	next = clutter_actor_get_first_child(start);
	while(next)
	{
		if((stop = get_next_tabstop_dfs(next)))
			return stop;
		next = clutter_actor_get_next_sibling(next);
	}
	
	ClutterActor *layer = start;
	while(layer)
	{
		// Make sure the new layer is in the focus stack
		if(focusStack
		&& (layer == (ClutterActor *)focusStack->data || !clutter_actor_contains(focusStack->data, layer)))
			break;
		
		// Search the siblings of @start
		next = layer;
		while((next = clutter_actor_get_next_sibling(next)))
		{
			if((stop = get_next_tabstop_dfs(next)))
				return stop;
		}
		
		// Go up a level and try again
		layer = clutter_actor_get_parent(layer);
	}
	
	return NULL;
}

static ClutterActor * get_next_tabstop_dfs_rev(ClutterActor *start)
{
	if(!clutter_actor_is_mapped(start))
		return NULL;
	
	if(CMK_IS_WIDGET(start)
	&& PRIVATE(CMK_WIDGET(start))->tabbable)
		return start;
	
	ClutterActor *next = clutter_actor_get_last_child(start);
	ClutterActor *stop;
	while(next)
	{
		if((stop = get_next_tabstop_dfs_rev(next)))
			return stop;
		next = clutter_actor_get_previous_sibling(next);
	}
	return NULL;
}

static ClutterActor * get_next_tabstop_rev(ClutterActor *start)
{
	ClutterActor *stop, *next;
	
	// Search the children of @start
	next = clutter_actor_get_last_child(start);
	while(next)
	{
		if((stop = get_next_tabstop_dfs_rev(next)))
			return stop;
		next = clutter_actor_get_previous_sibling(next);
	}
	
	ClutterActor *layer = start;
	while(layer)
	{
		// Make sure the new layer is in the focus stack
		if(focusStack
		&& (layer == (ClutterActor *)focusStack->data || !clutter_actor_contains(focusStack->data, layer)))
			break;
		
		// Search the siblings of @start
		next = layer;
		while((next = clutter_actor_get_previous_sibling(next)))
		{
			if((stop = get_next_tabstop_dfs_rev(next)))
				return stop;
		}
		
		// Go up a level and try again
		layer = clutter_actor_get_parent(layer);
	}
	
	return NULL;
}

static ClutterActor * get_next_tabstop_full(ClutterActor *start, gboolean rev)
{
	ClutterActor * (*next_tabstop)() = rev ? get_next_tabstop_rev : get_next_tabstop;
	
	ClutterActor *a = next_tabstop(start);
	if(!a)
	{
		if(focusStack)
		{
			a = focusStack->data;
			if(!cmk_widget_get_tabbable(CMK_WIDGET(a)))
				a = next_tabstop(a);
		}
		else
			a = next_tabstop(clutter_actor_get_stage(start));
	}
	return a;
}

static gboolean on_key_pressed(ClutterActor *self, ClutterKeyEvent *event)
{
	if(event->keyval == CLUTTER_KEY_Tab || event->keyval == CLUTTER_KEY_ISO_Left_Tab)
	{
		ClutterActor *a = get_next_tabstop_full(
			event->source,
			(event->modifier_state & CLUTTER_SHIFT_MASK));
		if(CMK_IS_WIDGET(a))
			PRIVATE(CMK_WIDGET(a))->justTabbed = TRUE;
		clutter_actor_grab_key_focus(a);
		return CLUTTER_EVENT_STOP;
	}
	return CLUTTER_EVENT_PROPAGATE;
}

static void on_key_focus_out(ClutterActor *self_)
{
	PRIVATE(CMK_WIDGET(self_))->justTabbed = FALSE;
}

static gboolean on_button_press(ClutterActor *self_, ClutterButtonEvent *event)
{
	// The default action for a reactive actor is to just grab
	// keyboard focus. Some widgets, like CmkTextfield, have
	// special considerations for gaining focus (namely, the fact
	// that the widget itself doesn't take keyboard focus, the
	// ClutterText child widget does). Widgets such as those
	// should set grab focus themselves.
	clutter_actor_grab_key_focus(self_);
	return CLUTTER_EVENT_STOP;
}

static void on_focus_stack_top_mapped(ClutterActor *actor)
{
	if(clutter_actor_is_mapped(actor))
		clutter_actor_grab_key_focus(actor);
}

void cmk_focus_stack_push(CmkWidget *widget)
{
	if(focusStackTopMappedSignalId && focusStack)
		g_signal_handler_disconnect(focusStack->data, focusStackTopMappedSignalId);
	focusStack = g_list_prepend(focusStack, widget);
	focusStackTopMappedSignalId = g_signal_connect(widget, "notify::mapped", G_CALLBACK(on_focus_stack_top_mapped), NULL);
	on_focus_stack_top_mapped(CLUTTER_ACTOR(widget));
	cmk_grab(TRUE);
}

void cmk_focus_stack_pop(CmkWidget *widget)
{
	g_return_if_fail(focusStack);
	g_return_if_fail(focusStack->data == widget);

	if(focusStackTopMappedSignalId)
		g_signal_handler_disconnect(focusStack->data, focusStackTopMappedSignalId);
	focusStackTopMappedSignalId = 0;
	focusStack = g_list_delete_link(focusStack, focusStack);
	cmk_grab(FALSE);
	if(!focusStack)
		return;
	focusStackTopMappedSignalId = g_signal_connect(focusStack->data, "notify::mapped", G_CALLBACK(on_focus_stack_top_mapped), NULL);
	on_focus_stack_top_mapped(CLUTTER_ACTOR(focusStack->data));
}

void cmk_set_grab_handler(CmkGrabHandler handler, gpointer userdata)
{
	grabHandler = handler;
	grabHandlerData = userdata;
}

void cmk_grab(gboolean grab)
{
	if(grabHandler)
		grabHandler(grab, grabHandlerData);
}



void cmk_scale_actor_box(ClutterActorBox *b, float scale, gboolean move)
{
	if(!b)
		return;
	float width = b->x2 - b->x1;
	float height = b->y2 - b->y1;

	if(move)
	{
		b->x1 *= scale;
		b->y1 *= scale;
	}

	b->x2 = b->x1 + width*scale;
	b->y2 = b->y1 + height*scale;
}

void cairo_set_source_clutter_color(cairo_t *cr, const ClutterColor *color)
{
	if(!color) return; // Don't produce warnings because this happens naturally sometimes during object destruction
	cairo_set_source_rgba(cr, color->red/255.0, color->green/255.0, color->blue/255.0, color->alpha/255.0);
}

void cmk_disabled_color(ClutterColor *dest, const ClutterColor *source)
{
	dest->alpha = source->alpha * 0.5;
	float avg = (source->red * 0.41)
	            + (source->green * 0.92)
	            + (source->blue * 0.27);
	if(avg > 255)
		avg = 255;
	dest->red = avg;
	dest->green = avg;
	dest->blue = avg;
}

void cmk_copy_color(ClutterColor *dest, const ClutterColor *source)
{
	dest->alpha = source->alpha;
	dest->red = source->red;
	dest->green = source->green;
	dest->blue = source->blue;
}


/*
 * Using regular g_debug or g_message isn't always useful on widgets
 * because you can't enable it only on specific widgets. If you have
 * a big scene, that means you'll get LOTS of messages. So, you can
 * turn on debug messages for a single widget here.
 */
void cmk_widget_set_debug(CmkWidget *self, gboolean debug)
{
	g_return_if_fail(CMK_IS_WIDGET(self));
	PRIVATE(self)->debug = debug;
}

gboolean cmk_widget_get_debug(CmkWidget *self)
{
	return PRIVATE(self)->debug;
}

static void cmk_color_free(CmkColor *color)
{
	if(!color) return;
	if(color->color)
		clutter_color_free(color->color);
	g_free(color->link);
	g_free(color);;
}
