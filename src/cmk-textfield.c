/*
* This file is part of graphene-desktop, the desktop environment of VeltOS.
* Copyright (C) 2016 Velt Technologies, Aidan Shafran <zelbrium@gmail.com>
* Licensed under the Apache License 2 <www.apache.org/licenses/LICENSE-2.0>.
*/

#include "cmk-textfield.h"
#include "cmk-label.h"
#include <string.h>

// TODO: Multi-line input. Due to line spacing adjustments, this will require
// getting the line spacing value from the PangoLayout instead of calculating
// it as (actual-request)/2 in _allocate.

// Sizes in dp, from Material Design standards
#define INPUT_LABEL_SIZE 16
#define HELPER_LABEL_SIZE 12
#define LABEL_PADDING 8
#define TOP_PADDING 16
#define UNDERLINE_UNFOCUS_SIZE 1
#define UNDERLINE_FOCUS_SIZE 2
// Opacity values averaged between standards for "Light" and "Dark" themes
#define PLACEHOLDER_OPACITY (0.46*255)
#define DESCRIPTION_OPACITY (0.62*255)
#define TRANS_TIME 50 // ms

typedef struct _CmkTextfieldPrivate CmkTextfieldPrivate;
struct _CmkTextfieldPrivate 
{
	gchar *placeholder;
	gchar *error;
	gboolean showClear, showName, showDescription;
	CmkLabel *name;
	CmkLabel *description;
	CmkLabel *input;
	CmkWidget *underline, *underlineFocus;
	ClutterTimeline *focusTimeline;
	gboolean focus;
};

enum
{
	PROP_NAME = 1,
	PROP_DESCRIPTION,
	PROP_PLACEHOLDER,
	PROP_SHOW_CLEAR,
	PROP_LAST
};

enum
{
	SIGNAL_CHANGED = 1,
	SIGNAL_ACTIVATE,
	SIGNAL_LAST
};

static GParamSpec *properties[PROP_LAST];
static guint signals[SIGNAL_LAST];

static void cmk_textfield_dispose(GObject *self_);
static void cmk_textfield_set_property(GObject *self_, guint propertyId, const GValue *value, GParamSpec *pspec);
static void cmk_textfield_get_property(GObject *self_, guint propertyId, GValue *value, GParamSpec *pspec);
static void cmk_textfield_get_preferred_width(ClutterActor *self_, gfloat forHeight, gfloat *minWidth, gfloat *natWidth);
static void cmk_textfield_get_preferred_height(ClutterActor *self_, gfloat forWidth, gfloat *minHeight, gfloat *natHeight);
static void timeline_new_frame(CmkTextfield *self, gint msecs, ClutterTimeline *timeline);
static void on_input_focus_in(CmkTextfield *self);
static void on_input_focus_out(CmkTextfield *self);
static void cmk_textfield_allocate(ClutterActor *self_, const ClutterActorBox *box, ClutterAllocationFlags flags);
//static gboolean on_crossing(ClutterActor *self_, ClutterCrossingEvent *event);

G_DEFINE_TYPE_WITH_PRIVATE(CmkTextfield, cmk_textfield, CMK_TYPE_WIDGET);
#define PRIVATE(textfield) ((CmkTextfieldPrivate *)cmk_textfield_get_instance_private(textfield))


CmkTextfield * cmk_textfield_new(const gchar *name, const gchar *description)
{
	return CMK_TEXTFIELD(g_object_new(CMK_TYPE_TEXTFIELD, "name", name, "description", description, NULL));
}

static void cmk_textfield_class_init(CmkTextfieldClass *class)
{
	GObjectClass *base = G_OBJECT_CLASS(class);
	base->dispose = cmk_textfield_dispose;
	base->set_property = cmk_textfield_set_property;
	base->get_property = cmk_textfield_get_property;
	
	ClutterActorClass *actorClass = CLUTTER_ACTOR_CLASS(class);
	actorClass->get_preferred_width = cmk_textfield_get_preferred_width;
	actorClass->get_preferred_height = cmk_textfield_get_preferred_height;
	actorClass->allocate = cmk_textfield_allocate;
	
	properties[PROP_NAME] =
		g_param_spec_string("name",
		                    "name",
		                    "Textfield name which shoes in or above the textfield depending on keyboard focus.",
		                    NULL,
		                    G_PARAM_READWRITE);
	properties[PROP_DESCRIPTION] =
		g_param_spec_string("description",
		                    "description",
		                    "Description label which shows below the textfield.",
		                    NULL,
		                    G_PARAM_READWRITE);
	properties[PROP_PLACEHOLDER] =
		g_param_spec_string("placeholder",
		                    "placeholder",
		                    "Placeholder text to show before the user starts typing.",
		                    NULL,
		                    G_PARAM_READWRITE);
	properties[PROP_SHOW_CLEAR] =
		g_param_spec_boolean("show-clear",
		                     "show clear",
		                     "Whether or not to show the clear button at the right.",
		                     TRUE,
		                     G_PARAM_READWRITE);
	
	g_object_class_install_properties(base, PROP_LAST, properties);
	
	signals[SIGNAL_CHANGED] =
		g_signal_new("changed",
		             G_TYPE_FROM_CLASS(class),
		             G_SIGNAL_RUN_FIRST,
		             G_STRUCT_OFFSET(CmkTextfieldClass, changed),
		             NULL, NULL, NULL, G_TYPE_NONE, 0);
	
	signals[SIGNAL_ACTIVATE] =
		g_signal_new("activate",
		             G_TYPE_FROM_CLASS(class),
		             G_SIGNAL_RUN_FIRST,
		             G_STRUCT_OFFSET(CmkTextfieldClass, activate),
		             NULL, NULL, NULL, G_TYPE_NONE, 0);
}

static void cmk_textfield_init(CmkTextfield *self)
{
	CmkTextfieldPrivate *private = PRIVATE(self);
	cmk_widget_set_tabbable(CMK_WIDGET(self), TRUE);
	private->showClear = TRUE;

	private->focusTimeline = clutter_timeline_new(TRANS_TIME);
	clutter_timeline_set_progress_mode(private->focusTimeline, CLUTTER_EASE_OUT_QUAD);
	g_signal_connect_swapped(private->focusTimeline, "new-frame", G_CALLBACK(timeline_new_frame), self);
	
	private->input = cmk_label_new();
	cmk_label_set_font_size(private->input, INPUT_LABEL_SIZE);
	cmk_label_set_editable(private->input);
	cmk_label_set_no_spacing(private->input, TRUE);
	ClutterText *inputClutterText = cmk_label_get_clutter_text(private->input);
	clutter_text_set_single_line_mode(inputClutterText, TRUE);
	g_signal_connect_swapped(inputClutterText, "key-focus-in", G_CALLBACK(on_input_focus_in), self);
	g_signal_connect_swapped(inputClutterText, "key-focus-out", G_CALLBACK(on_input_focus_out), self);
	
	private->name = cmk_label_new();
	cmk_label_set_no_spacing(private->name, TRUE);
	cmk_label_set_font_size(private->name, INPUT_LABEL_SIZE);
	clutter_actor_set_opacity(CLUTTER_ACTOR(private->name), PLACEHOLDER_OPACITY);
	clutter_text_set_single_line_mode(cmk_label_get_clutter_text(private->name), TRUE);
	
	private->description = cmk_label_new();
	cmk_label_set_no_spacing(private->description, TRUE);
	cmk_label_set_font_size(private->description, HELPER_LABEL_SIZE);
	clutter_actor_set_opacity(CLUTTER_ACTOR(private->description), DESCRIPTION_OPACITY);
	clutter_text_set_single_line_mode(cmk_label_get_clutter_text(private->description), TRUE);

	private->underline = cmk_widget_new();
	private->underlineFocus = cmk_widget_new();
	cmk_widget_set_background_color(private->underline, "foreground");
	cmk_widget_set_draw_background_color(private->underline, TRUE);
	cmk_widget_set_background_color(private->underlineFocus, "accent");
	cmk_widget_set_draw_background_color(private->underlineFocus, TRUE);
	
	clutter_actor_add_child(CLUTTER_ACTOR(self), CLUTTER_ACTOR(private->underline));
	clutter_actor_add_child(CLUTTER_ACTOR(self), CLUTTER_ACTOR(private->underlineFocus));
	clutter_actor_add_child(CLUTTER_ACTOR(self), CLUTTER_ACTOR(private->name));
	clutter_actor_add_child(CLUTTER_ACTOR(self), CLUTTER_ACTOR(private->description));
	clutter_actor_add_child(CLUTTER_ACTOR(self), CLUTTER_ACTOR(private->input));
}

static void cmk_textfield_dispose(GObject *self_)
{
	CmkTextfieldPrivate *private = PRIVATE(CMK_TEXTFIELD(self_));
	g_clear_pointer(&private->placeholder, g_free);
	g_clear_pointer(&private->error, g_free);
	g_clear_object(&private->focusTimeline);
	G_OBJECT_CLASS(cmk_textfield_parent_class)->dispose(self_);
}

static void cmk_textfield_set_property(GObject *self_, guint propertyId, const GValue *value, GParamSpec *pspec)
{
	g_return_if_fail(CMK_IS_TEXTFIELD(self_));
	CmkTextfield *self = CMK_TEXTFIELD(self_);
	
	switch(propertyId)
	{
	case PROP_NAME:
		cmk_textfield_set_name(self, g_value_get_string(value));
		break;
	case PROP_DESCRIPTION:
		cmk_textfield_set_description(self, g_value_get_string(value));
		break;
	case PROP_PLACEHOLDER:
		cmk_textfield_set_placeholder(self, g_value_get_string(value));
		break;
	case PROP_SHOW_CLEAR:
		cmk_textfield_set_show_clear(self, g_value_get_boolean(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(self, propertyId, pspec);
		break;
	}
}

static void cmk_textfield_get_property(GObject *self_, guint propertyId, GValue *value, GParamSpec *pspec)
{
	g_return_if_fail(CMK_IS_TEXTFIELD(self_));
	CmkTextfieldPrivate *private = PRIVATE(CMK_TEXTFIELD(self_));
	
	switch(propertyId)
	{
	case PROP_NAME:
		g_value_set_string(value, cmk_label_get_text(private->input));
		break;
	case PROP_DESCRIPTION:
		g_value_set_string(value, cmk_label_get_text(private->description));
		break;
	case PROP_PLACEHOLDER:
		g_value_set_string(value, private->placeholder);
		break;
	case PROP_SHOW_CLEAR:
		g_value_set_boolean(value, private->showClear);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(self_, propertyId, pspec);
		break;
	}
}

static void cmk_textfield_get_preferred_width(ClutterActor *self_, gfloat forHeight, gfloat *minWidth, gfloat *natWidth)
{
	CmkTextfieldPrivate *private = PRIVATE(CMK_TEXTFIELD(self_));
	float natA, natB;
	clutter_actor_get_preferred_width(CLUTTER_ACTOR(private->name), -1, NULL, &natA);
	clutter_actor_get_preferred_width(CLUTTER_ACTOR(private->description), -1, NULL, &natB);
	*minWidth = *natWidth = MAX(natA, natB);
}

static void cmk_textfield_get_preferred_height(ClutterActor *self_, gfloat forWidth, gfloat *minHeight, gfloat *natHeight)
{
	CmkTextfieldPrivate *private = PRIVATE(CMK_TEXTFIELD(self_));
	
	gfloat height = 0;
	height += TOP_PADDING;
	
	if(private->showName)
	{
		height += HELPER_LABEL_SIZE;
		height += LABEL_PADDING;
	}
	
	height += INPUT_LABEL_SIZE;
	height += LABEL_PADDING;
	height += UNDERLINE_FOCUS_SIZE;
	
	if(private->showDescription)
	{
		height += LABEL_PADDING;
		height += HELPER_LABEL_SIZE;
	}
	
	height += LABEL_PADDING;
	*minHeight = *natHeight = CMK_DP(self_, height);
}

static void timeline_new_frame(CmkTextfield *self, gint msecs, ClutterTimeline *timeline)
{
	CmkTextfieldPrivate *private = PRIVATE(self);
	clutter_actor_queue_relayout(CLUTTER_ACTOR(self));
}

static void on_input_focus_in(CmkTextfield *self)
{
	CmkTextfieldPrivate *private = PRIVATE(self);
	private->focus = TRUE;
	clutter_timeline_set_direction(private->focusTimeline, CLUTTER_TIMELINE_FORWARD);
	clutter_timeline_stop(private->focusTimeline);
	clutter_timeline_start(private->focusTimeline);
	cmk_label_animate_font_size(private->name, HELPER_LABEL_SIZE, TRANS_TIME*4/3);
	clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

	//cmk_label_set_bold(private->name, TRUE);

	// TODO: Using the accent/primary color on light backgrounds looks good,
	// but on dark backgrounds it's hard to read. How to deal with this?
	// For now, just use foreground color

	//const ClutterColor *r = cmk_widget_get_named_color(CMK_WIDGET(self), "accent");
	//ClutterText *text = cmk_label_get_clutter_text(private->name);
	//clutter_actor_save_easing_state(CLUTTER_ACTOR(text));
	//clutter_actor_set_easing_duration(CLUTTER_ACTOR(text), TRANS_TIME);
	//clutter_text_set_color(text, r);
	//clutter_actor_restore_easing_state(CLUTTER_ACTOR(text));

	clutter_actor_save_easing_state(CLUTTER_ACTOR(private->name));
	clutter_actor_set_easing_duration(CLUTTER_ACTOR(private->name), TRANS_TIME);
	clutter_actor_set_opacity(CLUTTER_ACTOR(private->name), 255);
	clutter_actor_restore_easing_state(CLUTTER_ACTOR(private->name));
}

static void on_input_focus_out(CmkTextfield *self)
{
	CmkTextfieldPrivate *private = PRIVATE(self);
	private->focus = FALSE;
	clutter_timeline_set_direction(private->focusTimeline, CLUTTER_TIMELINE_BACKWARD);
	clutter_timeline_stop(private->focusTimeline);
	clutter_timeline_start(private->focusTimeline);
	if(strlen(cmk_label_get_text(private->input)) == 0)
		cmk_label_animate_font_size(private->name, INPUT_LABEL_SIZE, TRANS_TIME*4/3);
	clutter_actor_queue_relayout(CLUTTER_ACTOR(self));
	
	//const ClutterColor *r = cmk_widget_get_named_color(CMK_WIDGET(self), "foreground");
	//ClutterText *text = cmk_label_get_clutter_text(private->name);
	//clutter_actor_save_easing_state(CLUTTER_ACTOR(text));
	//clutter_actor_set_easing_duration(CLUTTER_ACTOR(text), TRANS_TIME);
	//clutter_text_set_color(text, r);
	//clutter_actor_restore_easing_state(CLUTTER_ACTOR(text));

	clutter_actor_save_easing_state(CLUTTER_ACTOR(private->name));
	clutter_actor_set_easing_duration(CLUTTER_ACTOR(private->name), TRANS_TIME);
	clutter_actor_set_opacity(CLUTTER_ACTOR(private->name), PLACEHOLDER_OPACITY);
	clutter_actor_restore_easing_state(CLUTTER_ACTOR(private->name));
}


static void cmk_textfield_allocate(ClutterActor *self_, const ClutterActorBox *box, ClutterAllocationFlags flags)
{
	CmkTextfieldPrivate *private = PRIVATE(CMK_TEXTFIELD(self_));

	ClutterActorBox childBox = {0, 0, box->x2-box->x1, box->y2-box->y1};
	float dpScale = CMK_DP(self_, 1);
	float transition = 0;
	ClutterText *inputClutterText = cmk_label_get_clutter_text(private->input);
	gboolean hasText = strlen(cmk_label_get_text(private->input)) > 0;
	if(clutter_timeline_is_playing(private->focusTimeline))
		transition = clutter_timeline_get_progress(private->focusTimeline);
	else if(private->focus)
		transition = 1;
	else
		transition = 0;
	childBox.y1 = TOP_PADDING*dpScale;

	ClutterActorBox nameBox = {
		childBox.x1,
		childBox.y1,
		childBox.x2,
		childBox.y1 + HELPER_LABEL_SIZE*dpScale
	};
	
	if(private->showName)
		childBox.y1 += HELPER_LABEL_SIZE*dpScale + LABEL_PADDING*dpScale;
	
	ClutterActorBox inputBox = {
		childBox.x1, childBox.y1,
		childBox.x2, childBox.y1 + INPUT_LABEL_SIZE*dpScale
	};
	
	if(private->showName)
	{
		if(hasText)
		{
			clutter_actor_allocate(CLUTTER_ACTOR(private->name), &nameBox, flags);
		}
		else
		{
			ClutterActorBox out;
			clutter_actor_box_interpolate(&inputBox, &nameBox, transition, &out);
			clutter_actor_allocate(CLUTTER_ACTOR(private->name), &out, flags);
		}
	}

	clutter_actor_allocate(CLUTTER_ACTOR(private->input), &inputBox, flags);
	childBox.y1 += INPUT_LABEL_SIZE*dpScale;
	childBox.y1 += LABEL_PADDING*dpScale;

	ClutterActorBox underlineBox = {
		childBox.x1, childBox.y1,
		childBox.x2, childBox.y1 + UNDERLINE_UNFOCUS_SIZE*dpScale
	};
	clutter_actor_allocate(CLUTTER_ACTOR(private->underline), &underlineBox, flags);

	float width = childBox.x2 - childBox.x1;
	ClutterActorBox underlineFocusBox = {
		childBox.x1+(width/2)*(1-transition), childBox.y1,
		childBox.x2-(width/2)*(1-transition), childBox.y1 + UNDERLINE_FOCUS_SIZE*dpScale
	};
	clutter_actor_allocate(CLUTTER_ACTOR(private->underlineFocus), &underlineFocusBox, flags);

	childBox.y1 += UNDERLINE_FOCUS_SIZE*dpScale;

	if(private->showDescription)
	{
		childBox.y1 += LABEL_PADDING*dpScale;
		ClutterActorBox descriptionBox = {
			childBox.x1, childBox.y1,
			childBox.x2, childBox.y1 + HELPER_LABEL_SIZE*dpScale
		};
		clutter_actor_allocate(CLUTTER_ACTOR(private->description), &descriptionBox, flags);
	}
	
	CLUTTER_ACTOR_CLASS(cmk_textfield_parent_class)->allocate(self_, box, flags);
}

static void on_styles_changed(CmkWidget *self_, guint flags)
{
	CMK_WIDGET_CLASS(cmk_textfield_parent_class)->styles_changed(self_, flags);
	CmkTextfieldPrivate *private = PRIVATE(CMK_TEXTFIELD(self_));
}

void cmk_textfield_set_text(CmkTextfield *self, const gchar *text)
{
	g_return_if_fail(CMK_IS_TEXTFIELD(self));
	cmk_label_set_text(PRIVATE(self)->input, text);
}

const gchar * cmk_textfield_get_text(CmkTextfield *self)
{
	g_return_val_if_fail(CMK_IS_TEXTFIELD(self), NULL);
	return cmk_label_get_text(PRIVATE(self)->input);
}

void cmk_textfield_set_name(CmkTextfield *self, const gchar *text)
{
	g_return_if_fail(CMK_IS_TEXTFIELD(self));
	PRIVATE(self)->showName = (text != NULL);
	cmk_label_set_text(PRIVATE(self)->name, text);
}

void cmk_textfield_set_description(CmkTextfield *self, const gchar *text)
{
	g_return_if_fail(CMK_IS_TEXTFIELD(self));
	PRIVATE(self)->showDescription = (text != NULL);
	cmk_label_set_text(PRIVATE(self)->description, text);
}

void cmk_textfield_set_placeholder(CmkTextfield *self, const gchar *placeholder)
{
}

void cmk_textfield_set_error(CmkTextfield *self, const gchar *error)
{
}

void cmk_textfield_set_show_clear(CmkTextfield *self, gboolean show)
{
}

void cmk_textfield_set_is_password(CmkTextfield *self, gboolean isPassword)
{
	g_return_if_fail(CMK_IS_TEXTFIELD(self));
	// 0x2022 is a 'bullet' unicode char.
	// TODO: ClutterText selection is broken when using a password char,
	// probably due to confusing about the width of the text
	if(isPassword)
		clutter_text_set_password_char(cmk_label_get_clutter_text(PRIVATE(self)->input), 0x2022);
	else
		clutter_text_set_password_char(cmk_label_get_clutter_text(PRIVATE(self)->input), 0);
}
