/*
 * libcmk, Copyright 2018 Aidan Shafran <zelbrium@gmail.com>
 * Apache License 2.0 <www.apache.org/licenses/LICENSE-2.0>.
 */

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <pango/pangocairo.h>
#include "cmk-label.h"

#define CMK_LABEL_EVENT_MASK 0

// Only set when a CmkLabel exists
static PangoContext *gDefaultContext = NULL;

typedef struct _CmkLabelPrivate CmkLabelPrivate;
struct _CmkLabelPrivate
{
	PangoContext *defaultContextRef;
	PangoContext *context;
	bool contextIsSet; // true if set, false if using default context

	// When using a set context, keep track of serial to
	// make sure it actually changed before doing relayout
	guint contextSerial;

	PangoLayout *layout;
	bool singleline;
};

enum
{
	PROP_PANGO_CONTEXT = 1,
	PROP_TEXT,
	PROP_SINGLE_LINE,
	// TODO
	//PROP_ALIGNMENT,
	PROP_BOLD,
	//PROP_FONT_SIZE,
	//PROP_FONT_FAMILY,
	PROP_LAST,
	PROP_OVERRIDE_EVENT_MASK
};

static GParamSpec *properties[PROP_LAST];

G_DEFINE_TYPE_WITH_PRIVATE(CmkLabel, cmk_label, CMK_TYPE_WIDGET);
#define PRIV(label) ((CmkLabelPrivate *)cmk_label_get_instance_private(label))

static void on_dispose(GObject *self_);
static void set_property(GObject *self_, guint id, const GValue *value, GParamSpec *pspec);
static void get_property(GObject *self_, guint id, GValue *value, GParamSpec *pspec);
static void on_draw(CmkWidget *self_, cairo_t *cr);
static void get_preferred_width(CmkWidget *self_, float forHeight, float *min, float *nat);
static void get_preferred_height(CmkWidget *self_, float forWidth, float *min, float *nat);
static void get_draw_rect(CmkWidget *self_, CmkRect *rect);

static void set_pango_context(CmkLabel *self, PangoContext *context);

static PangoLayout * cmk_pango_layout_copy_new_context(PangoLayout *original, PangoContext *context);


CmkLabel * cmk_label_new(const char *text)
{
	return CMK_LABEL(g_object_new(CMK_TYPE_LABEL,
		"text", text,
		NULL));
}

CmkLabel * cmk_label_new_bold(const char *text)
{
	return CMK_LABEL(g_object_new(CMK_TYPE_LABEL,
		"text", text,
		"bold", true,
		NULL));
}

static void cmk_label_class_init(CmkLabelClass *class)
{
	GObjectClass *base = G_OBJECT_CLASS(class);
	base->dispose = on_dispose;
	base->get_property = get_property;
	base->set_property = set_property;

	CmkWidgetClass *widgetClass = CMK_WIDGET_CLASS(class);
	widgetClass->draw = on_draw;
	widgetClass->get_preferred_width = get_preferred_width;
	widgetClass->get_preferred_height = get_preferred_height;
	widgetClass->get_draw_rect = get_draw_rect;

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

	/**
	 * CmkWidget:text:
	 *
	 * The text being displayed.
	 */
	properties[PROP_TEXT] =
		g_param_spec_string("text", "text", "text",
		                    NULL,
		                    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

	/**
	 * CmkWidget:single-line:
	 *
	 * To force only a single line of text.
	 */
	properties[PROP_SINGLE_LINE] =
		g_param_spec_boolean("single-line", "single line", "single line",
		                     false,
		                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

	/**
	 * CmkWidget:bold:
	 *
	 * Bold text or not. More specific variation of font weight
	 * can be made using the PangoLayout object from
	 * cmk_label_get_layout().
	 */
	properties[PROP_BOLD] =
		g_param_spec_boolean("bold", "bold", "bold",
		                     false,
		                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

	g_object_class_install_properties(base, PROP_LAST, properties);

	g_object_class_override_property(base, PROP_OVERRIDE_EVENT_MASK, "event-mask");
}

static void cmk_label_init(CmkLabel *self)
{
	CmkLabelPrivate *priv = PRIV(self);
	//priv->useDefaultSize = true;

	// Make sure the default context exists
	if(PANGO_IS_CONTEXT(gDefaultContext))
	{
		// Keep the default context around while any CmkLabels exist
		priv->defaultContextRef = g_object_ref(gDefaultContext);
	}
	else
	{
		PangoFontMap *fontmap = pango_cairo_font_map_get_default();
		gDefaultContext = pango_font_map_create_context(fontmap);
		g_object_add_weak_pointer(G_OBJECT(gDefaultContext), (gpointer *)&gDefaultContext);
		priv->defaultContextRef = gDefaultContext;
	}

	// Initialize to default context
	priv->context = g_object_ref(priv->defaultContextRef);
	priv->layout = pango_layout_new(priv->context);
	priv->contextIsSet = false;
}

static void on_dispose(GObject *self_)
{
	CmkLabelPrivate *priv = PRIV(CMK_LABEL(self_));
	g_clear_object(&priv->layout);
	g_clear_object(&priv->context);
	g_clear_object(&priv->defaultContextRef);
	G_OBJECT_CLASS(cmk_label_parent_class)->dispose(self_);
}

static void get_property(GObject *self_, guint id, GValue *value, GParamSpec *pspec)
{
	CmkLabel *self = CMK_LABEL(self_);
	
	switch(id)
	{
	case PROP_PANGO_CONTEXT:
		g_value_set_object(value, PRIV(self)->contextIsSet ? PRIV(self)->context : NULL);
		break;
	case PROP_TEXT:
		g_value_set_string(value, cmk_label_get_text(self));
		break;
	case PROP_SINGLE_LINE:
		g_value_set_boolean(value, PRIV(self)->singleline);
		break;
	case PROP_BOLD:
		g_value_set_boolean(value, cmk_label_get_bold(self));
		break;
	case PROP_OVERRIDE_EVENT_MASK:
		g_value_set_uint(value, CMK_LABEL_EVENT_MASK);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(self, id, pspec);
		break;
	}
}

static void set_property(GObject *self_, guint id, const GValue *value, GParamSpec *pspec)
{
	CmkLabel *self = CMK_LABEL(self_);
	
	switch(id)
	{
	case PROP_PANGO_CONTEXT:
		set_pango_context(self, PANGO_CONTEXT(g_value_get_object(value)));
		break;
	case PROP_TEXT:
		cmk_label_set_text(self, g_value_get_string(value));
		break;
	case PROP_SINGLE_LINE:
		cmk_label_set_single_line(self, g_value_get_boolean(value));
		break;
	case PROP_BOLD:
		cmk_label_set_bold(self, g_value_get_boolean(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(self, id, pspec);
		break;
	}
}

static void on_draw(CmkWidget *self_, cairo_t *cr)
{
	CmkLabelPrivate *priv = PRIV(CMK_LABEL(self_));
	PangoLayout *layout = priv->layout;

	pango_cairo_update_context(cr, priv->context);

	float width, height;
	cmk_widget_get_size(self_, &width, &height);
	pango_layout_set_width(layout, width * PANGO_SCALE);

	if(priv->singleline)
		pango_layout_set_height(layout, 0);
	else
		pango_layout_set_height(layout, height * PANGO_SCALE);

	pango_cairo_show_layout(cr, layout);
}

static void get_preferred_width(CmkWidget *self_, float forHeight, float *min, float *nat)
{
	CmkLabelPrivate *priv = PRIV(CMK_LABEL(self_));
	PangoLayout *layout = priv->layout;
	int originalWidth = pango_layout_get_width(layout);
	int originalHeight = pango_layout_get_height(layout);

	pango_layout_set_width(layout, -1);

	if(priv->singleline)
		pango_layout_set_height(layout, 0);
	else if(forHeight >= 0)
		pango_layout_set_height(layout, forHeight * PANGO_SCALE);
	else
		pango_layout_set_height(layout, INT_MAX);

	PangoRectangle logicalRect = {0};
	pango_layout_get_extents(layout, NULL, &logicalRect);
	float logicalWidth = (float)(logicalRect.x + logicalRect.width) / PANGO_SCALE;

	if(pango_layout_get_ellipsize(layout) != PANGO_ELLIPSIZE_NONE)
		*min = 0;
	else
		*min = logicalWidth;

	*nat = logicalWidth;

	pango_layout_set_width(layout, originalWidth);
	pango_layout_set_height(layout, originalHeight);
}

static void get_preferred_height(CmkWidget *self_, float forWidth, float *min, float *nat)
{
	CmkLabelPrivate *priv = PRIV(CMK_LABEL(self_));
	PangoLayout *layout = priv->layout;
	int originalWidth = pango_layout_get_width(layout);
	int originalHeight = pango_layout_get_height(layout);

	if(forWidth < 0)
		forWidth = -1;

	pango_layout_set_width(layout, forWidth * PANGO_SCALE);

	if(priv->singleline)
		pango_layout_set_height(layout, 0);
	else
		pango_layout_set_height(layout, INT_MAX);

	PangoRectangle logicalRect = {0};
	pango_layout_get_extents(layout, NULL, &logicalRect);
	float logicalHeight = (float)(logicalRect.y + logicalRect.height) / PANGO_SCALE;

	if(!priv->singleline
	&& pango_layout_get_ellipsize(layout) != PANGO_ELLIPSIZE_NONE)
	{
		// In the case of ellipsizing, the minimum height is 1 line.
		pango_layout_set_height(layout, 0);
		pango_layout_get_extents(layout, NULL, &logicalRect);
		*min = (float)(logicalRect.y + logicalRect.height) / PANGO_SCALE;
	}
	else
		*min = logicalHeight;

	*nat = logicalHeight;

	pango_layout_set_width(layout, originalWidth);
	pango_layout_set_height(layout, originalHeight);
}

static void get_draw_rect(CmkWidget *self_, CmkRect *rect)
{
	PangoLayout *layout = PRIV(CMK_LABEL(self_))->layout;

	float width, height;
	cmk_widget_get_size(self_, &width, &height);

	pango_layout_set_width(layout, width * PANGO_SCALE);
	pango_layout_set_height(layout, height * PANGO_SCALE);

	PangoRectangle inkRect = {0};
	pango_layout_get_extents(layout, &inkRect, NULL);

	rect->x = (float)inkRect.x / PANGO_SCALE;
	rect->y = (float)inkRect.y / PANGO_SCALE;
	rect->width = (float)inkRect.width / PANGO_SCALE;
	rect->height = (float)inkRect.height / PANGO_SCALE;
}

void cmk_label_set_text(CmkLabel *self, const char *text)
{
	g_return_if_fail(CMK_IS_LABEL(self));
	pango_layout_set_text(PRIV(self)->layout, text ? text : "", -1);
	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_TEXT]);
}

void cmk_label_set_markup(CmkLabel *self, const char *markup)
{
	g_return_if_fail(CMK_IS_LABEL(self));
	pango_layout_set_markup(PRIV(self)->layout, markup, -1);
	cmk_widget_relayout(CMK_WIDGET(self));
	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_TEXT]);
}

const char * cmk_label_get_text(CmkLabel *self)
{
	g_return_val_if_fail(CMK_IS_LABEL(self), NULL);
	return pango_layout_get_text(PRIV(self)->layout);
}

#define context_font_desc(priv) pango_context_get_font_description(priv->context)
#define layout_font_desc(priv) pango_layout_get_font_description(priv->layout)
#define desc_has_set(desc, flag) ((desc != NULL) && ((pango_font_description_get_set_fields(desc) & (flag)) != 0))

// Gets the layout's font description, or a blank one if the
// layout's was NULL. Good for writing values. Must free.
static PangoFontDescription * get_write_font_desc(CmkLabel *self)
{
	const PangoFontDescription *d = pango_layout_get_font_description(PRIV(self)->layout);
	return d ? pango_font_description_copy(d) : pango_font_description_new();
}

// Unset value(s) on the layout's font description,
// which will cause the context's values to be used
static void font_desc_unset(CmkLabel *self, PangoFontMask mask)
{
	CmkLabelPrivate *priv = PRIV(self);

	const PangoFontDescription *d = layout_font_desc(priv);
	if(!d)
		return;

	PangoFontDescription *copy = pango_font_description_copy_static(d);
	pango_font_description_unset_fields(copy, mask);

	if(pango_font_description_get_set_fields(copy) == 0)
		pango_layout_set_font_description(priv->layout, NULL);
	else
		pango_layout_set_font_description(priv->layout, copy);

	pango_font_description_free(copy);
}

void cmk_label_set_font_family(CmkLabel *self, const char *family)
{
	g_return_if_fail(CMK_IS_LABEL(self));

	if(family)
	{
		PangoFontDescription *desc = get_write_font_desc(self);
		pango_font_description_set_family(desc, family);
		pango_layout_set_font_description(PRIV(self)->layout, desc);
		pango_font_description_free(desc);
	}
	else
	{
		font_desc_unset(self, PANGO_FONT_MASK_FAMILY);
	}

	cmk_widget_relayout(CMK_WIDGET(self));
}

const char * cmk_label_get_font_family(CmkLabel *self)
{
	g_return_val_if_fail(CMK_IS_LABEL(self), NULL);

	const PangoFontDescription *desc = layout_font_desc(PRIV(self));
	if(!desc_has_set(desc, PANGO_FONT_MASK_FAMILY))
		desc = context_font_desc(PRIV(self));
	if(!desc_has_set(desc, PANGO_FONT_MASK_FAMILY))
		return NULL;

	return pango_font_description_get_family(desc);
}

void cmk_label_set_font_size(CmkLabel *self, float size)
{
	g_return_if_fail(CMK_IS_LABEL(self));

	if(size >= 0)
	{
		PangoFontDescription *desc = get_write_font_desc(self);
		pango_font_description_set_size(desc, size * PANGO_SCALE);
		pango_layout_set_font_description(PRIV(self)->layout, desc);
		pango_font_description_free(desc);
	}
	else
	{
		font_desc_unset(self, PANGO_FONT_MASK_SIZE);
	}

	cmk_widget_relayout(CMK_WIDGET(self));
}

float cmk_label_get_font_size(CmkLabel *self)
{
	g_return_val_if_fail(CMK_IS_LABEL(self), 0);

	const PangoFontDescription *desc = layout_font_desc(PRIV(self));
	if(!desc_has_set(desc, PANGO_FONT_MASK_SIZE))
		desc = context_font_desc(PRIV(self));
	if(!desc_has_set(desc, PANGO_FONT_MASK_SIZE))
		return 0;

	return (float)pango_font_description_get_size(desc) / PANGO_SCALE;
}

void cmk_label_set_single_line(CmkLabel *self, bool singleline)
{
	g_return_if_fail(CMK_IS_LABEL(self));
	if(PRIV(self)->singleline != singleline)
	{
		PRIV(self)->singleline = singleline;
		cmk_widget_relayout(CMK_WIDGET(self));
		g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_SINGLE_LINE]);
	}
}

bool cmk_label_get_single_line(CmkLabel *self)
{
	g_return_val_if_fail(CMK_IS_LABEL(self), false);
	return PRIV(self)->singleline;
}

void cmk_label_set_alignment(CmkLabel *self, PangoAlignment alignment)
{
	g_return_if_fail(CMK_IS_LABEL(self));
	if(pango_layout_get_alignment(PRIV(self)->layout) != alignment)
	{
		pango_layout_set_alignment(PRIV(self)->layout, alignment);
		// Alignment can't change allocation size, so only invalidate
		cmk_widget_invalidate(CMK_WIDGET(self), NULL);
	}
}

void cmk_label_set_bold(CmkLabel *self, bool bold)
{
	g_return_if_fail(CMK_IS_LABEL(self));

	PangoFontDescription *desc = get_write_font_desc(self);
	pango_font_description_set_weight(desc, bold ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL);
	pango_layout_set_font_description(PRIV(self)->layout, desc);
	pango_font_description_free(desc);

	cmk_widget_relayout(CMK_WIDGET(self));
	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_BOLD]);
}

bool cmk_label_get_bold(CmkLabel *self)
{
	g_return_val_if_fail(CMK_IS_LABEL(self), false);

	const PangoFontDescription *desc = layout_font_desc(PRIV(self));
	if(!desc_has_set(desc, PANGO_FONT_MASK_WEIGHT))
		desc = context_font_desc(PRIV(self));
	if(!desc_has_set(desc, PANGO_FONT_MASK_WEIGHT))
		return 0;

	return pango_font_description_get_weight(desc) >= PANGO_WEIGHT_BOLD;
}

PangoLayout * cmk_label_get_layout(CmkLabel *self)
{
	return PRIV(self)->layout;
}

static void set_pango_context(CmkLabel *self, PangoContext *context)
{
	CmkLabelPrivate *priv = PRIV(self);

	// If setting to default context and it's already default, ignore
	if(!context && priv->context == priv->defaultContextRef)
		return;

	// If the context hasn't changed at all, ignore
	if(context
	&& priv->contextIsSet
	&& context == priv->context
	&& pango_context_get_serial(context) == priv->contextSerial)
		return;

	// Update serial
	if(context)
		priv->contextSerial = pango_context_get_serial(context);

	// If the context object changed, get the
	// new one and recreate the layout
	if(!priv->context || priv->context != context)
	{
		priv->contextIsSet = (context != NULL);

		// Acquire new context
		PangoContext *newContext = context ? context : priv->defaultContextRef;
		g_object_ref(newContext);

		// Make layout with the new context
		PangoLayout *newLayout;
		if(priv->layout)
			newLayout = cmk_pango_layout_copy_new_context(priv->layout, newContext);
		else
			newLayout = pango_layout_new(newContext);

		// Replace layout and context
		g_clear_object(&priv->layout);
		g_clear_object(&priv->context);
		priv->layout = newLayout;
		priv->context = newContext;
	}
	else
	{
		// Otherwise just update the layout
		pango_layout_context_changed(priv->layout);
	}

	// Relayout
	cmk_widget_relayout(CMK_WIDGET(self));
	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_PANGO_CONTEXT]);
}

// Based on the pango_layout_copy() source code.
// This really needs to be part of the Pango API.
static PangoLayout * cmk_pango_layout_copy_new_context(PangoLayout *original, PangoContext *context)
{
	PangoLayout *new = pango_layout_new(context);

	const char *text = pango_layout_get_text(original);
	if(text)
		pango_layout_set_text(new, text, -1);

	PangoTabArray *tabs = pango_layout_get_tabs(original); // makes a copy
	if(tabs)
	{
		pango_layout_set_tabs(new, tabs);
		pango_tab_array_free(tabs);
	}

	pango_layout_set_font_description(new, pango_layout_get_font_description(original));
	pango_layout_set_attributes(new, pango_layout_get_attributes(original));
	pango_layout_set_width(new, pango_layout_get_width(original));
	pango_layout_set_height(new, pango_layout_get_height(original));
	pango_layout_set_indent(new, pango_layout_get_indent(original));
	pango_layout_set_spacing(new, pango_layout_get_spacing(original));
	pango_layout_set_justify(new, pango_layout_get_justify(original));
	pango_layout_set_alignment(new, pango_layout_get_alignment(original));
	pango_layout_set_single_paragraph_mode(new, pango_layout_get_single_paragraph_mode(original));
	pango_layout_set_auto_dir(new, pango_layout_get_auto_dir(original));
	pango_layout_set_wrap(new, pango_layout_get_wrap(original));
	pango_layout_set_ellipsize(new, pango_layout_get_ellipsize(original));

	return new;
}
