/*
 * libcmk, Copyright 2018 Aidan Shafran <zelbrium@gmail.com>
 * Apache License 2.0 <www.apache.org/licenses/LICENSE-2.0>.
 */

#include "cmk-palette.h"
#include <stdbool.h>

struct _CmkPalette
{
	GObject parent;

	CmkPalette *inherit;

	GArray *colors;
};

static const CmkColor kBackground = {1, 1, 1, 1};
static const CmkColor kForeground = {0, 0, 0, 1};
static const CmkColor kHover      = {0, 0, 0, 0.1};
static const CmkColor kSelected   = {0, 0, 0, 0.2};
static const CmkColor kAlert      = {0.827, 0.184, 0.184, 1};

static guint changeSignal;

G_DEFINE_TYPE(CmkPalette, cmk_palette, G_TYPE_OBJECT);


static void on_dispose(GObject *self_);
static void set_inherit(CmkPalette *self, CmkPalette *inherit);
static void on_inherit_changed(CmkPalette *self, CmkPalette *inherit);
static void free_named_color(CmkNamedColor *color);
static guint find_color(CmkPalette *self, const char *name);


static CmkPalette * cmk_palette_get(const char *name, CmkPalette **def, GType type)
{
	if(!*def)
		*def = cmk_palette_new(NULL);

	if(type == 0)
	{
		return *def;
	}
	else
	{
		GQuark q = g_quark_from_static_string(name);
		CmkPalette *p = g_type_get_qdata(type, q);
		if(!p)
		{
			p = cmk_palette_new(*def);
			g_type_set_qdata(type, q, p);
		}
		return p;
	}
}

CmkPalette * cmk_palette_get_base(GType type)
{
	static CmkPalette *def = NULL;
	return cmk_palette_get("cmk-palette-base", &def, type);
}

CmkPalette * cmk_palette_get_primary(GType type)
{
	static CmkPalette *def = NULL;
	return cmk_palette_get("cmk-palette-primary", &def, type);
}

CmkPalette * cmk_palette_get_secondary(GType type)
{
	static CmkPalette *def = NULL;
	return cmk_palette_get("cmk-palette-secondary", &def, type);
}

CmkPalette * cmk_palette_new(CmkPalette *inherit)
{
	CmkPalette *self = CMK_PALETTE(g_object_new(CMK_TYPE_PALETTE, NULL));
	set_inherit(self, inherit);
	return self;
}

static void cmk_palette_class_init(CmkPaletteClass *class)
{
	GObjectClass *base = G_OBJECT_CLASS(class);
	base->dispose = on_dispose;

	/**
	 * CmkPalette::change:
	 *
	 * The palette has changed and widgets should redraw themselves.
	 */
	changeSignal =
		g_signal_new("change",
		             G_TYPE_FROM_CLASS(class),
		             G_SIGNAL_RUN_FIRST,
		             0, NULL, NULL, NULL,
		             G_TYPE_NONE,
		             0);
}

static void cmk_palette_init(CmkPalette *self)
{
	self->colors = g_array_new(FALSE, TRUE, sizeof(CmkNamedColor));
	g_array_set_clear_func(self->colors, (GDestroyNotify)free_named_color);
}

static void on_dispose(GObject *self_)
{
	CmkPalette *self = CMK_PALETTE(self_);
	g_clear_object(&self->inherit);
	g_clear_pointer(&self->colors, (GDestroyNotify)g_array_unref);
	G_OBJECT_CLASS(cmk_palette_parent_class)->dispose(self_);
}

static void set_inherit(CmkPalette *self, CmkPalette *inherit)
{
	if(self->inherit)
		g_signal_handlers_disconnect_by_data(self->inherit, self);

	g_clear_object(&self->inherit);
	
	if(!inherit)
		return;

	self->inherit = g_object_ref(inherit);

	g_signal_connect_swapped(self->inherit,
	                 "change",
	                 G_CALLBACK(on_inherit_changed),
	                 self);

	on_inherit_changed(self, self->inherit);
}

static void on_inherit_changed(CmkPalette *self, UNUSED CmkPalette *inherit)
{
	g_signal_emit(self, changeSignal, 0);
}

static void free_named_color(CmkNamedColor *color)
{
	g_free(color->name);
}

static guint find_color(CmkPalette *self, const char *name)
{
	CmkNamedColor *colors = (CmkNamedColor *)self->colors->data;
	for(guint i = 0; i < self->colors->len; ++i)
		if(g_strcmp0(colors[i].name, name) == 0)
			return i;
	return -1;
}

bool cmk_palette_set_color_internal(CmkPalette *self, const char *name, const CmkColor *color)
{
	guint i = find_color(self, name);
	CmkNamedColor *named = (i == (guint)-1) ? NULL : &(((CmkNamedColor *)self->colors->data)[i]);

	if(named)
	{
		if(color)
			named->color = *color;
		else
			g_array_remove_index_fast(self->colors, i);
		return true;
	}
	else if(color)
	{
		CmkNamedColor new = {
			.name = g_strdup(name),
			.color = *color
		};
		g_array_append_val(self->colors, new);
		return true;
	}

	return false;
}

void cmk_palette_set_color(CmkPalette *self, const char *name, const CmkColor *color)
{
	g_return_if_fail(CMK_IS_PALETTE(self));

	if(cmk_palette_set_color_internal(self, name, color))
		g_signal_emit(self, changeSignal, 0);
}

void cmk_palette_set_colors(CmkPalette *self, const CmkNamedColor *colors)
{
	g_return_if_fail(CMK_IS_PALETTE(self));

	bool change = false;
	for(guint i = 0; colors[i].name != NULL; ++i)
		if(cmk_palette_set_color_internal(self, colors[i].name, &colors[i].color))
			change = true;

	if(change)
		g_signal_emit(self, changeSignal, 0);
}

const CmkColor * cmk_palette_get_color(CmkPalette *self, const char *name)
{
	g_return_val_if_fail(CMK_IS_PALETTE(self), NULL);

	guint i = find_color(self, name);
	if(i == (guint)-1)
	{
		if(self->inherit)
			return cmk_palette_get_color(self->inherit, name);
		else if(g_strcmp0(name, "background") == 0)
			return &kBackground;
		else if(g_strcmp0(name, "foreground") == 0)
			return &kForeground;
		else if(g_strcmp0(name, "hover") == 0)
			return &kHover;
		else if(g_strcmp0(name, "selected") == 0)
			return &kSelected;
		else if(g_strcmp0(name, "alert") == 0)
			return &kAlert;
		else
			return NULL;
	}
	else
	{
		return &(((CmkNamedColor *)self->colors->data)[i].color);
	}
}
