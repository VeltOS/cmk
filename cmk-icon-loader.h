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
 * Sets the current GUI scale. This is used to find icons that were designed
 * for scaled GUIs. Size requests passed to cmk_icon_loader_lookup(_full)
 * should always use sizes for 1x scale GUI scale.
 */
void cmk_icon_loader_set_scale(CmkIconLoader *loader, guint scale);
guint cmk_icon_loader_get_scale(CmkIconLoader *loader);

const gchar * cmk_icon_loader_get_default_theme(CmkIconLoader *loader);

/*
 * Lookup an icon's file path using the current default icon theme.
 * Size is icon size in pixels. The icon with the closest available size
 * is returned.
 * Equivelent to calling
 * cmk_icon_loader_lookup_full(loader, name, TRUE, NULL, TRUE, size, FALSE);
 */
gchar * cmk_icon_loader_lookup(CmkIconLoader *loader, const gchar *name, guint size);

gchar * cmk_icon_loader_lookup_full(CmkIconLoader *self, const gchar *name, gboolean useFallbackNames, const gchar *theme, gboolean useFallbackTheme, guint size, guint scale);

/*
 * Loads the icon. Set cache to TRUE if this icon is to be loaded often.
 */
//GdkPixbuf * cmk_icon_loader_load(CmkIconLoader *loader, const gchar *path, gboolean cache);

/* Clears lookup and load cache */
void cmk_icon_loader_clear_cache(CmkIconLoader *loader);

G_END_DECLS

#endif
