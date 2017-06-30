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
	gfloat size, scale;
};

enum
{
	PROP_TEXT = 1,
	PROP_SIZE,
	PROP_BOLD,
	PROP_SCALE,
	PROP_LAST
};

static GParamSpec *properties[PROP_LAST];

static void cmk_label_dispose(GObject *self_);
static void cmk_label_set_property(GObject *self_, guint propertyId, const GValue *value, GParamSpec *pspec);
static void cmk_label_get_property(GObject *self_, guint propertyId, GValue *value, GParamSpec *pspec);
static void cmk_label_get_preferred_width(ClutterActor *self_, gfloat forHeight, gfloat *minWidth, gfloat *natWidth);
static void cmk_label_get_preferred_height(ClutterActor *self_, gfloat forWidth, gfloat *minHeight, gfloat *natHeight);
static void cmk_label_allocate(ClutterActor *self_, const ClutterActorBox *box, ClutterAllocationFlags flags);
static void on_styles_changed(CmkWidget *self_, guint flags);
static void on_clutter_settings_changed(CmkLabel *self);

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

CmkLabel * cmk_label_new_full(const gchar *text, gfloat scale, gboolean bold)
{
	return CMK_LABEL(g_object_new(CMK_TYPE_LABEL, "text", text, "scale", scale, "bold", bold, NULL));
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

	properties[PROP_TEXT] = g_param_spec_string("text", "text", "text", NULL, G_PARAM_READWRITE);
	properties[PROP_SIZE] = g_param_spec_float("size", "size", "size in pt", -1, G_MAXFLOAT, -1, G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
	properties[PROP_SCALE] = g_param_spec_float("scale", "scale", "scale", 0, G_MAXFLOAT, 1, G_PARAM_READWRITE);
	properties[PROP_BOLD] = g_param_spec_boolean("bold", "bold", "bold", FALSE, G_PARAM_READWRITE);

	g_object_class_install_properties(base, PROP_LAST, properties);
}

static void cmk_label_init(CmkLabel *self)
{
	g_message("label: %p", self);
	CmkLabelPrivate *private = PRIVATE(self);
	private->scale = 1;
	
	private->text = CLUTTER_TEXT(clutter_text_new());
	clutter_text_set_line_wrap(private->text, TRUE);
	clutter_text_set_ellipsize(private->text, PANGO_ELLIPSIZE_NONE);
	
	PangoFontDescription *desc = pango_font_description_new();
	clutter_text_set_font_description(private->text, desc);
	pango_font_description_free(desc);

	clutter_actor_add_child(CLUTTER_ACTOR(self), CLUTTER_ACTOR(private->text));

	private->settingsChangedId = g_signal_connect_swapped(clutter_get_default_backend(),
		"settings-changed",
		G_CALLBACK(on_clutter_settings_changed),
		self);
}

static void cmk_label_dispose(GObject *self_)
{
	CmkLabelPrivate *private = PRIVATE(CMK_LABEL(self_));
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
		cmk_label_set_font_size_pt(self, g_value_get_float(value));
		break;
	case PROP_SCALE:
		cmk_label_set_font_scale(self, g_value_get_float(value));
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
		g_value_set_float(value, cmk_label_get_font_size_pt(self));
		break;
	case PROP_SCALE:
		g_value_set_float(value, cmk_label_get_font_scale(self));
		break;
	case PROP_BOLD:
		g_value_set_boolean(value, cmk_label_get_bold(self));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(self, propertyId, pspec);
		break;
	}
}

static void cmk_label_get_preferred_width(ClutterActor *self_, gfloat forHeight, gfloat *minWidth, gfloat *natWidth)
{
	clutter_actor_get_preferred_width(CLUTTER_ACTOR(PRIVATE(CMK_LABEL(self_))->text), forHeight, minWidth, natWidth);
	// TODO
	gfloat padding = 20;//cmk_widget_style_get_padding(CMK_WIDGET(self_));
}

static void cmk_label_get_preferred_height(ClutterActor *self_, gfloat forWidth, gfloat *minHeight, gfloat *natHeight)
{
	clutter_actor_get_preferred_height(CLUTTER_ACTOR(PRIVATE(CMK_LABEL(self_))->text), forWidth, minHeight, natHeight);
	// TODO
	gfloat padding = 20;//cmk_widget_style_get_padding(CMK_WIDGET(self_));
}

static void cmk_label_allocate(ClutterActor *self_, const ClutterActorBox *box, ClutterAllocationFlags flags)
{
	cmk_label_set_font_size_pt(CMK_LABEL(self_), PRIVATE(CMK_LABEL(self_))->size);
	// TODO
	gfloat padding = 20;//cmk_widget_style_get_padding(CMK_WIDGET(self_));
	ClutterActorBox text = {0, 0, box->x2 - box->x1, box->y2 - box->y1};
	clutter_actor_allocate(CLUTTER_ACTOR(PRIVATE(CMK_LABEL(self_))->text), &text, flags);
	CLUTTER_ACTOR_CLASS(cmk_label_parent_class)->allocate(self_, box, flags);
}

static void on_styles_changed(CmkWidget *self_, guint flags)
{
	CMK_WIDGET_CLASS(cmk_label_parent_class)->styles_changed(self_, flags);
	if((flags & CMK_STYLE_FLAG_COLORS)
	|| (flags & CMK_STYLE_FLAG_BACKGROUND_NAME))
	{
		const ClutterColor *color = cmk_widget_get_foreground_clutter_color(self_);
		clutter_text_set_color(PRIVATE(CMK_LABEL(self_))->text, color);
	}
	if(flags & CMK_STYLE_FLAG_DP)
		cmk_label_set_font_size_pt(CMK_LABEL(self_), PRIVATE(CMK_LABEL(self_))->size);
}

static void on_clutter_settings_changed(CmkLabel *self)
{
	if(PRIVATE(self)->size < 0)
		cmk_label_set_font_size_pt(self, -1);
}

void cmk_label_set_font_size_pt(CmkLabel *self, gfloat size)
{
	g_return_if_fail(CMK_IS_LABEL(self));
	PRIVATE(self)->size = size;

	if(size < 0)
	{
		ClutterSettings *settings = clutter_settings_get_default();
		gchar *name = NULL;
		g_object_get(settings, "font-name", &name, NULL);
		
		PangoFontDescription *desc = pango_font_description_from_string(name);
		g_free(name);
		
		size = pango_font_description_get_size(desc);
		size /= PANGO_SCALE;
		//if(pango_font_description_get_size_is_absolute(desc))
		pango_font_description_free(desc);
	}

	double abs = size * (96.0/72.0) * PRIVATE(self)->scale * cmk_widget_get_dp_scale(CMK_WIDGET(self)); 

	// Get the current font description instead of using private->desc,
	// because it may have been modified by the user
	PangoFontDescription *desc = pango_font_description_copy_static(clutter_text_get_font_description(PRIVATE(self)->text));
	pango_font_description_set_absolute_size(desc, abs*PANGO_SCALE);
	clutter_text_set_font_description(PRIVATE(self)->text, desc);
	pango_font_description_free(desc);
}

gfloat cmk_label_get_font_size_pt(CmkLabel *self)
{
	g_return_val_if_fail(CMK_IS_LABEL(self), 0);
	return PRIVATE(self)->size;
}

void cmk_label_set_font_scale(CmkLabel *self, gfloat fontScale)
{
	g_return_if_fail(CMK_IS_LABEL(self));
	PRIVATE(self)->scale = fontScale;
	cmk_label_set_font_size_pt(self, PRIVATE(self)->size);
}

gfloat cmk_label_get_font_scale(CmkLabel *self)
{
	g_return_val_if_fail(CMK_IS_LABEL(self), 0);
	return PRIVATE(self)->scale;
}

void cmk_label_set_bold(CmkLabel *self, gboolean bold)
{
	g_return_if_fail(CMK_IS_LABEL(self));

	PangoFontDescription *desc = pango_font_description_copy_static(clutter_text_get_font_description(PRIVATE(self)->text));
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

ClutterText * cmk_label_get_clutter_text(CmkLabel *self)
{
	g_return_val_if_fail(CMK_IS_LABEL(self), NULL);
	return PRIVATE(self)->text;
}
