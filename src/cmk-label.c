/*
 * libcmk, Copyright 2018 Aidan Shafran <zelbrium@gmail.com>
 * Apache License 2.0 <www.apache.org/licenses/LICENSE-2.0>.
 */

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <pango/pangocairo.h>
#include "cmk-label.h"
#include "array.h"

#define PRIV(self) ((CmkLabelPrivate *)((self)->priv))

CmkLabelClass *cmk_label_class = NULL;

typedef struct
{
	PangoContext *context;
	PangoLayout *layout;
	bool singleline;

	int useDefaultFamily; // 0 no, 1 yes, 2 yes (mono)
	bool useDefaultSize;

} CmkLabelPrivate;

static void on_destroy(CmkWidget *self_);
static void on_draw(CmkWidget *self_, cairo_t *cr);
static void get_preferred_width(CmkWidget *self_, float forHeight, float *min, float *nat);
static void get_preferred_height(CmkWidget *self_, float forWidth, float *min, float *nat);
static void get_draw_rect(CmkWidget *self_, CmkRect *rect);
static void on_global_properties_changed(void *self_);
static void text_direction_changed(CmkWidget *self_, bool rtl);


CmkLabel * cmk_label_new(void)
{
	CmkLabel *self = calloc(sizeof(CmkLabel), 1);
	cmk_label_init(self);
	return self;
}

CmkLabel * cmk_label_new_with_text(const char *text)
{
	CmkLabel *label = cmk_label_new();
	cmk_label_set_text(label, text);
	return label;
}

void cmk_label_init(CmkLabel *self)
{
	cmk_widget_init((CmkWidget *)self);

	if(cmk_label_class == NULL)
	{
		cmk_label_class = calloc(sizeof(CmkLabelClass), 1);
		CmkWidgetClass *widgetClass = &cmk_label_class->parentClass;
		*widgetClass = *cmk_widget_class;
		widgetClass->destroy = on_destroy;
		widgetClass->draw = on_draw;
		widgetClass->get_preferred_width = get_preferred_width;
		widgetClass->get_preferred_height = get_preferred_height;
		widgetClass->get_draw_rect = get_draw_rect;
		widgetClass->text_direction_changed = text_direction_changed;
	}

	((CmkWidget *)self)->class = cmk_label_class;
	self->priv = calloc(sizeof(CmkLabelPrivate), 1);
	PRIV(self)->useDefaultFamily = 1;
	PRIV(self)->useDefaultSize = true;

	cmk_widget_set_event_mask((CmkWidget *)self, 0);

	// Listen for changes to global properties
	cmk_label_global_properties_listen(on_global_properties_changed, self);

	// Create Pango context
	PangoFontMap *fontmap = pango_cairo_font_map_get_default();
	PRIV(self)->context = pango_font_map_create_context(fontmap);

	unsigned int resolution;
	const PangoFontDescription *desc;
	bool baseRTL;
	cmk_label_get_global_font_properties(&resolution, &desc, NULL, &baseRTL);

	pango_cairo_context_set_resolution(PRIV(self)->context, resolution);
	pango_context_set_font_description(PRIV(self)->context, desc);
	pango_context_set_base_dir(PRIV(self)->context,
		baseRTL ? PANGO_DIRECTION_RTL : PANGO_DIRECTION_LTR);

	PRIV(self)->layout = pango_layout_new(PRIV(self)->context);
}

static void on_destroy(CmkWidget *self_)
{
	// Unlisten
	cmk_label_global_properties_listen(on_global_properties_changed, NULL);

	free(((CmkLabel *)self_)->priv);
	cmk_widget_class->destroy(self_);
}

static void on_draw(CmkWidget *self_, cairo_t *cr)
{
	PangoLayout *layout = PRIV((CmkLabel *)self_)->layout;

	pango_cairo_update_context(cr, PRIV((CmkLabel *)self_)->context);

	float width, height;
	cmk_widget_get_size(self_, &width, &height);
	pango_layout_set_width(layout, width * PANGO_SCALE);

	if(PRIV((CmkLabel *)self_)->singleline)
		pango_layout_set_height(layout, 0);
	else
		pango_layout_set_height(layout, height * PANGO_SCALE);

	pango_cairo_show_layout(cr, layout);
}

static void get_preferred_width(CmkWidget *self_, float forHeight, float *min, float *nat)
{
	PangoLayout *layout = PRIV((CmkLabel *)self_)->layout;
	int originalWidth = pango_layout_get_width(layout);
	int originalHeight = pango_layout_get_height(layout);

	pango_layout_set_width(layout, -1);

	if(PRIV((CmkLabel *)self_)->singleline)
		pango_layout_set_height(layout, 0);
	else if(forHeight >= 0)
		pango_layout_set_height(layout, forHeight * PANGO_SCALE);
	else
		pango_layout_set_height(layout, INT_MAX);

	PangoRectangle logicalRect = {0};
	pango_layout_get_extents(layout, NULL, &logicalRect);
	float logicalWidth = (float)(logicalRect.x + logicalRect.width) / PANGO_SCALE + 200;

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
	bool singleline = PRIV((CmkLabel *)self_)->singleline;
	PangoLayout *layout = PRIV((CmkLabel *)self_)->layout;
	int originalWidth = pango_layout_get_width(layout);
	int originalHeight = pango_layout_get_height(layout);

	if(forWidth < 0)
		forWidth = -1;

	pango_layout_set_width(layout, forWidth * PANGO_SCALE);

	if(singleline)
		pango_layout_set_height(layout, 0);
	else
		pango_layout_set_height(layout, INT_MAX);

	PangoRectangle logicalRect = {0};
	pango_layout_get_extents(layout, NULL, &logicalRect);
	float logicalHeight = (float)(logicalRect.y + logicalRect.height) / PANGO_SCALE;

	if(!singleline
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
	PangoLayout *layout = PRIV((CmkLabel *)self_)->layout;

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

static void on_global_properties_changed(void *self_)
{
	CmkLabel *self = self_;

	unsigned int resolution;
	const PangoFontDescription *desc;
	bool baseRTL;
	cmk_label_get_global_font_properties(&resolution, &desc, NULL, &baseRTL);

	pango_cairo_context_set_resolution(PRIV(self)->context, resolution);
	pango_context_set_font_description(PRIV(self)->context, desc);
	pango_context_set_base_dir(PRIV(self)->context,
		baseRTL ? PANGO_DIRECTION_RTL : PANGO_DIRECTION_LTR);
	
	pango_layout_context_changed(PRIV(self)->layout);

	// Update layout's family + size if set to default
	if(PRIV(self)->useDefaultFamily == 1) // yes
		cmk_label_set_font_family(self, NULL);
	else if(PRIV(self)->useDefaultFamily == 2) // yes (mono)
		cmk_label_set_font_family(self, "mono");

	if(PRIV(self)->useDefaultSize)
		cmk_label_set_font_size(self, -1);

	cmk_widget_invalidate((CmkWidget *)self, NULL);
}

static void text_direction_changed(CmkWidget *self_, bool rtl)
{
	pango_context_set_base_dir(PRIV((CmkLabel *)self_)->context,
		rtl ? PANGO_DIRECTION_RTL : PANGO_DIRECTION_LTR);
	pango_layout_context_changed(PRIV((CmkLabel *)self_)->layout);

	cmk_widget_class->text_direction_changed(self_, rtl);
}

void cmk_label_set_text(CmkLabel *self, const char *text)
{
	cmk_return_if_fail(self, )
	pango_layout_set_text(PRIV(self)->layout, text, -1);
}

void cmk_label_set_markup(CmkLabel *self, const char *markup)
{
	cmk_return_if_fail(self, )
	pango_layout_set_markup(PRIV(self)->layout, markup, -1);
}

const char * cmk_label_get_text(CmkLabel *self)
{
	cmk_return_if_fail(self, NULL)
	return pango_layout_get_text(PRIV(self)->layout);
}

static const PangoFontDescription * get_font_desc(CmkLabel *self)
{
	const PangoFontDescription *desc = pango_layout_get_font_description(PRIV(self)->layout);
	if(!desc)
		desc = pango_context_get_font_description(PRIV(self)->context);
	if(!desc)
		cmk_label_get_global_font_properties(NULL, &desc, NULL, NULL);
	return desc;
}

static PangoFontDescription * get_font_desc_copy(CmkLabel *self)
{
	return pango_font_description_copy(get_font_desc(self));
}

void cmk_label_set_font_family(CmkLabel *self, const char *family)
{
	cmk_return_if_fail(self, )

	PangoFontDescription *desc = get_font_desc_copy(self);

	if(family && strcmp(family, "mono") == 0)
	{
		PRIV(self)->useDefaultFamily = 2; // yes (mono)
		const PangoFontDescription *cdesc;
		cmk_label_get_global_font_properties(NULL, NULL, &cdesc, NULL);
		family = pango_font_description_get_family(cdesc);
	}
	else if(family)
	{
		PRIV(self)->useDefaultFamily = 0; // no
	}
	else
	{
		PRIV(self)->useDefaultFamily = 1; // yes
		const PangoFontDescription *cdesc;
		cmk_label_get_global_font_properties(NULL, &cdesc, NULL, NULL);
		family = pango_font_description_get_family(cdesc);
	}

	pango_font_description_set_family(desc, family);
	pango_layout_set_font_description(PRIV(self)->layout, desc);
	pango_font_description_free(desc);
}

const char * cmk_label_get_font_family(CmkLabel *self)
{
	cmk_return_if_fail(self, NULL)
	const PangoFontDescription *desc = get_font_desc(self);
	const char *family = pango_font_description_get_family(desc);
	return family ? family : "";
}

void cmk_label_set_font_size(CmkLabel *self, float size)
{
	cmk_return_if_fail(self, )

	PangoFontDescription *desc = get_font_desc_copy(self);

	if(size < 0)
	{
		PRIV(self)->useDefaultSize = true;
		const PangoFontDescription *cdesc;
		int defaultSize;

		if(PRIV(self)->useDefaultFamily == 2) // yes (mono)
			cmk_label_get_global_font_properties(NULL, NULL, &cdesc, NULL);
		else
			cmk_label_get_global_font_properties(NULL, &cdesc, NULL, NULL);

		defaultSize = pango_font_description_get_size(cdesc);
		if(pango_font_description_get_size_is_absolute(cdesc))
			pango_font_description_set_absolute_size(desc, defaultSize);
		else
			pango_font_description_set_size(desc, defaultSize);
	}
	else
	{
		PRIV(self)->useDefaultSize = false;
		pango_font_description_set_size(desc, size * PANGO_SCALE);
	}

	pango_layout_set_font_description(PRIV(self)->layout, desc);
	pango_font_description_free(desc);
}

float cmk_label_get_font_size(CmkLabel *self)
{
	cmk_return_if_fail(self, 0)
	const PangoFontDescription *desc = get_font_desc(self);
	return (float)pango_font_description_get_size(desc) / PANGO_SCALE;
}

void cmk_label_set_single_line(CmkLabel *self, bool singleline)
{
	cmk_return_if_fail(self, )
	if(PRIV(self)->singleline != singleline)
	{
		PRIV(self)->singleline = singleline;
		cmk_widget_invalidate((CmkWidget *)self, NULL);
	}
}

bool cmk_label_get_single_line(CmkLabel *self)
{
	cmk_return_if_fail(self, false)
	return PRIV(self)->singleline;
}

void cmk_label_set_alignment(CmkLabel *self, PangoAlignment alignment)
{
	cmk_return_if_fail(self, )
	pango_layout_set_alignment(PRIV(self)->layout, alignment);
}

void cmk_label_set_bold(CmkLabel *self, bool bold)
{
	cmk_return_if_fail(self, )
	PangoFontDescription *desc = get_font_desc_copy(self);
	pango_font_description_set_weight(desc, bold ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL);
	pango_layout_set_font_description(PRIV(self)->layout, desc);
	pango_font_description_free(desc);
}

PangoLayout * cmk_label_get_layout(CmkLabel *self)
{
	return PRIV(self)->layout;
}


typedef struct
{
	CmkLabelGlobalPropertiesListenCallback callback;
	void *userdata;
} Listener;

static unsigned int gResolution = 96;
static PangoFontDescription *gDefaultFont = NULL;
static PangoFontDescription *gDefaultMono = NULL;
static bool gBaseRTL = false;
static Listener *gListeners = NULL;

void cmk_label_set_global_font_properties(unsigned int resolution, const PangoFontDescription *defaultFont, const PangoFontDescription *defaultMono, bool baseRTL)
{
	gResolution = resolution;
	pango_font_description_free(gDefaultFont);
	pango_font_description_free(gDefaultMono);
	gDefaultFont = pango_font_description_copy(defaultFont);
	gDefaultMono = pango_font_description_copy(defaultMono);
	gBaseRTL = baseRTL;

	if(gListeners)
	{
		size_t size = array_size(gListeners);
		for(size_t i = 0; i < size; ++i)
			gListeners[i].callback(gListeners[i].userdata);
	}
}

void cmk_label_get_global_font_properties(unsigned int *resolution, const PangoFontDescription **defaultFont, const PangoFontDescription **defaultMono, bool *baseRTL)
{
	if(resolution) *resolution = gResolution;
	if(baseRTL) *baseRTL = gBaseRTL;
	if(defaultFont)
	{
		if(!gDefaultFont)
			gDefaultFont = pango_font_description_from_string("sans 11");
		*defaultFont = gDefaultFont;
	}
	if(defaultMono)
	{
		if(!gDefaultMono)
			gDefaultMono = pango_font_description_from_string("mono 11");
		*defaultMono = gDefaultMono;
	}
}

void cmk_label_global_properties_listen(CmkLabelGlobalPropertiesListenCallback callback, void *userdata)
{
	if(!gListeners)
		gListeners = array_new(sizeof(Listener), NULL);

	if(callback)
	{
		Listener l = {
			.callback = callback,
			.userdata = userdata
		};
		gListeners = array_append(gListeners, &l);
	}
	else
	{
		size_t size = array_size(gListeners);
		for(size_t i = size + 1; i != 0; --i)
			if(gListeners[i - 1].userdata == userdata)
				array_remove(gListeners, i - 1, false);
	}
}
