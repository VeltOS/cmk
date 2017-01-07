/*
 * This file is part of graphene-desktop, the desktop environment of VeltOS.
 * Copyright (C) 2016 Velt Technologies, Aidan Shafran <zelbrium@gmail.com>
 * Licensed under the Apache License 2 <www.apache.org/licenses/LICENSE-2.0>.
 */

#include "cmk-icon-loader.h"
#include <gio/gio.h>
#include <librsvg/rsvg.h>

typedef struct
{
	gchar *context;
	gchar *where; // Relative to theme
	guint size; // maxSize >= size >= minSize
	guint minSize;
	guint maxSize;
	gboolean scalable; // Shorthand for "minSize == maxSize"
	guint scale; // Not same as "scalable"; this is for icons on HiDPI screens using x2,x3,etc GUI scale
} IconThemeGroup;

enum
{
	FEXT_SVG=1,
	FEXT_PNG=2,
	//FEXT_JPG=4,
};

typedef struct _IconInfo IconInfo;
struct _IconInfo
{
	IconThemeGroup *group; // Do not free
	guchar extFlags; // Bitfield of FEXT_* file extension flags
	IconInfo *next; // Next version of this icon
};

typedef struct
{
	gchar *name;
	gchar *where; // Relative to root
	gchar **fallbacks; // NULL terminated
	gsize numGroups;
	IconThemeGroup *groups;
	
	// Key: icon name (string)
	// Value: IconInfo * (a linked list)
	GTree *icons;
} IconTheme;

struct _CmkIconLoader
{
	GObject parent;
	guint setScale;
	gchar *setDefaultTheme;
	GSettings *settings;
	GTree *themes;
};

enum
{
	PROP_SCALE = 1,
	PROP_DEFAULT_THEME,
	PROP_LAST
};

static GParamSpec *properties[PROP_LAST];

static void cmk_icon_loader_dispose(GObject *self_);
static void cmk_icon_loader_set_property(GObject *self_, guint propertyId, const GValue *value, GParamSpec *pspec);
static void cmk_icon_loader_get_property(GObject *self_, guint propertyId, GValue *value, GParamSpec *pspec);
static void on_scale_changed(CmkIconLoader *self);
static void on_default_theme_changed(CmkIconLoader *self);
static void free_icon_theme(IconTheme *theme);

G_DEFINE_TYPE(CmkIconLoader, cmk_icon_loader, G_TYPE_OBJECT);



CmkIconLoader * cmk_icon_loader_new(void)
{
	return CMK_ICON_LOADER(g_object_new(CMK_TYPE_ICON_LOADER, NULL));
}

CmkIconLoader * cmk_icon_loader_get_default(void)
{
	static CmkIconLoader *global = NULL;
	if(CMK_IS_ICON_LOADER(global))
		return g_object_ref(global);
	global = cmk_icon_loader_new();
	return global;
}

static void cmk_icon_loader_class_init(CmkIconLoaderClass *class)
{
	GObjectClass *base = G_OBJECT_CLASS(class);
	base->dispose = cmk_icon_loader_dispose;
	base->set_property = cmk_icon_loader_set_property;
	base->get_property = cmk_icon_loader_get_property;

	properties[PROP_SCALE] = g_param_spec_int("scale", "scale", "Global GUI scale", 0, 10, 1, G_PARAM_READWRITE);
	properties[PROP_DEFAULT_THEME] = g_param_spec_string("default-theme", "default-theme", "Global default icon theme", NULL, G_PARAM_READWRITE);

	g_object_class_install_properties(base, PROP_LAST, properties);
}

static void cmk_icon_loader_init(CmkIconLoader *self)
{
	self->setScale = 1;
	self->themes = g_tree_new_full((GCompareDataFunc)g_strcmp0, NULL, g_free, (GDestroyNotify)free_icon_theme);
	self->settings = g_settings_new("org.gnome.desktop.interface");
	g_signal_connect_swapped(self->settings, "changed::scaling-factor", G_CALLBACK(on_scale_changed), self);
	g_signal_connect_swapped(self->settings, "changed::icon-theme", G_CALLBACK(on_default_theme_changed), self);
}

static void cmk_icon_loader_dispose(GObject *self_)
{
	CmkIconLoader *self = CMK_ICON_LOADER(self_);
	g_clear_pointer(&self->themes, g_tree_unref);
	g_clear_pointer(&self->setDefaultTheme, g_free);
	g_clear_object(&self->settings);
	G_OBJECT_CLASS(cmk_icon_loader_parent_class)->dispose(self_);
}

static void cmk_icon_loader_set_property(GObject *self_, guint propertyId, const GValue *value, GParamSpec *pspec)
{
	g_return_if_fail(CMK_IS_ICON_LOADER(self_));
	CmkIconLoader *self = CMK_ICON_LOADER(self_);
	
	switch(propertyId)
	{
	case PROP_SCALE:
		cmk_icon_loader_set_scale(self, g_value_get_int(value));
		break;
	case PROP_DEFAULT_THEME:
		cmk_icon_loader_set_default_theme(self, g_value_get_string(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(self, propertyId, pspec);
		break;
	}
}

static void cmk_icon_loader_get_property(GObject *self_, guint propertyId, GValue *value, GParamSpec *pspec)
{
	g_return_if_fail(CMK_IS_ICON_LOADER(self_));
	CmkIconLoader *self = CMK_ICON_LOADER(self_);
	
	switch(propertyId)
	{
	case PROP_SCALE:
		g_value_set_int(value, cmk_icon_loader_get_scale(self));
		break;
	case PROP_DEFAULT_THEME:
		g_value_set_string(value, cmk_icon_loader_get_default_theme(self));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(self, propertyId, pspec);
		break;
	}
}

static void on_scale_changed(CmkIconLoader *self)
{
	if(self->setScale != 0)
		g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_SCALE]);
}

static void on_default_theme_changed(CmkIconLoader *self)
{
	if(!self->setDefaultTheme)
		g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_DEFAULT_THEME]);
}

void cmk_icon_loader_set_scale(CmkIconLoader *self, guint scale)
{
	g_return_if_fail(CMK_ICON_LOADER(self));
	if(self->setScale != scale)
	{
		self->setScale = scale;
		g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_SCALE]);
	}
}

guint cmk_icon_loader_get_scale(CmkIconLoader *self)
{
	g_return_val_if_fail(CMK_ICON_LOADER(self), 0);
	if(self->setScale != 0)
		return self->setScale;
	return g_settings_get_uint(self->settings, "scaling-factor");
}

void cmk_icon_loader_set_default_theme(CmkIconLoader *self, const gchar *theme)
{
	g_return_if_fail(CMK_ICON_LOADER(self));
	if(g_strcmp0(self->setDefaultTheme, theme) != 0)
	{
		g_free(self->setDefaultTheme);
		self->setDefaultTheme = g_strdup(theme);
		g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_DEFAULT_THEME]);
	}
}

const gchar * cmk_icon_loader_get_default_theme(CmkIconLoader *self)
{
	g_return_val_if_fail(CMK_ICON_LOADER(self), NULL);
	if(self->setDefaultTheme)
		return self->setDefaultTheme;
	return g_settings_get_string(self->settings, "icon-theme");
}

static void free_icon_theme(IconTheme *theme)
{
	g_free(theme->name);
	g_free(theme->where);
	g_strfreev(theme->fallbacks);
	for(gsize i=0;i<theme->numGroups;++i)
	{
		g_free(theme->groups[i].context);
		g_free(theme->groups[i].where);
	}
	g_free(theme->groups);
	if(theme->icons)
		g_tree_unref(theme->icons);
	g_free(theme);
}

static void free_icon_info_list(IconInfo *base)
{
	for(IconInfo *it=base;it!=NULL;)
	{
		IconInfo *next=it->next;
		g_free(it);
		it=next;
	}
}

static IconInfo * icon_info_list_add(IconInfo *base, IconThemeGroup *group, guint extFlag)
{
	for(IconInfo *it=base;it!=NULL;it=it->next)
	{
		if(it->group == group)
		{
			it->extFlags |= extFlag;
			return base;
		}
	}
	
	IconInfo *info = g_new(IconInfo, 1);
	info->group = group;
	info->extFlags = extFlag;
	info->next = NULL;

	if(!base)
		return info;

	// Doesn't matter where the new one goes as long as base isn't modified
	if(base->next)
		info->next = base->next;
	base->next = info;
	return base;
}

static guint fext_to_flag(const gchar *ext)
{
	guint extFlag = 0;
	if(g_strcmp0(ext, "svg") == 0)
		extFlag = FEXT_SVG;
	else if(g_strcmp0(ext, "png") == 0)
		extFlag = FEXT_PNG;
	return extFlag;
}

static void search_theme_group(IconTheme *theme, IconThemeGroup *group)
{
	if(!group->where || !theme->where)
		return;
	gchar *path = g_strdup_printf("%s/%s/", theme->where, group->where);
	GDir *dir = g_dir_open(path, 0, NULL);
	g_free(path);
	if(!dir)
		return;

	const gchar *entry = NULL;
	while(entry = g_dir_read_name(dir))
	{
		const gchar *extStart = g_strrstr(entry, ".");
		gsize basenameLength = extStart - entry;
		gchar *name = g_strndup(entry, basenameLength);
		guint extFlag = fext_to_flag(extStart+1);
		if(extFlag == 0)
			continue;

		IconInfo *infoList = NULL;
		if(g_tree_lookup_extended(theme->icons, name, NULL, (gpointer *)&infoList) && infoList)
			icon_info_list_add(infoList, group, extFlag);
		else
			g_tree_insert(theme->icons, name, icon_info_list_add(NULL, group, extFlag));
	}
	
	g_dir_close(dir);
}

static gboolean load_theme_group(GKeyFile *index, IconThemeGroup *group)
{
	group->size = g_key_file_get_integer(index, group->where, "Size", NULL);
	if(group->size == 0)
		return FALSE;

	gchar *type = g_key_file_get_string(index, group->where, "Type", NULL);
	if(!type || g_strcmp0(type, "Threshold") == 0)
	{
		GError *err = NULL;
		guint threshold = g_key_file_get_integer(index, group->where, "Threshold", &err);
		if(err)
		{
			threshold = 2; // Defaults to 2 according to XDG spec
			g_error_free(err);
		}
		group->maxSize = group->size + threshold;
		group->minSize = group->size - threshold;
		group->scalable = (group->maxSize == group->minSize);
	}
	else if(g_strcmp0(type, "Scalable") == 0)
	{
		group->minSize = g_key_file_get_integer(index, group->where, "MinSize", NULL);
		if(group->minSize == 0 || group->minSize > group->size)
			group->minSize = group->size;
		group->maxSize = g_key_file_get_integer(index, group->where, "MaxSize", NULL);
		if(group->maxSize == 0 || group->maxSize < group->size)
			group->maxSize = group->size;
		group->scalable = (group->maxSize == group->minSize);
	}
	else // Assume Fixed
	{
		group->minSize = group->size;
		group->maxSize = group->size;
		group->scalable = FALSE;
	}
	
	group->context = g_key_file_get_string(index, group->where, "Context", NULL);
	group->scale = g_key_file_get_integer(index, group->where, "Scale", NULL);
	if(group->scale == 0)
		group->scale = 1;
	return TRUE;
}

static IconTheme * load_theme_from(const gchar *themeName, const gchar *dir)
{
	gchar *path = g_strdup_printf("%s/%s/index.theme", dir, themeName);
	GKeyFile *index = g_key_file_new();
	gboolean r = g_key_file_load_from_file(index, path, G_KEY_FILE_NONE, NULL);
	g_free(path);
	if(!r || !index)
		return NULL;
	
	g_key_file_set_list_separator(index, ',');

	IconTheme *theme = g_new0(IconTheme, 1);
	theme->name = g_strdup(themeName);
	theme->where = g_strdup_printf("%s/%s/", dir, themeName);

	theme->fallbacks = g_key_file_get_string_list(index, "Icon Theme", "Inherits", NULL, NULL);
	theme->icons = g_tree_new_full((GCompareDataFunc)g_strcmp0, NULL, g_free, (GDestroyNotify)free_icon_info_list);

	// TODO: "ScaledDirectories" folder
	gchar **directories = g_key_file_get_string_list(index, "Icon Theme", "Directories", &theme->numGroups, NULL);
	if(!directories)
	{
		free_icon_theme(theme);
		g_object_unref(index);
		return NULL;
	}
	
	if(theme->numGroups)	
		theme->groups = g_new0(IconThemeGroup, theme->numGroups);

	gboolean noGroups = TRUE;
	for(gsize i=0;i<theme->numGroups;++i)
	{
		theme->groups[i].where = directories[i];
		if(load_theme_group(index, &theme->groups[i]))
		{
			search_theme_group(theme, &theme->groups[i]);
			noGroups = FALSE;
		}
		else
			g_clear_pointer(&(theme->groups[i].where), g_free);
	}

	g_free(directories); // The strings have been stolen by the Groups
	g_key_file_unref(index);

	if(noGroups)
	{
		free_icon_theme(theme);
		theme = NULL;
	}
	return theme;
}

static IconTheme * load_theme(const gchar *name)
{
	gchar *homeIconsDir = g_strdup_printf("%s/.icons", g_get_home_dir());
	
	IconTheme *theme;
	if(theme = load_theme_from(name, homeIconsDir))
	{
		g_free(homeIconsDir);
		return theme;
	}
	g_free(homeIconsDir);
	if(theme = load_theme_from(name, "/usr/share/local/icons"))
		return theme;
	if(theme = load_theme_from(name, "/usr/share/icons"))
		return theme;
	return NULL;
}

static IconTheme * get_theme(CmkIconLoader *self, const gchar *name)
{
	// TODO: Check for changes to directory mtime or index.theme
	IconTheme *theme = g_tree_lookup(self->themes, name);
	if(theme)
		return theme;
	theme = load_theme(name);
	if(theme)
		g_tree_insert(self->themes, g_strdup(name), theme);
	return theme;
}

inline static guint uint_diff(guint a, guint b)
{
	return (a>b) ? a-b : b-a;
}

static IconInfo * best_icon_from_info_list(IconInfo *infoList, guint size, guint scale)
{
	// First look for perfect size icon (scale and size match)
	for(IconInfo *it=infoList;it!=NULL;it=it->next)
		if(it->group->scale == scale && it->group->size == size)
			return it;
	
	// Look for 'pretty good' sized icon (scale matches, and size is within bounds)
	// This is differentiable from just finding the closest abs size as below,
	// because (for example) a 64x64@1x icon may not equal a 32x32@2x icon. The
	// scale should try to match first before finding the best size.
	for(IconInfo *it=infoList;it!=NULL;it=it->next)
		if(it->group->scale == scale && size >= it->group->minSize && size <= it->group->maxSize)
			return it;
	
	// Anything else is probably going to look equally bad, so just find the
	// icon with an abs scalable size closest to the abs pixel size requested.
	guint absSize = size * scale;
	guint closestDist = G_MAXUINT;
	IconInfo *icon = NULL;
	for(IconInfo *it=infoList;it!=NULL;it=it->next)
	{
		guint dMin = uint_diff(it->group->minSize*it->group->scale, absSize);
		guint dMax = uint_diff(it->group->maxSize*it->group->scale, absSize);
		guint d = MIN(dMin, dMax);
		if(d == 0)
			return it;
		if(d < closestDist)
		{
			closestDist = d;
			icon = it;
		}
	}
	return icon;
}

static gchar * find_icon_in_theme(IconTheme *theme, const gchar *name, guint size, guint scale)
{
	IconInfo *infoList = g_tree_lookup(theme->icons, name);
	if(!infoList)
		return NULL;
	
	IconInfo *icon = best_icon_from_info_list(infoList, size, scale);
	if(!icon)
		return NULL;

	/*
	 * We've found an icon, but there may be multiple filetypes for the same
	 * icon. Prefer a non-scalable type (ex. png) if the icon doesn't need
	 * to be scaled, since it'll load faster. Otherwise, prefer .svg.
	 */
	
	#define ICRETURN(ext) g_strdup_printf("%s/%s/%s." ext, theme->where, icon->group->where, name)

	gboolean needsScaling = (icon->group->size != size);
	if(needsScaling)
	{
		if((icon->extFlags & FEXT_SVG) == FEXT_SVG)
			return ICRETURN("svg");
		else if((icon->extFlags & FEXT_PNG) == FEXT_PNG)
			return ICRETURN("png");
	}
	else
	{
		if((icon->extFlags & FEXT_PNG) == FEXT_PNG)
			return ICRETURN("png");
		else if((icon->extFlags & FEXT_SVG) == FEXT_SVG)
			return ICRETURN("svg");
	}
	#undef ICRETURN
	return NULL;
}

gchar * cmk_icon_loader_lookup(CmkIconLoader *self, const gchar *name, guint size)
{
	return cmk_icon_loader_lookup_full(self, name, FALSE, NULL, TRUE, size, cmk_icon_loader_get_scale(self));
}

gchar * cmk_icon_loader_lookup_full(CmkIconLoader *self, const gchar *name, gboolean useFallbackNames, const gchar *themeName, gboolean useFallbackTheme, guint size, guint scale)
{
	g_return_val_if_fail(CMK_IS_ICON_LOADER(self), NULL);
	
	if(!themeName)
		themeName = cmk_icon_loader_get_default_theme(self);
	//if(!themeName)
	// 	just search pixmaps

	gboolean triedHicolor = FALSE;
	IconTheme *theme = get_theme(self, themeName);
	if(!theme)
	{
		if(!useFallbackTheme)
			return NULL; // Search pixmaps
		theme = get_theme(self, "hicolor");
		if(!theme)
			return NULL; // Search pixmaps
		triedHicolor = TRUE;
	}

	gchar *path = find_icon_in_theme(theme, name, size, scale);
	if(path)
		return path;

	if(!useFallbackTheme)
		return NULL; // TOOD: Search pixmaps

	if(theme->fallbacks)
	{
		for(guint i=0;theme->fallbacks[i]!=NULL;++i)
		{
			if(g_strcmp0(theme->fallbacks[i], "hicolor") == 0)
			{
				if(triedHicolor)
					continue;
				triedHicolor = TRUE;
			}
			IconTheme *t = get_theme(self, theme->fallbacks[i]);
			if(t)
				path = find_icon_in_theme(t, name, size, scale);
			if(path)
				return path;
		}
	}
	
	if(!triedHicolor)
	{
		theme = get_theme(self, "hicolor");
		path = find_icon_in_theme(theme, name, size, scale);
		if(path)
			return path;
	}

	// TODO: Search /usr/share/pixmaps

	// TODO: Fallback names
	return NULL;
}

static cairo_surface_t * load_svg(const gchar *path, guint size)
{
	RsvgHandle *handle = rsvg_handle_new_from_file(path, NULL);
	if(!handle)
		return NULL;

	cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, size, size);
	if(cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS)
	{
		g_object_unref(handle);
		return NULL;
	}

	cairo_t *cr = cairo_create(surface);
	if(cairo_status(cr) != CAIRO_STATUS_SUCCESS)
	{
		cairo_surface_destroy(surface);
		g_object_unref(handle);
		return NULL;
	}

	RsvgDimensionData dimensions;
	rsvg_handle_get_dimensions(handle, &dimensions);
	gdouble factor = MIN((gdouble)size / dimensions.width, (gdouble)size / dimensions.height);
	cairo_scale(cr, factor, factor);
	gboolean r = rsvg_handle_render_cairo(handle, cr);
	cairo_destroy(cr);
	g_object_unref(handle);
	if(!r)
		g_clear_pointer(&surface, cairo_surface_destroy);
	return surface;
}

static cairo_surface_t * load_png(const gchar *path, guint size)
{
	// TODO: Scale surface to size
	cairo_surface_t *surface = cairo_image_surface_create_from_png(path);
	if(cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS)
		g_clear_pointer(&surface, cairo_surface_destroy);
	return surface;
}

cairo_surface_t * cmk_icon_loader_load(CmkIconLoader *self, const gchar *path, guint size, guint scale, gboolean cache)
{
	if(!path)
		return NULL;

	size *= scale;

	cairo_surface_t *surface = NULL;
	if(g_str_has_suffix(path, ".svg"))
		surface = load_svg(path, size);
	else if(g_str_has_suffix(path, ".png"))
		surface = load_png(path, size);
	
	// TODO: Support other image types
	return surface;
}

cairo_surface_t * cmk_icon_loader_get(CmkIconLoader *self, const gchar *name, guint size)
{
	guint scale = cmk_icon_loader_get_scale(self);
	gchar *path = cmk_icon_loader_lookup_full(self, name, FALSE, NULL, TRUE, size, scale);
	cairo_surface_t *surface = cmk_icon_loader_load(self, path, size, scale, TRUE); 
	g_free(path);
	return surface;
}
