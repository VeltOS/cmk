/*
 * This file is part of graphene-desktop, the desktop environment of VeltOS.
 * Copyright (C) 2016 Velt Technologies, Aidan Shafran <zelbrium@gmail.com>
 * Licensed under the Apache License 2 <www.apache.org/licenses/LICENSE-2.0>.
 */

#include "cmk-label.h"

typedef struct _CmkLabelPrivate CmkLabelPrivate;
struct _CmkLabelPrivate
{
	guint settingsChangedId;
	ClutterText *text;
	ClutterTimeline *timeline;
	gboolean usingSystemSize;
	gboolean nospacing;
	gboolean editable;
	float size, timelineStartSize, timelineEndSize;
	gchar *fontFace;
};

enum
{
	PROP_TEXT = 1,
	PROP_SIZE,
	PROP_FACE,
	PROP_BOLD,
	PROP_LAST
};

static GParamSpec *properties[PROP_LAST];

static void cmk_label_dispose(GObject *self_);
static void cmk_label_set_property(GObject *self_, guint propertyId, const GValue *value, GParamSpec *pspec);
static void cmk_label_get_property(GObject *self_, guint propertyId, GValue *value, GParamSpec *pspec);
static void cmk_label_get_preferred_width(ClutterActor *self_, float forHeight, float *minWidth, float *natWidth);
static void cmk_label_get_preferred_height(ClutterActor *self_, float forWidth, float *minHeight, float *natHeight);
static void cmk_label_allocate(ClutterActor *self_, const ClutterActorBox *box, ClutterAllocationFlags flags);
static void on_styles_changed(CmkWidget *self_, guint flags);
static void on_clutter_settings_changed(CmkLabel *self);
static void set_editable_internal(CmkLabel *self, gboolean editable);

G_DEFINE_TYPE_WITH_PRIVATE(CmkLabel, cmk_label, CMK_TYPE_WIDGET);
#define PRIVATE(text) ((CmkLabelPrivate *)cmk_label_get_instance_private(text))

CmkLabel * cmk_label_new(void)
{
	return CMK_LABEL(g_object_new(CMK_TYPE_LABEL, NULL));
}

CmkLabel * cmk_label_new_with_text(const gchar *text)
{
	return CMK_LABEL(g_object_new(CMK_TYPE_LABEL, "text", text, NULL));
}

CmkLabel * cmk_label_new_full(const gchar *text, gboolean bold)
{
	return CMK_LABEL(g_object_new(CMK_TYPE_LABEL, "text", text, "bold", bold, NULL));
}

static void cmk_label_class_init(CmkLabelClass *class)
{
	GObjectClass *base = G_OBJECT_CLASS(class);
	base->dispose = cmk_label_dispose;
	base->get_property = cmk_label_get_property;
	base->set_property = cmk_label_set_property;

	ClutterActorClass *actorClass = CLUTTER_ACTOR_CLASS(class);
	actorClass->allocate = cmk_label_allocate;
	actorClass->get_preferred_width = cmk_label_get_preferred_width;
	actorClass->get_preferred_height = cmk_label_get_preferred_height;
	
	CMK_WIDGET_CLASS(class)->styles_changed = on_styles_changed;

	properties[PROP_TEXT] =
		g_param_spec_string("text",
		                    "text",
		                    "The text displayed by the label",
		                    NULL,
		                    G_PARAM_READWRITE);

	properties[PROP_SIZE] =
		g_param_spec_float("size",
		                   "font size",
		                   "Font size in dps, or -1 for system default",
		                   -1,
		                   G_MAXFLOAT,
		                   -1,
		                   G_PARAM_READWRITE);

	properties[PROP_FACE] =
		g_param_spec_string("font-face",
		                    "font face",
		                    "The name of the font face to use, or NULL for default",
		                    NULL,
		                    G_PARAM_READWRITE);

	properties[PROP_BOLD] =
		g_param_spec_boolean("bold",
		                     "bold",
		                     "Use bold or light font",
		                     FALSE,
		                     G_PARAM_READWRITE);

	g_object_class_install_properties(base, PROP_LAST, properties);
}

static void cmk_label_init(CmkLabel *self)
{
	CmkLabelPrivate *private = PRIVATE(self);
	
	private->text = CLUTTER_TEXT(clutter_text_new());
	clutter_text_set_line_wrap(private->text, TRUE);
	clutter_text_set_ellipsize(private->text, PANGO_ELLIPSIZE_NONE);
	// Redirect keyboard focus to the ClutterText
	g_signal_connect_swapped(self, "key-focus-in", G_CALLBACK(clutter_actor_grab_key_focus), private->text);
	
	PangoFontDescription *desc = pango_font_description_new();
	clutter_text_set_font_description(private->text, desc);
	pango_font_description_free(desc);

	cmk_label_set_font_size(self, -1);
	clutter_actor_add_child(CLUTTER_ACTOR(self), CLUTTER_ACTOR(private->text));

	private->settingsChangedId = g_signal_connect_swapped(clutter_get_default_backend(),
		"settings-changed",
		G_CALLBACK(on_clutter_settings_changed),
		self);
}

static void cmk_label_dispose(GObject *self_)
{
	CmkLabelPrivate *private = PRIVATE(CMK_LABEL(self_));
	g_clear_object(&private->timeline);
	g_clear_pointer(&private->fontFace, g_free);
	if(private->settingsChangedId)
		g_signal_handler_disconnect(clutter_get_default_backend(), private->settingsChangedId);
	private->settingsChangedId = 0;
	G_OBJECT_CLASS(cmk_label_parent_class)->dispose(self_);
}

static void cmk_label_set_property(GObject *self_, guint propertyId, const GValue *value, GParamSpec *pspec)
{
	g_return_if_fail(CMK_IS_LABEL(self_));
	CmkLabel *self = CMK_LABEL(self_);
	
	switch(propertyId)
	{
	case PROP_TEXT:
		cmk_label_set_text(self, g_value_get_string(value));
		break;
	case PROP_SIZE:
		cmk_label_set_font_size(self, g_value_get_float(value));
		break;
	case PROP_FACE:
		cmk_label_set_font_face(self, g_value_get_string(value));
		break;
	case PROP_BOLD:
		cmk_label_set_bold(self, g_value_get_boolean(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(self, propertyId, pspec);
		break;
	}
}

static void cmk_label_get_property(GObject *self_, guint propertyId, GValue *value, GParamSpec *pspec)
{
	g_return_if_fail(CMK_IS_LABEL(self_));
	CmkLabel *self = CMK_LABEL(self_);
	
	switch(propertyId)
	{
	case PROP_TEXT:
		g_value_set_string(value, cmk_label_get_text(self));
		break;
	case PROP_SIZE:
		g_value_set_float(value, cmk_label_get_font_size(self));
		break;
	case PROP_FACE:
		g_value_set_string(value, cmk_label_get_font_face(self));
		break;
	case PROP_BOLD:
		g_value_set_boolean(value, cmk_label_get_bold(self));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(self, propertyId, pspec);
		break;
	}
}

static void cmk_label_get_preferred_width(ClutterActor *self_, float forHeight, float *minWidth, float *natWidth)
{
	clutter_actor_get_preferred_width(CLUTTER_ACTOR(PRIVATE(CMK_LABEL(self_))->text), forHeight, minWidth, natWidth);
}

static void cmk_label_get_preferred_height(ClutterActor *self_, float forWidth, float *minHeight, float *natHeight)
{
	CmkLabelPrivate *private = PRIVATE(CMK_LABEL(self_));
	if(private->nospacing)
		*natHeight = *minHeight = CMK_DP(self_, private->size);
	else
		clutter_actor_get_preferred_height(CLUTTER_ACTOR(PRIVATE(CMK_LABEL(self_))->text), forWidth, minHeight, natHeight);
}

static void cmk_label_allocate(ClutterActor *self_, const ClutterActorBox *box, ClutterAllocationFlags flags)
{
	CmkLabelPrivate *private = PRIVATE(CMK_LABEL(self_));
	
	// The correct size will be calculated in the height request, so
	// now just center the text.
	if(private->nospacing)
	{
		float height;
		clutter_actor_get_preferred_height(CLUTTER_ACTOR(private->text),
			(box->x2 - box->x1),
			NULL, &height);
		float offset = (height - (box->y2 - box->y1))/2.0;
		
		ClutterActorBox text = {0, -offset, box->x2 - box->x1, height};
		clutter_actor_allocate(CLUTTER_ACTOR(private->text), &text, flags);
	}
	else
	{
		ClutterActorBox text = {0, 0, box->x2 - box->x1, box->y2 - box->y1};
		clutter_actor_allocate(CLUTTER_ACTOR(private->text), &text, flags);
	}
	
	CLUTTER_ACTOR_CLASS(cmk_label_parent_class)->allocate(self_, box, flags);
}

static void on_styles_changed(CmkWidget *self_, guint flags)
{
	CMK_WIDGET_CLASS(cmk_label_parent_class)->styles_changed(self_, flags);
	if((flags & CMK_STYLE_FLAG_COLORS)
	|| (flags & CMK_STYLE_FLAG_DISABLED))
	{
		const ClutterColor *color = cmk_widget_get_default_named_color(self_, "foreground");
		if(cmk_widget_get_disabled(self_))
		{
			ClutterColor final;
			cmk_disabled_color(&final, color);
			clutter_text_set_color(PRIVATE(CMK_LABEL(self_))->text, &final);
		}
		else
			clutter_text_set_color(PRIVATE(CMK_LABEL(self_))->text, color);
	}
	if(flags & CMK_STYLE_FLAG_DP)
		cmk_label_set_font_size(CMK_LABEL(self_), PRIVATE(CMK_LABEL(self_))->size);
	if((flags & CMK_STYLE_FLAG_DISABLED) && PRIVATE(CMK_LABEL(self_))->editable)
		set_editable_internal(CMK_LABEL(self_), !cmk_widget_get_disabled(self_));
}

static void on_clutter_settings_changed(CmkLabel *self)
{
	if(PRIVATE(self)->usingSystemSize)
		cmk_label_set_font_size(self, -1);
}

static float get_system_font_size_dp(CmkLabel *self)
{
	ClutterSettings *settings = clutter_settings_get_default();
	gchar *name = NULL;
	g_object_get(settings, "font-name", &name, NULL);
	PangoFontDescription *desc = pango_font_description_from_string(name);
	g_free(name);
	
	float dpScale = cmk_widget_get_dp_scale(CMK_WIDGET(self));
	
	float size = pango_font_description_get_size(desc);
	size /= PANGO_SCALE;
	if(pango_font_description_get_size_is_absolute(desc))
		size /= dpScale;
	else
		size = (96.0/72.0)*size / dpScale;
	pango_font_description_free(desc);
	return size;
}

// Must be converted from -1 beforehand
static void internal_set_font_size(CmkLabel *self, float size)
{
	CmkLabelPrivate *private = PRIVATE(CMK_LABEL(self));
	private->size = size;
	double abs = CMK_DP(self, size);
	
	// Get the current font description instead of using private->desc,
	// because it may have been modified by the user
	PangoFontDescription *desc = pango_font_description_copy(clutter_text_get_font_description(private->text));
	pango_font_description_set_absolute_size(desc, abs*PANGO_SCALE);
	clutter_text_set_font_description(private->text, desc);
	pango_font_description_free(desc);
}

void cmk_label_set_font_size(CmkLabel *self, float size)
{
	g_return_if_fail(CMK_IS_LABEL(self));
	CmkLabelPrivate *private = PRIVATE(CMK_LABEL(self));
	private->usingSystemSize = (size < 0);
	if(size < 0)
		size = get_system_font_size_dp(self);
	internal_set_font_size(self, size);
	// TODO: notify signal
}

static void timeline_new_frame(CmkLabel *self, UNUSED guint msecs, ClutterTimeline *timeline)
{
	CmkLabelPrivate *private = PRIVATE(CMK_LABEL(self));
	float progress = clutter_timeline_get_progress(timeline);
	float size = private->timelineStartSize + (private->timelineEndSize - private->timelineStartSize)*progress;
	internal_set_font_size(self, size);
}

void cmk_label_animate_font_size(CmkLabel *self, float size, guint duration)
{
	g_return_if_fail(CMK_IS_LABEL(self));
	CmkLabelPrivate *private = PRIVATE(CMK_LABEL(self));
	private->usingSystemSize = (size < 0);
	if(size < 0)
		size = get_system_font_size_dp(self);
	
	// If we're already transitioning to this new size, just exit.
	if(private->timeline
	&& clutter_timeline_is_playing(private->timeline)
	&& private->timelineEndSize == size)
		return;
	
	g_clear_object(&private->timeline);
	private->timelineStartSize = private->size;
	private->timelineEndSize = size;
	private->timeline = clutter_timeline_new(duration);
	g_signal_connect_swapped(private->timeline, "new-frame", G_CALLBACK(timeline_new_frame), self);
	clutter_timeline_set_progress_mode(private->timeline, CLUTTER_EASE_OUT_QUAD);
	clutter_timeline_start(private->timeline);
}

float cmk_label_get_font_size(CmkLabel *self)
{
	g_return_val_if_fail(CMK_IS_LABEL(self), 0);
	return PRIVATE(self)->size;
}

void cmk_label_set_font_face(CmkLabel *self, const gchar *faceName)
{
	g_return_if_fail(CMK_IS_LABEL(self));
	CmkLabelPrivate *private = PRIVATE(CMK_LABEL(self));
	
	g_clear_pointer(&private->fontFace, g_free);
	private->fontFace = g_strdup(faceName);

	PangoFontDescription *desc = pango_font_description_copy(clutter_text_get_font_description(private->text));
	pango_font_description_set_family_static(desc, faceName);
	clutter_text_set_font_description(private->text, desc);
	pango_font_description_free(desc);
}

const gchar * cmk_label_get_font_face(CmkLabel *self)
{
	g_return_val_if_fail(CMK_IS_LABEL(self), NULL);
	return PRIVATE(self)->fontFace;
}

void cmk_label_set_bold(CmkLabel *self, gboolean bold)
{
	g_return_if_fail(CMK_IS_LABEL(self));

	PangoFontDescription *desc = pango_font_description_copy(clutter_text_get_font_description(PRIVATE(self)->text));
	pango_font_description_set_weight(desc, bold ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL);
	clutter_text_set_font_description(PRIVATE(self)->text, desc);
	pango_font_description_free(desc);
}

gboolean cmk_label_get_bold(CmkLabel *self)
{
	g_return_val_if_fail(CMK_IS_LABEL(self), FALSE);
	PangoFontDescription *desc = clutter_text_get_font_description(PRIVATE(self)->text);
	return pango_font_description_get_weight(desc) > PANGO_WEIGHT_NORMAL;
}

void cmk_label_set_text(CmkLabel *self, const gchar *val)
{
	g_return_if_fail(CMK_IS_LABEL(self));
	clutter_text_set_text(PRIVATE(self)->text, val);
}

void cmk_label_set_markup(CmkLabel *self, const gchar *markup)
{
	g_return_if_fail(CMK_IS_LABEL(self));
	clutter_text_set_markup(PRIVATE(self)->text, markup);
}

const gchar * cmk_label_get_text(CmkLabel *self)
{
	g_return_val_if_fail(CMK_IS_LABEL(self), NULL);
	return clutter_text_get_text(PRIVATE(self)->text);
}

void cmk_label_set_line_alignment(CmkLabel *self, PangoAlignment alignment)
{
	g_return_if_fail(CMK_IS_LABEL(self));
	clutter_text_set_line_alignment(PRIVATE(self)->text, alignment);
}

static void set_editable_internal(CmkLabel *self, gboolean editable)
{
	clutter_text_set_editable(PRIVATE(self)->text, editable);
	clutter_actor_set_reactive(CLUTTER_ACTOR(PRIVATE(self)->text), editable);
	cmk_widget_set_tabbable(CMK_WIDGET(self), editable);
}

void cmk_label_set_editable(CmkLabel *self, gboolean editable)
{
	g_return_if_fail(CMK_IS_LABEL(self));
	if(PRIVATE(self)->editable != editable)
	{
		PRIVATE(self)->editable = editable;
		if(editable && cmk_widget_get_disabled(CMK_WIDGET(self)))
			return;
		set_editable_internal(self, editable);
	}
}

void cmk_label_set_no_spacing(CmkLabel *self, gboolean nospacing)
{
	g_return_if_fail(CMK_IS_LABEL(self));
	if(PRIVATE(self)->nospacing == nospacing)
		return;
	PRIVATE(self)->nospacing = nospacing;
	clutter_actor_queue_relayout(CLUTTER_ACTOR(self));
}

ClutterText * cmk_label_get_clutter_text(CmkLabel *self)
{
	g_return_val_if_fail(CMK_IS_LABEL(self), NULL);
	return PRIVATE(self)->text;
}
