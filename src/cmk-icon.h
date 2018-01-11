/*
 * libcmk, Copyright 2018 Aidan Shafran <zelbrium@gmail.com>
 * Apache License 2.0 <www.apache.org/licenses/LICENSE-2.0>.
 */

/**
 * SECTION:cmk-icon
 * @TITLE: CmkIcon
 *
 * Displays an icon using the given or system default icon theme.
 */

#ifndef __CMK_ICON_H__
#define __CMK_ICON_H__

#include "cmk-widget.h"

#define CMK_TYPE_ICON cmk_icon_get_type()
G_DECLARE_DERIVABLE_TYPE(CmkIcon, cmk_icon, CMK, ICON, GInitiallyUnowned);
typedef struct _CmkIconClass CmkIconClass;

struct _CmkIconClass
{
	/*< private >*/
	CmkWidgetClass parentClass;
};

/**
 * cmk_icon_new:
 * @iconName: The name of the icon.
 * @size: The size to show the icon, in Cairo units.
 *
 * Create a new CmkIcon.
 */
CmkIcon * cmk_icon_new(const char *iconName, float size);

/**
 * cmk_icon_new_full:
 * @iconName: The name of the icon.
 * @theme: The name of the icon theme to use (e.g. "Adwaita"), or NULL.
 * @size: The size to show the icon, in Cairo units.
 * @useForeground: true to paint the icon using only the foreground
 *    color, or false to paint it with its real colors.
 *
 * Create a new CmkIcon using an icon from the given theme.
 * If theme is NULL, the default icon theme is used. The icon
 * will also automatically update if the default icon theme
 * changes.
 */
CmkIcon * cmk_icon_new_full(const char *iconName, const char *theme, float size, bool useForeground);

/**
 * cmk_icon_set_icon:
 * @iconName: The name of the icon.
 */
void cmk_icon_set_icon(CmkIcon *icon, const char *iconName);

/**
 * cmk_icon_set_icon:
 *
 * Returns: The name of the icon currently used,
 *    or NULL if using a pixmap.
 */
const char * cmk_icon_get_icon(CmkIcon *icon);

/**
 * cmk_icon_set_pixmap:
 * @icon: The icon
 * @data: A pixel buffer in @format pixel format, with @size pixels per
 *        row. Must be exactly size*size*[@format bytes]*@frames bytes
 *        long.
 * @format: The pixel format of the data buffer
 * @size: Size in pixels of the icon.
 * @frames: Number of frames of images in the animation. (Current unsupported)
 * @fps: Frames per second of the animation (Currently unsupported)
 *
 * Instead of using a named icon, use a pixel buffer (or animation). For a
 * static image, set frames to 1. Otherwise, buf should contain a sequence
 * of pixmaps in order and frames should be the number of pixmaps. 
 * Note that the pixmap must be a square, of width and height 'size'. This
 * does not need to be the same size as set with cmk_icon_set_size or
 * the #CmkIcon constructors; if it is not, the pixmap will be scaled.
 * The stride of the data buffer must be appropriate for the given format
 * and size.
 *
 * TODO: Animation not yet supported. Only the first frame will be used.
 */
void cmk_icon_set_pixmap(CmkIcon *icon, unsigned char *data, cairo_format_t format, unsigned int size, unsigned int frames, unsigned int fps);

/**
 * cmk_icon_set_size:
 *
 * Sets the square size of the icon, in Cairo units.
 */
void cmk_icon_set_size(CmkIcon *icon, float size);

/**
 * cmk_icon_get_size:
 *
 * Gets the icon size set with cmk_icon_set_size().
 */
float cmk_icon_get_size(CmkIcon *icon);

/**
 * cmk_icon_set_use_foreground_color:
 *
 * If the icon is set to use the foreground color, it will mask the entire
 * icon with the current foreground (font) color. This is useful for icons
 * which are solid-colored and should match the current theme, but this
 * should not be used on, for example, application icons.
 */
void cmk_icon_set_use_foreground_color(CmkIcon *icon, bool useForeground);

/**
 * cmk_icon_get_use_foreground_color:
 *
 * Gets the value set with cmk_icon_set_use_foreground_color().
 */
bool cmk_icon_get_use_foreground_color(CmkIcon *icon);

/**
 * cmk_icon_set_theme:
 * @theme: The name of the icon theme to use (e.g. "Adwaita"), or %NULL.
 *
 * Sets the icon theme to use by name. If set to %NULL (default),
 * the system default icon theme will be used.
 */
void cmk_icon_set_theme(CmkIcon *icon, const char *theme);

/**
 * cmk_icon_get_theme:
 *
 * Gets the icon theme set with cmk_icon_set_theme(), or %NULL
 * if using the default.
 */
const char * cmk_icon_get_icon_theme(CmkIcon *icon);

#endif
