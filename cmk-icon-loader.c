/*
 * This file is part of graphene-desktop, the desktop environment of VeltOS.
 * Copyright (C) 2016 Velt Technologies, Aidan Shafran <zelbrium@gmail.com>
 * Licensed under the Apache License 2 <www.apache.org/licenses/LICENSE-2.0>.
 */

#include "cmk-icon-loader.h"

typedef struct
{
	gchar *context;
	gchar *where; // Relative to theme
	guint size; // maxSize >= size >= minSize
	guint minSize;
	guint maxSize;
	gboolean scalable; // Shorthand for "minSize == maxSize"
	guint scale; // Not same as "scalable"; this is for icons on HiDPI screens using x2,x3,etc GUI scale
	
	// When this group is first used for looking up an icon, this icon list is
	// populated. It acts as a cache, and should be cleared if the mtime on
	// the theme's directory or the index.theme file is changed.
	GTree *icons;
} IconThemeGroup;

typedef struct
{
	gchar *name;
	gchar *where; // Relative to root
	gchar **fallbacks; // NULL terminated
	gsize numGroups;
	IconThemeGroup *groups;
} IconTheme;

struct _CmkIconLoader
{
	GObject parent;
	guint scale;
	GTree *themes;
};

enum
{
	SIGNAL_ICON_THEME_CHANGED = 1,
	SIGNAL_LAST
};

static void cmk_icon_loader_dispose(GObject *self_);
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
	G_OBJECT_CLASS(class)->dispose = cmk_icon_loader_dispose;
}

static void cmk_icon_loader_init(CmkIconLoader *self)
{
	self->themes = g_tree_new_full((GCompareDataFunc)g_strcmp0, NULL, g_free, (GDestroyNotify)free_icon_theme);
}

static void cmk_icon_loader_dispose(GObject *self_)
{
	CmkIconLoader *self = CMK_ICON_LOADER(self_);
	g_clear_pointer(&self->themes, g_tree_unref);
	G_OBJECT_CLASS(cmk_icon_loader_parent_class)->dispose(self_);
}

void cmk_icon_loader_set_scale(CmkIconLoader *self, guint scale)
{
	g_return_if_fail(CMK_ICON_LOADER(self));
	g_return_if_fail(scale == 0);
	self->scale = scale;
}

guint cmk_icon_loader_get_scale(CmkIconLoader *self)
{
	g_return_val_if_fail(CMK_ICON_LOADER(self), 0);
	return self->scale;
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
		if(theme->groups[i].icons)
			g_tree_unref(theme->groups[i].icons);
	}
	g_free(theme->groups);
	g_free(theme);
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
			noGroups = FALSE;
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
	IconTheme *theme = g_tree_lookup(self->themes, name);
	if(theme)
		return theme;
	theme = load_theme(name);
	if(theme)
		g_tree_insert(self->themes, g_strdup(name), theme);
	return theme;
}

static GList * get_group_list_for_icon(IconTheme *theme, const gchar *name, guint size, guint scale)
{
	GList *groups = NULL;
	
	// Find all groups that have a size range that 'size' falls within
	// 'groups' is reverse-ordered, but it will be reversed again as they
	// move to 'ordered'
	for(gsize i=0;i<theme->numGroups;++i)
	{
		if(size >= theme->groups[i].minSize && size <= theme->groups[i].maxSize)
			groups = g_list_prepend(groups, &theme->groups[i]);
	}
	
	GList *ordered = NULL;	

	/*
	 * Order
	 * XDG doesn't give a specific order to look for icons, so I'm using:
	 * 1. Perfect size, correct GUI scale
	 * 2. Scalable, correct GUI scale
	 * 3. Scalable, wrong GUI scale
	 * 4. Perfect size, wrong GUI scale
 	 */	

	// Moves all groups from 'groups' to 'ordered' that match the given condition
	// Reverses 'groups', making all items matching condition be added to 'ordered'
	// in the same order they were in 'theme->groups'
	#define GROUPS_MATCHING_COND(condition) \
		for(GList *it=groups;it!=NULL;) { \
			IconThemeGroup *group = (IconThemeGroup *)it->data; \
			GList *next = it->next; \
			if(condition) { \
				groups = g_list_remove_link(groups, it); \
				ordered = g_list_concat(it, ordered); \
			} \
			it = next; \
		}

	// Add condition sets in reverse order, so the final 'ordered' list has
	// the best-matching groups at the start
	GROUPS_MATCHING_COND(group->size == size && group->scale != scale)
	GROUPS_MATCHING_COND(group->size != size && group->scale != scale)
	GROUPS_MATCHING_COND(group->size != size && group->scale == scale)
	GROUPS_MATCHING_COND(group->size == size && group->scale == scale)
	#undef GROUPS_MATCHING_COND
	// 'groups' should always have 0 items after this

	return ordered;
}

static void free_ext_list(gpointer extList)
{
	g_list_free_full((GList *)extList, g_free);
}

static void populate_group_icons(IconTheme *theme, IconThemeGroup *group)
{
	// Don't bother if the group has already been populated
	if(group->icons)
		return;

	gchar *path = g_strdup_printf("%s/%s/", theme->where, group->where);
	GDir *dir = g_dir_open(path, 0, NULL);
	g_free(path);
	if(!dir)
		return;

	group->icons = g_tree_new_full((GCompareDataFunc)g_strcmp0, NULL, g_free, free_ext_list);

	const gchar *entry = NULL;
	while(entry = g_dir_read_name(dir))
	{
		const gchar *extStart = g_strrstr(entry, ".");
		gsize basenameLength = extStart - entry;
		gchar *name = g_strndup(entry, basenameLength);
		gchar *ext = g_strdup(extStart+1);
		GList *extList = NULL;
		GList *avoidWarning = NULL; // Return value of append isn't necessary here
		// If the icon already exists, add this new extension
		if(g_tree_lookup_extended(group->icons, name, NULL, (gpointer *)&extList) && extList)
			avoidWarning = g_list_append(extList, ext);
		else
			g_tree_insert(group->icons, name, g_list_append(NULL, ext));
	}
	
	g_dir_close(dir);
}

static gchar * find_icon_in_theme(IconTheme *theme, const gchar *name, guint size, guint scale)
{
	// Get a list of groups this icon could possibly be in, ordered from
	// best to worst
	GList *groups = get_group_list_for_icon(theme, name, size, scale);
	
	// Look for the icon
	IconThemeGroup *group = NULL;
	GList *exts = NULL;
	for(GList *it=groups;it!=NULL;it=it->next)
	{
		group = (IconThemeGroup *)(it->data);
		populate_group_icons(theme, group);
		exts = g_tree_lookup(group->icons, name);
		if(exts)
			break;
	}
	
	if(!exts || !group)
		return NULL;
	
	/*
	 * We've found an icon, but there may be multiple filetypes for the same
	 * icon. Prefer a non-scalable type (ex. png) if the icon doesn't need
	 * to be scaled, since it'll load faster. Otherwise, prefer .svg.
	 */
	
	gboolean needsScaling = (group->size != size);
	if(needsScaling)
	{
		for(GList *it=exts;it!=NULL;it=it->next)
		{
			if(g_strcmp0(it->data, "svg") == 0)
				return g_strdup_printf("%s/%s/%s.svg", theme->where, group->where, name);
		}
	}
	else
	{
		for(GList *it=exts;it!=NULL;it=it->next)
		{
			if(g_strcmp0(it->data, "svg") != 0)
				return g_strdup_printf("%s/%s/%s.%s", theme->where, group->where, name, it->data);
		}
	}
	
	return g_strdup_printf("%s/%s/%s.%s", theme->where, group->where, name, exts->data);
}

gchar * cmk_icon_loader_lookup(CmkIconLoader *self, const gchar *name, guint size)
{
	return cmk_icon_loader_lookup_full(self, name, FALSE, NULL, TRUE, size, self->scale);
}

gchar * cmk_icon_loader_lookup_full(CmkIconLoader *self, const gchar *name, gboolean useFallbackNames, const gchar *themeName, gboolean useFallbackTheme, guint size, guint scale)
{
	g_return_val_if_fail(CMK_IS_ICON_LOADER(self), NULL);
	
	//if(!themeName)
	//	themeName = cmk_icon_loader_get_default_theme(self);
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
			if(triedHicolor && g_strcmp0(theme->fallbacks[i], "hicolor") == 0)
				continue;
			theme = get_theme(self, theme->fallbacks[i]);
			gchar *path = find_icon_in_theme(theme, name, size, scale);
			if(path)
				return path;
		}
	}
	
	if(!triedHicolor)
	{
		theme = get_theme(self, "hicolor");
		gchar *path = find_icon_in_theme(theme, name, size, scale);
		if(path)
			return path;
	}

	// TODO: Search /usr/share/pixmaps

	// TODO: Fallback names
	return NULL;
}

void cmk_icon_loader_test(CmkIconLoader *self)
{
	gchar *path;
	for(int i=0;i<1000;++i)
		path = cmk_icon_loader_lookup_full(self, "telegram", FALSE, "Paper", TRUE, 32, 1); 
	g_message("path: %s", path);
}
