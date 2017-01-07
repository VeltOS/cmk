/*
 * This file is part of graphene-desktop, the desktop environment of VeltOS.
 * Copyright (C) 2016 Velt Technologies, Aidan Shafran <zelbrium@gmail.com>
 * Licensed under the Apache License 2 <www.apache.org/licenses/LICENSE-2.0>.
 */

#ifndef __CMK_ICON_LOADER_H__
#define __CMK_ICON_LOADER_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define CMK_TYPE_ICON_LOADER cmk_icon_loader_get_type()
G_DECLARE_FINAL_TYPE(CmkIconLoader, cmk_icon_loader, CMK, ICON_LOADER, GObject);

CmkIconLoader * cmk_icon_loader_new(void);

/* Returns new ref to default loader or creates new loader if no default */
CmkIconLoader * cmk_icon_loader_get_default(void);

void cmk_icon_loader_test(CmkIconLoader *loader);

/*
 * Sets the current GUI scale. This is used by cmk_icon_loader_lookup
 * for getting a scale. If 0 is passed for the scale, dconf will be
 * used (monitoring /org/gnome/desktop/interface/scaling-factor) to
 * find the GUI scale. The default value for scale is 1.
 */
void cmk_icon_loader_set_scale(CmkIconLoader *loader, guint scale);
guint cmk_icon_loader_get_scale(CmkIconLoader *loader);

/*
 * Set the default theme. If NULL is passed for theme, dconf will be
 * used (monitoring /org/gnome/desktop/interface/icon-theme) to get
 * the default theme. By default, CmkIconLoader uses the dconf setting.
 * CmkIconLoader's notify::default-theme signal should be watched
 * to see when the default theme changes in order to update icons live.
 */
void cmk_icon_loader_set_default_theme(CmkIconLoader *loader, const gchar *theme);
const gchar * cmk_icon_loader_get_default_theme(CmkIconLoader *loader);

/*
 * Set the value returned by get_default_theme. If NUjj
 */

/*
 * Lookup an icon's file path using the current default icon theme.
 * Equivelent to calling
 * cmk_icon_loader_lookup_full(loader, name, FALSE, NULL, TRUE, size, [get_scale]);
 */
gchar * cmk_icon_loader_lookup(CmkIconLoader *loader, const gchar *name, guint size);

/*
 * Looks up an icon's file path.
 * name: Icon's name.
 * useFallbackNames: If the icon is not found, attempt to shorten the name
 *          at hyphens and try again. For example, if the requested icon is
 *          'printer-remote', and it isn't found, this function will then
 *          try 'printer' before giving up and returning NULL.
 * theme: Theme name to use, or NULL to use the current default theme.
 * useFallbackTheme: If the icon is not found in the given theme, try its
 *			fallbacks in order down to hicolor.
 * size: Icon size in pixels. (ie. size=32 looks for a 32x32 sized icon)
 *       If the exact size requested isn't found, the lookup may find an
 *       icon which can be scaled to the requested size instead.
 * scale: The current GUI scale. This is probably a DE-global value.
 * 		  The icon size passed to 'size' must NOT be affected by this scale.
 */
gchar * cmk_icon_loader_lookup_full(CmkIconLoader *self, const gchar *name, gboolean useFallbackNames, const gchar *theme, gboolean useFallbackTheme, guint size, guint scale);

/*
 * Loads the icon. Set cache to TRUE if this icon is to be loaded often.
 */
//GdkPixbuf * cmk_icon_loader_load(CmkIconLoader *loader, const gchar *path, gboolean cache);

G_END_DECLS

#endif
