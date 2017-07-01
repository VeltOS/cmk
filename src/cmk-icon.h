/*
 * This file is part of graphene-desktop, the desktop environment of VeltOS.
 * Copyright (C) 2016 Velt Technologies, Aidan Shafran <zelbrium@gmail.com>
 * Licensed under the Apache License 2 <www.apache.org/licenses/LICENSE-2.0>.
 */

#ifndef __CMK_ICON_H__
#define __CMK_ICON_H__

#include "cmk-widget.h"
#include <cairo.h>

G_BEGIN_DECLS

#define CMK_TYPE_ICON cmk_icon_get_type()
G_DECLARE_DERIVABLE_TYPE(CmkIcon, cmk_icon, CMK, ICON, CmkWidget);

/**
 * SECTION:cmk-icon
 * @TITLE: CmkIcon
 * @SHORT_DESCRIPTION: Static icon
 *
 * Draws a icon loaded from a system icon theme. Icons must be given
 * a set pixel size, such as 32x32, but this is automatically scaled
 * by the widget's dp value. For example, if you set the icon's size
 * to 32 at a dp scale of 2.1, a 64x64 icon will be loaded, and then
 * scaled up to 67.2x67.2 pixels. As many icons are loaded from SVG,
 * this should still result in a clean-looking icon.
 */

typedef struct _CmkIconClass CmkIconClass;

struct _CmkIconClass
{
	/*< private >*/
	CmkWidgetClass parentClass;
};

/**
 * cmk_icon_new:
 *
 * Creates a new, unspecified icon of size @size.
 */
CmkIcon * cmk_icon_new(gfloat size);
CmkIcon * cmk_icon_new_from_name(const gchar *iconName, gfloat size);

/**
 * cmk_icon_new_full:
 * @iconName: The name of the icon to show
 * @themeName: The system icon theme to use. If %NULL, uses the default,
 *             and will automatically update if the default is changed.
 * @size: Icon size
 * @useForeground: %TRUE to mask the icon with the current foreground color.
 *                 See cmk_icon_set_use_foreground_color() for details.
 */
CmkIcon * cmk_icon_new_full(const gchar *iconName, const gchar *themeName, gfloat size, gboolean useForeground);

void cmk_icon_set_icon(CmkIcon *icon, const gchar *iconName);
const gchar * cmk_icon_get_icon(CmkIcon *icon);

/**
 * cmk_icon_set_pixmap:
 * @icon: The icon
 * @data: A pixel buffer in @format pixel format, with @size pixels per
 *        row. Must be exactly size*size*[@format bytes]*@frames bytes
 *        long.
 * @format: The pixel format of the data buffer
 * @size: Size in pixels of the icon. This is NOT scaled by the current
 *        dp; you must specify a scaled icon manually.
 * @frames: Number of frames of images in the animation. (Current unsupported)
 * @fps: Frames per second of the animation (Currently unsupported)
 *
 * Instead of using a named icon, use a pixel buffer (or animation). For a
 * static image, set frames to 1. Otherwise, buf should contain a sequence
 * of pixmaps in order and frames should be the number of pixmaps. 
 * Note that the pixmap must be a square, of width and height 'size'. This
 * does not need to be the same size as set with cmk_icon_set_size or
 * the CmkIcon constructors; if it is not, the pixmap will be scaled.
 * The stride of the data buffer must be appropriate for the given format
 * and size.
 *
 * TODO: Animation not yet supported. Only the first frame will be used.
 */
void cmk_icon_set_pixmap(CmkIcon *icon, guchar *data, cairo_format_t format, guint size, guint frames, guint fps);

/**
 * cmk_icon_set_size:
 *
 * Sets the square size of the icon. This automatically takes into
 * account the widget's dp scale and adjusts the size appropriately.
 */
void cmk_icon_set_size(CmkIcon *icon, gfloat size);
gfloat cmk_icon_get_size(CmkIcon *icon);

/**
 * cmk_icon_set_use_foreground_color:
 *
 * If the icon is set to use the foreground color, it will mask the entire
 * icon with the current foreground (font) color. This is useful for icons
 * which are solid-colored and should match the current theme, but should
 * not be used on, for example, application icons.
 */
void cmk_icon_set_use_foreground_color(CmkIcon *icon, gboolean useForeground);
gboolean cmk_icon_get_use_foreground_color(CmkIcon *icon);

/**
 * cmk_icon_set_icon_theme:
 *
 * Sets the icon theme to use by name. If set to %NULL (default),
 * the icon theme chosen in #GSettings will be used.
 */
void cmk_icon_set_icon_theme(CmkIcon *icon, const gchar *themeName);
const gchar * cmk_icon_get_icon_theme(CmkIcon *icon);

G_END_DECLS

#endif
