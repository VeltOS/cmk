/*
 * This file is part of graphene-desktop, the desktop environment of VeltOS.
 * Copyright (C) 2016 Velt Technologies, Aidan Shafran <zelbrium@gmail.com>
 * Licensed under the Apache License 2 <www.apache.org/licenses/LICENSE-2.0>.
 */

#ifndef __CMK_ICON_H__
#define __CMK_ICON_H__

#include "cmk-widget.h"

G_BEGIN_DECLS

#define CMK_TYPE_ICON cmk_icon_get_type()
G_DECLARE_DERIVABLE_TYPE(CmkIcon, cmk_icon, CMK, ICON, CmkWidget);

typedef struct _CmkIconClass CmkIconClass;

struct _CmkIconClass
{
	CmkWidgetClass parentClass;
};

CmkIcon * cmk_icon_new(void);
CmkIcon * cmk_icon_new_from_name(const gchar *iconName);
CmkIcon * cmk_icon_new_full(const gchar *iconName, const gchar *themeName, gfloat size);

void cmk_icon_set_icon(CmkIcon *icon, const gchar *iconName);
const gchar * cmk_icon_get_icon(CmkIcon *icon);

/*
 * Sets the actor's size to be a square of side
 * length size*scale, where scale is the icon loader's GUI scale.
 * If the icon's size should match its surroundings instead of
 * staying fixed, just allocate the actor as needed or use a layout
 * manager; the icon will automatically adjust its size.
 */
void cmk_icon_set_size(CmkIcon *icon, gfloat size);
gfloat cmk_icon_get_size(CmkIcon *icon);

/*
 * Sets the icon theme to use by name. If set to NULL (default),
 * the icon theme chosen in GSettings will be used.
 */
void cmk_icon_set_icon_theme(CmkIcon *icon, const gchar *themeName);
const gchar * cmk_icon_get_icon_theme(CmkIcon *icon);

G_END_DECLS

#endif
