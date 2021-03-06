/*
 * This file is part of graphene-desktop, the desktop environment of VeltOS.
 * Copyright (C) 2016 Velt Technologies, Aidan Shafran <zelbrium@gmail.com>
 * Licensed under the Apache License 2 <www.apache.org/licenses/LICENSE-2.0>.
 */

#include "cmk-button.h"
#include "cmk-label.h"
#include "cmk-shadow.h"
#include <math.h>

typedef struct _CmkButtonPrivate CmkButtonPrivate;
struct _CmkButtonPrivate 
{
	CmkWidget *content;
	CmkLabel *text;
	gboolean held, hover, selected;
	CmkButtonType type;
	ClutterTimeline *downAnim;
	ClutterTimeline *upAnim;
	ClutterPoint clickPoint;
	CmkShadowEffect *shadow;
};

enum
{
	PROP_TEXT = 1,
	PROP_TYPE,
	PROP_LAST
};

enum
{
	SIGNAL_ACTIVATE = 1,
	SIGNAL_LAST
};

static GParamSpec *properties[PROP_LAST];
static guint signals[SIGNAL_LAST];

static void cmk_button_dispose(GObject *self_);
static void cmk_button_set_property(GObject *self_, guint propertyId, const GValue *value, GParamSpec *pspec);
static void cmk_button_get_property(GObject *self_, guint propertyId, GValue *value, GParamSpec *pspec);
static void cmk_button_get_preferred_width(ClutterActor *self_, gfloat forHeight, gfloat *minWidth, gfloat *natWidth);
static void cmk_button_get_preferred_height(ClutterActor *self_, gfloat forWidth, gfloat *minHeight, gfloat *natHeight);
static void cmk_button_allocate(ClutterActor *self_, const ClutterActorBox *box, ClutterAllocationFlags flags);
static gboolean on_button_press(ClutterActor *self_, ClutterButtonEvent *event);
static gboolean on_button_release(ClutterActor *self_, ClutterButtonEvent *event);
static gboolean on_crossing(ClutterActor *self_, ClutterCrossingEvent *event);
static void on_styles_changed(CmkWidget *self_, guint flags);
static gboolean on_draw_canvas(ClutterCanvas *canvas, cairo_t *cr, int width, int height, CmkButton *self);
static gboolean on_key_pressed(ClutterActor *self_, ClutterKeyEvent *event);
static void on_key_focus(ClutterActor *self_);
static void on_key_unfocus(ClutterActor *self_);

G_DEFINE_TYPE_WITH_PRIVATE(CmkButton, cmk_button, CMK_TYPE_WIDGET);
#define PRIVATE(button) ((CmkButtonPrivate *)cmk_button_get_instance_private(button))



CmkButton * cmk_button_new(CmkButtonType type)
{
	return CMK_BUTTON(g_object_new(CMK_TYPE_BUTTON, "type", type, NULL));
}

CmkButton * cmk_button_new_with_text(const gchar *text, CmkButtonType type)
{
	return CMK_BUTTON(g_object_new(CMK_TYPE_BUTTON, "text", text, "type", type, NULL));
}

static void cmk_button_class_init(CmkButtonClass *class)
{
	GObjectClass *base = G_OBJECT_CLASS(class);
	base->dispose = cmk_button_dispose;
	base->set_property = cmk_button_set_property;
	base->get_property = cmk_button_get_property;

	ClutterActorClass *actorClass = CLUTTER_ACTOR_CLASS(class);
	actorClass->enter_event = on_crossing;
	actorClass->leave_event = on_crossing;
	actorClass->get_preferred_width = cmk_button_get_preferred_width;
	actorClass->get_preferred_height = cmk_button_get_preferred_height;
	actorClass->allocate = cmk_button_allocate;
	actorClass->key_press_event = on_key_pressed;
	actorClass->key_focus_in = on_key_focus;
	actorClass->key_focus_out = on_key_unfocus;
	actorClass->button_press_event = on_button_press;
	actorClass->button_release_event = on_button_release;

	CMK_WIDGET_CLASS(class)->styles_changed = on_styles_changed;

	properties[PROP_TEXT] = g_param_spec_string("text", "text", "The button's text label", "", G_PARAM_READWRITE);
	properties[PROP_TYPE] = g_param_spec_int("type", "type", "CmkButtonType value of this button", CMK_BUTTON_TYPE_EMBED, CMK_BUTTON_TYPE_ACTION, CMK_BUTTON_TYPE_RAISED, G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
	
	g_object_class_install_properties(base, PROP_LAST, properties);

	/**
	 * CmkButton::activate:
	 * @self: The button being activated
	 *
	 * Emitted when the user presses and releases a mouse button. CmkButton
	 * does not yet distinguish between left/right/other clicks.
	 */
	signals[SIGNAL_ACTIVATE] = g_signal_new("activate", G_TYPE_FROM_CLASS(class), G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(CmkButtonClass, activate), NULL, NULL, NULL, G_TYPE_NONE, 0);
}

static void on_anim_canvas(CmkButton *self)
{
	if(clutter_actor_get_content(CLUTTER_ACTOR(self)))
		clutter_content_invalidate(clutter_actor_get_content(CLUTTER_ACTOR(self)));
}

static void cmk_button_init(CmkButton *self)
{
	ClutterActor *self_ = CLUTTER_ACTOR(self);
	CmkButtonPrivate *private = PRIVATE(self);
	
	clutter_actor_set_reactive(self_, TRUE);
	cmk_widget_set_tabbable(CMK_WIDGET(self), TRUE);

	ClutterContent *canvas = clutter_canvas_new();
	g_signal_connect(canvas, "draw", G_CALLBACK(on_draw_canvas), self);
	clutter_actor_set_content_gravity(self_, CLUTTER_CONTENT_GRAVITY_CENTER);
	clutter_actor_set_content(self_, canvas);

	// TODO: Styling time
	private->downAnim = clutter_timeline_new(600);
	private->upAnim = clutter_timeline_new(600);
	clutter_timeline_set_progress_mode(private->downAnim, CLUTTER_EASE_OUT_CUBIC);
	g_signal_connect_swapped(private->downAnim, "new-frame", G_CALLBACK(on_anim_canvas), self);
	g_signal_connect_swapped(private->upAnim, "new-frame", G_CALLBACK(on_anim_canvas), self);
}

static void cmk_button_dispose(GObject *self_)
{
	CmkButtonPrivate *private = PRIVATE(CMK_BUTTON(self_));
	clutter_actor_set_content(CLUTTER_ACTOR(self_), NULL);
	if(private->downAnim)
		clutter_timeline_stop(private->downAnim);
	if(private->upAnim)
		clutter_timeline_stop(private->upAnim);
	g_clear_object(&private->downAnim);
	g_clear_object(&private->upAnim);
	if(private->held)
		cmk_grab(FALSE); // Be careful to not let a cmk_grab(TRUE) go unpaired
	private->held = FALSE;
	G_OBJECT_CLASS(cmk_button_parent_class)->dispose(self_);
}

static void cmk_button_set_property(GObject *self_, guint propertyId, const GValue *value, GParamSpec *pspec)
{
	g_return_if_fail(CMK_IS_BUTTON(self_));
	CmkButton *self = CMK_BUTTON(self_);
	
	switch(propertyId)
	{
	case PROP_TEXT:
		cmk_button_set_text(self, g_value_get_string(value));
		break;
	case PROP_TYPE:
		cmk_button_set_type(self, g_value_get_int(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(self, propertyId, pspec);
		break;
	}
}

static void cmk_button_get_property(GObject *self_, guint propertyId, GValue *value, GParamSpec *pspec)
{
	g_return_if_fail(CMK_IS_BUTTON(self_));
	CmkButton *self = CMK_BUTTON(self_);
	
	switch(propertyId)
	{
	case PROP_TEXT:
		g_value_set_string(value, cmk_button_get_text(self));
		break;
	case PROP_TYPE:
		g_value_set_int(value, PRIVATE(self)->type);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(self, propertyId, pspec);
		break;
	}
}

// Based on Material design spec
#define WIDTH_PADDING 16 // dp
#define HEIGHT_PADDING 9 // dp
#define BEVEL_RADIUS 2 // dp

static void cmk_button_get_preferred_width(ClutterActor *self_, gfloat forHeight, gfloat *minWidth, gfloat *natWidth)
{
	CmkButtonPrivate *private = PRIVATE(CMK_BUTTON(self_));
	*minWidth = 0;
	float padMul = cmk_widget_get_padding_multiplier(CMK_WIDGET(self_));
	float wPad = CMK_DP(self_, ((private->content && !private->text) ? HEIGHT_PADDING : WIDTH_PADDING)) * padMul;

	if(private->content)
	{
		gfloat min, nat;
		clutter_actor_get_preferred_width(CLUTTER_ACTOR(private->content), forHeight, &min, &nat);
		*minWidth += nat;
	}
	
	if(private->content && private->text)
		*minWidth += wPad;

	if(private->text)
	{
		gfloat min, nat;
		clutter_actor_get_preferred_width(CLUTTER_ACTOR(private->text), forHeight, &min, &nat);
		*minWidth += nat;
	}

	*minWidth += (wPad*2);
	*natWidth = *minWidth;
}

static void cmk_button_get_preferred_height(ClutterActor *self_, gfloat forWidth, gfloat *minHeight, gfloat *natHeight)
{
	CmkButtonPrivate *private = PRIVATE(CMK_BUTTON(self_));
	*minHeight = 0;
	float padMul = cmk_widget_get_padding_multiplier(CMK_WIDGET(self_));
	float hPad = CMK_DP(self_, HEIGHT_PADDING) * padMul;
	
	if(private->content)
	{
		gfloat min, nat;
		clutter_actor_get_preferred_height(CLUTTER_ACTOR(private->content), forWidth, &min, &nat);
		*minHeight = nat;
	}

	if(private->text)
	{
		gfloat min, nat;
		clutter_actor_get_preferred_height(CLUTTER_ACTOR(private->text), forWidth, &min, &nat);
		*minHeight = MAX(nat, *minHeight);
	}

	*minHeight += (hPad*2);
	*natHeight = *minHeight;
}

static void cmk_button_allocate(ClutterActor *self_, const ClutterActorBox *box, ClutterAllocationFlags flags)
{
	/*
	 * Goal is to get an allocation like this:
	 * ----------------------------
	 * |                          |  <- padding (hPad)
	 * |  [Con.] [Text         ]  |  <- minHeight
	 * |                          |
	 * ----------------------------
	 *	   ^---- minWidth ----^  ^wPad
	 * Where both Content and Text are optional (both, either, neither).
	 * If both are present, Content should get its natural width and Text
	 * gets the rest. Otherwise, the single child gets all the space except
	 * padding.
	 */
	
	gfloat maxHeight = box->y2 - box->y1;
	gfloat maxWidth = box->x2 - box->x1;
	ClutterContent *canvas = clutter_actor_get_content(CLUTTER_ACTOR(self_));
	if(canvas)
		clutter_canvas_set_size(CLUTTER_CANVAS(canvas), maxWidth, maxHeight);

	CmkButtonPrivate *private = PRIVATE(CMK_BUTTON(self_));
	if(!private->content && !private->text)
	{
		CLUTTER_ACTOR_CLASS(cmk_button_parent_class)->allocate(self_, box, flags);
		return;
	}

	float dp = cmk_widget_get_dp_scale(CMK_WIDGET(self_));
	float padMul = cmk_widget_get_padding_multiplier(CMK_WIDGET(self_));
	float wPadding = CMK_DP(self_, ((private->content && !private->text) ? HEIGHT_PADDING : WIDTH_PADDING)) * padMul;
	float hPadding = HEIGHT_PADDING * dp * padMul;
	
	gfloat minHeight, natHeight, minWidth, natWidth;
	clutter_actor_get_preferred_height(self_, -1, &minHeight, &natHeight);
	clutter_actor_get_preferred_width(self_, -1, &minWidth, &natWidth);

	gfloat hPad = MIN(MAX((maxHeight - (minHeight-(hPadding*2)))/2, 0), hPadding);
	gfloat wPad = MIN(MAX((maxWidth - (minWidth-(wPadding*2)))/2, 0), wPadding);
	hPad = (gint)hPad;
	wPad = (gint)wPad;

	if(private->content && private->text)
	{
		gfloat min, nat;
		clutter_actor_get_preferred_width(CLUTTER_ACTOR(private->content), maxHeight-hPad*2, &min, &nat);
		gfloat contentRight = (gint)(MIN(wPad+nat, maxWidth));
		ClutterActorBox contentBox = {wPad, hPad, contentRight, maxHeight-hPad};
		clutter_actor_allocate(CLUTTER_ACTOR(private->content), &contentBox, flags);
		gfloat textRight = MAX(contentRight+wPad, maxWidth-wPad);
		ClutterActorBox textBox = {contentRight+wPad, hPad, textRight, maxHeight-hPad};
		clutter_actor_allocate(CLUTTER_ACTOR(private->text), &textBox, flags);
	}
	else
	{
		ClutterActorBox contentBox = {wPad, hPad, maxWidth-wPad, maxHeight-hPad};
		if(private->content)
			clutter_actor_allocate(CLUTTER_ACTOR(private->content), &contentBox, flags);
		else
			clutter_actor_allocate(CLUTTER_ACTOR(private->text), &contentBox, flags);
	}

	CLUTTER_ACTOR_CLASS(cmk_button_parent_class)->allocate(self_, box, flags);
}

static gboolean on_button_press(ClutterActor *self_, ClutterButtonEvent *event)
{
	if(cmk_widget_get_disabled(CMK_WIDGET(self_)))
		return CLUTTER_EVENT_STOP;
	CmkButtonPrivate *private = PRIVATE(CMK_BUTTON(self_));
	private->held = TRUE;
	
	cmk_grab(TRUE);
	clutter_input_device_grab(event->device, self_);
	
	gfloat rx, ry;
	clutter_actor_get_transformed_position(self_, &rx, &ry);
	clutter_point_init(&private->clickPoint, event->x - rx, event->y - ry);
	clutter_timeline_stop(private->upAnim);
	clutter_timeline_stop(private->downAnim);
	clutter_timeline_start(private->downAnim);
	return CLUTTER_ACTOR_CLASS(cmk_button_parent_class)->button_press_event(self_, event);
}

static gboolean on_button_release(ClutterActor *self_, ClutterButtonEvent *event)
{
	CmkButtonPrivate *private = PRIVATE(CMK_BUTTON(self_));
	if(G_UNLIKELY(!private->held))
		return CLUTTER_EVENT_STOP;
	private->held = FALSE;
	clutter_input_device_ungrab(event->device);
	
	clutter_timeline_stop(private->upAnim);
	clutter_timeline_start(private->upAnim);

	private->hover = clutter_actor_has_pointer(self_);
	clutter_content_invalidate(clutter_actor_get_content(self_));
	
	if(private->hover)
		g_signal_emit(self_, signals[SIGNAL_ACTIVATE], 0);
	cmk_grab(FALSE);
	return CLUTTER_EVENT_STOP;
}

static gboolean on_crossing(ClutterActor *self_, ClutterCrossingEvent *event)
{
	gboolean hover =
		(event->type == CLUTTER_ENTER && !cmk_widget_get_disabled(CMK_WIDGET(self_)));
	if(hover != PRIVATE(CMK_BUTTON(self_))->hover)
	{
		if(PRIVATE(CMK_BUTTON(self_))->shadow)
			cmk_shadow_effect_animate_radius(PRIVATE(CMK_BUTTON(self_))->shadow, hover ? 1 : 0.5);
		PRIVATE(CMK_BUTTON(self_))->hover = hover;
		clutter_content_invalidate(clutter_actor_get_content(self_));
	}
	return TRUE;
}

static void on_styles_changed(CmkWidget *self_, guint flags)
{
	CMK_WIDGET_CLASS(cmk_button_parent_class)->styles_changed(self_, flags);
	ClutterContent *content = clutter_actor_get_content(CLUTTER_ACTOR(self_));
	if(content)
		clutter_content_invalidate(content);
	if((flags & CMK_STYLE_FLAG_DISABLED))
	{
		CmkButtonPrivate *private = PRIVATE(CMK_BUTTON(self_));
		gboolean disabled = cmk_widget_get_disabled(self_);
		if(private->type == CMK_BUTTON_TYPE_RAISED && !disabled)
		{
			if(!private->shadow)
			{
				private->shadow = cmk_shadow_effect_new(5);
				cmk_shadow_effect_set(private->shadow, 0, 3, 0.5, 0);
				clutter_actor_add_effect(CLUTTER_ACTOR(self_), CLUTTER_EFFECT(private->shadow));
			}
			//cmk_shadow_effect_animate_radius(private->shadow, 0.5);
		}
		else
		{
			clutter_actor_clear_effects(CLUTTER_ACTOR(self_));
			private->shadow = NULL; // self->shadow owned by the actor
		}
		cmk_widget_set_tabbable(self_, !disabled);
	}
}

static gboolean on_draw_canvas(UNUSED ClutterCanvas *canvas, cairo_t *cr, int width, int height, CmkButton *self)
{
	CmkButtonPrivate *private = PRIVATE(self);
	
	// Clear
	cairo_save(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
	cairo_paint(cr);
	cairo_restore(cr);

	// For non-square buttons, clip the drawing area
	if(private->type != CMK_BUTTON_TYPE_EMBED)
	{
		double radius;
		double degrees = M_PI / 180.0;

		if(private->type == CMK_BUTTON_TYPE_FLAT || private->type == CMK_BUTTON_TYPE_RAISED)
			radius = CMK_DP(self, BEVEL_RADIUS) * cmk_widget_get_bevel_radius_multiplier(CMK_WIDGET(self));
		else
			radius = MIN(width, height)/2;

		radius = MIN(MAX(radius, 0), MIN(width,height)/2);

		cairo_new_sub_path(cr);
		cairo_arc(cr, width - radius, radius, radius, -90 * degrees, 0 * degrees);
		cairo_arc(cr, width - radius, height - radius, radius, 0 * degrees, 90 * degrees);
		cairo_arc(cr, radius, height - radius, radius, 90 * degrees, 180 * degrees);
		cairo_arc(cr, radius, radius, radius, 180 * degrees, 270 * degrees);
		cairo_close_path(cr);
		cairo_clip(cr);
	}

	// Paint background
	if(private->type == CMK_BUTTON_TYPE_RAISED)
	{
		const ClutterColor *color = cmk_widget_get_default_named_color(CMK_WIDGET(self), "primary");
		ClutterColor final;
		if(cmk_widget_get_disabled(CMK_WIDGET(self)))
			cmk_disabled_color(&final, color);
		else
			cmk_copy_color(&final, color);
		cairo_set_source_clutter_color(cr, &final);
		cairo_paint(cr);
	}
	else if(private->type == CMK_BUTTON_TYPE_ACTION)
	{
		const ClutterColor *color = cmk_widget_get_default_named_color(CMK_WIDGET(self), "accent");
		cairo_set_source_clutter_color(cr, color);
		cairo_paint(cr);
	}

	// Paint hover / selected color
	if((private->hover
	|| private->selected
	|| (cmk_widget_get_just_tabbed(CMK_WIDGET(self)) && clutter_actor_has_key_focus(CLUTTER_ACTOR(self))))
	   && !cmk_widget_get_disabled(CMK_WIDGET(self)))
	{
		const gchar *color = private->hover ? "hover" : "selected";
		cairo_set_source_clutter_color(cr, cmk_widget_get_default_named_color(CMK_WIDGET(self), color));
		cairo_paint(cr);
	}
	
	// Click anim (includes time when the user holds the button)
	if(private->held
	|| clutter_timeline_is_playing(private->upAnim)
	|| clutter_timeline_is_playing(private->downAnim))
	{
		gdouble downAnimProgress = clutter_timeline_get_progress(private->downAnim);
		gdouble upAnimProgress = clutter_timeline_get_progress(private->upAnim);
		
		// The down anim resets to 0 progress after it stops, which means the
		// circle will dissappear before the up animation can complete.
		if(!clutter_timeline_is_playing(private->downAnim))
			downAnimProgress = 1;
		
		gfloat x = PRIVATE(self)->clickPoint.x;
		gfloat y = PRIVATE(self)->clickPoint.y;
		
		cairo_new_sub_path(cr);
		cairo_arc(cr, x, y, MAX(width, height)*1.5*downAnimProgress, 0, 2*M_PI);
		cairo_close_path(cr);
		cairo_set_source_rgba(cr, 1, 1, 1, (1-upAnimProgress)*0.2);
		cairo_fill(cr);
	}
	
	return TRUE;
}

void cmk_button_set_text(CmkButton *self, const gchar *text)
{
	g_return_if_fail(CMK_IS_BUTTON(self));
	if(text)
	{
		if(!PRIVATE(self)->text)
		{
			PRIVATE(self)->text = cmk_label_new();
			clutter_actor_set_y_align(CLUTTER_ACTOR(PRIVATE(self)->text), CLUTTER_ACTOR_ALIGN_CENTER);
			clutter_actor_add_child(CLUTTER_ACTOR(self), CLUTTER_ACTOR(PRIVATE(self)->text));
		}
		cmk_label_set_text(PRIVATE(self)->text, text);
	}
	else if(PRIVATE(self)->text)
	{
		clutter_actor_remove_child(CLUTTER_ACTOR(self), CLUTTER_ACTOR(PRIVATE(self)->text));
	}
}

const gchar * cmk_button_get_text(CmkButton *self)
{
	g_return_val_if_fail(CMK_IS_BUTTON(self), NULL);
	if(!PRIVATE(self)->text)
		return NULL;
	return cmk_label_get_text(PRIVATE(self)->text);
}

void cmk_button_set_content(CmkButton *self, CmkWidget *content)
{
	g_return_if_fail(CMK_IS_BUTTON(self));
	if(clutter_actor_get_parent(CLUTTER_ACTOR(content)))
		return;
	if(PRIVATE(self)->content)
		clutter_actor_remove_child(CLUTTER_ACTOR(self), CLUTTER_ACTOR(PRIVATE(self)->content));
	PRIVATE(self)->content = content;
	if(content)
		clutter_actor_add_child(CLUTTER_ACTOR(self), CLUTTER_ACTOR(content));
	//clutter_actor_queue_relayout(CLUTTER_ACTOR(self));
}

CmkWidget * cmk_button_get_content(CmkButton *self)
{
	g_return_val_if_fail(CMK_IS_BUTTON(self), NULL);
	return PRIVATE(self)->content;
}

void cmk_button_set_type(CmkButton *self, CmkButtonType type)
{
	g_return_if_fail(CMK_IS_BUTTON(self));
	if(PRIVATE(self)->type != type)
	{
		PRIVATE(self)->type = type;
		gboolean disabled = cmk_widget_get_disabled(CMK_WIDGET(self));
		if(type == CMK_BUTTON_TYPE_RAISED && !disabled)
		{
			PRIVATE(self)->shadow = cmk_shadow_effect_new(5);
			cmk_shadow_effect_set(PRIVATE(self)->shadow, 0, 3, 0.5, 0);
			clutter_actor_add_effect(CLUTTER_ACTOR(self), CLUTTER_EFFECT(PRIVATE(self)->shadow));
		}
		else
		{
			clutter_actor_clear_effects(CLUTTER_ACTOR(self));
			PRIVATE(self)->shadow = NULL; // self->shadow owned by the actor
		}
		clutter_content_invalidate(clutter_actor_get_content(CLUTTER_ACTOR(self)));
	}
}

CmkButtonType cmk_button_get_btype(CmkButton *self)
{
	g_return_val_if_fail(CMK_IS_BUTTON(self), CMK_BUTTON_TYPE_EMBED);
	return PRIVATE(self)->type;
}

void cmk_button_set_selected(CmkButton *self, gboolean selected)
{
	g_return_if_fail(CMK_IS_BUTTON(self));
	if(PRIVATE(self)->selected == selected)
		return;
	PRIVATE(self)->selected = selected;
	clutter_content_invalidate(clutter_actor_get_content(CLUTTER_ACTOR(self)));
}

gboolean cmk_button_get_selected(CmkButton *self)
{
	g_return_val_if_fail(CMK_IS_BUTTON(self), FALSE);
	return PRIVATE(self)->selected;
}

const gchar * cmk_button_get_name(CmkButton *self)
{
	g_return_val_if_fail(CMK_IS_BUTTON(self), NULL);
	const gchar *name = clutter_actor_get_name(CLUTTER_ACTOR(self));
	if(name)
		return name;
	return cmk_button_get_text(self);
}

static gboolean on_key_pressed(ClutterActor *self_, ClutterKeyEvent *event)
{
	if(event->keyval == CLUTTER_KEY_Return
	&& !cmk_widget_get_disabled(CMK_WIDGET(self_)))
	{
		clutter_point_init(&PRIVATE(CMK_BUTTON(self_))->clickPoint, -1, -1);
		clutter_timeline_stop(PRIVATE(CMK_BUTTON(self_))->downAnim);
		clutter_timeline_stop(PRIVATE(CMK_BUTTON(self_))->upAnim);
		clutter_timeline_start(PRIVATE(CMK_BUTTON(self_))->upAnim);
		// Down anim starting at the same frame as up glitches out, so meh
		//clutter_timeline_start(PRIVATE(CMK_BUTTON(self_))->downAnim);
		g_signal_emit(self_, signals[SIGNAL_ACTIVATE], 0);
		return CLUTTER_EVENT_STOP;
	}
	return CLUTTER_EVENT_PROPAGATE;
}

static void on_key_focus(ClutterActor *self_)
{
	if(!cmk_widget_get_disabled(CMK_WIDGET(self_)) && clutter_actor_get_content(self_))
		clutter_content_invalidate(clutter_actor_get_content(self_));
}

static void on_key_unfocus(ClutterActor *self_)
{
	if(!cmk_widget_get_disabled(CMK_WIDGET(self_)) && clutter_actor_get_content(self_))
		clutter_content_invalidate(clutter_actor_get_content(self_));
}
